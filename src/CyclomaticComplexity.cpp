//===- CyclomaticComplexity.cpp - Clang plugin ----------------------------===//
//
// Reports the cyclomatic complexity of every function defined in the file
// being compiled.
//
// Cyclomatic complexity (McCabe, 1976) is the number of linearly independent
// paths through a function. Formally, on a control-flow graph:
//
//     M = E - N + 2P
//
// where E = edges, N = nodes, P = connected components. For a single function
// P = 1, and the formula collapses to a much easier equivalent:
//
//     M = (number of decision points) + 1
//
// A decision point is anywhere control can branch two ways. That equivalence
// is why this plugin counts branch constructs on the AST rather than building
// a CFG: same number, far less machinery.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace clang;

namespace {

/// Counts decision points inside one function body.
///
/// This is a plain recursive walk rather than a RecursiveASTVisitor because
/// we need to stop at nested function boundaries (lambdas, blocks): their
/// complexity belongs to them, not to the enclosing function.
class DecisionPointCounter {
public:
  int count(const Stmt *S) {
    if (!S)
      return 0;

    // A lambda body is a separate function. Do not fold its branches into
    // the parent's score -- VisitFunctionDecl will reach it on its own.
    if (isa<LambdaExpr>(S))
      return 0;

    int Points = isDecisionPoint(S) ? 1 : 0;
    for (const Stmt *Child : S->children())
      Points += count(Child);
    return Points;
  }

private:
  /// Does this node add an independent path through the function?
  static bool isDecisionPoint(const Stmt *S) {
    // Straightforward branches.
    if (isa<IfStmt>(S) || isa<ForStmt>(S) || isa<WhileStmt>(S) ||
        isa<DoStmt>(S) || isa<CXXForRangeStmt>(S) || isa<CXXCatchStmt>(S))
      return true;

    // switch: count each `case`, NOT the SwitchStmt and NOT `default`.
    //
    // A switch with three cases is three independent paths, so counting the
    // SwitchStmt as one (as the 2024 version did) understates it badly.
    // `default` is the fall-through path that already exists, so it adds none.
    if (isa<CaseStmt>(S))
      return true;

    // Ternary `c ? a : b`, and the GNU `c ?: b` form, which Clang models as
    // BinaryConditionalOperator. Both branch.
    if (isa<ConditionalOperator>(S) || isa<BinaryConditionalOperator>(S))
      return true;

    // && and || short-circuit, so each one is a branch. Note this is
    // deliberately restricted to logical operators: `a + b` is not a branch.
    if (const auto *BO = dyn_cast<BinaryOperator>(S))
      return BO->isLogicalOp();

    // GNU computed goto (`goto *label;`) -- an indirect branch.
    if (isa<IndirectGotoStmt>(S))
      return true;

    return false;
  }
};

struct FunctionResult {
  std::string Name;
  unsigned Line;
  int Complexity;
};

class CyclomaticComplexityVisitor
    : public RecursiveASTVisitor<CyclomaticComplexityVisitor> {
public:
  CyclomaticComplexityVisitor(ASTContext &Context,
                              std::vector<FunctionResult> &Results)
      : Context(Context), Results(Results) {}

  bool VisitFunctionDecl(FunctionDecl *FD) {
    // Declarations without a body have no control flow to measure.
    if (!FD->hasBody() || !FD->isThisDeclarationADefinition())
      return true;

    // Without this, every function pulled in from <stdio.h> and friends gets
    // reported -- thousands of lines of noise for a ten-line input file.
    SourceLocation Loc = FD->getLocation();
    if (!Context.getSourceManager().isInMainFile(Loc))
      return true;

    DecisionPointCounter Counter;
    int Complexity = Counter.count(FD->getBody()) + 1;

    Results.push_back({FD->getQualifiedNameAsString(),
                       Context.getSourceManager().getSpellingLineNumber(Loc),
                       Complexity});
    return true;
  }

private:
  ASTContext &Context;
  std::vector<FunctionResult> &Results;
};

class CyclomaticComplexityConsumer : public ASTConsumer {
public:
  explicit CyclomaticComplexityConsumer(std::string OutputPath)
      : OutputPath(std::move(OutputPath)) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    std::vector<FunctionResult> Results;
    CyclomaticComplexityVisitor Visitor(Context, Results);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());

    if (OutputPath.empty()) {
      for (const FunctionResult &R : Results)
        llvm::outs() << R.Line << "\t" << R.Complexity << "\t" << R.Name << "\n";
      return;
    }

    std::ofstream Out(OutputPath);
    if (!Out) {
      llvm::errs() << "cyclomatic-complexity: cannot write to '" << OutputPath
                   << "'\n";
      return;
    }
    for (const FunctionResult &R : Results)
      Out << R.Line << "\t" << R.Complexity << "\t" << R.Name << "\n";
  }

private:
  std::string OutputPath;
};

class CyclomaticComplexityAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &,
                                                 llvm::StringRef) override {
    return std::make_unique<CyclomaticComplexityConsumer>(OutputPath);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &Args) override {
    for (size_t I = 0; I < Args.size(); ++I) {
      if (Args[I] == "-o" && I + 1 < Args.size()) {
        OutputPath = Args[++I];
      } else if (Args[I] == "-help") {
        llvm::errs() << "  -o <file>   write results to <file> "
                        "(default: stdout)\n";
        return false;
      } else {
        // Returning false aborts compilation with a diagnostic, which is the
        // correct response to a typo in a build flag -- silently ignoring it
        // would produce an empty report that looks like a clean result.
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                        "unknown plugin argument '%0'");
        D.Report(ID) << Args[I];
        return false;
      }
    }
    return true;
  }

  /// Run automatically after the main compilation action, so the plugin works
  /// with a plain `-fplugin=` invocation and does not replace codegen.
  PluginASTAction::ActionType getActionType() override {
    return AddAfterMainAction;
  }

private:
  std::string OutputPath;
};

} // namespace

static FrontendPluginRegistry::Add<CyclomaticComplexityAction>
    X("cyclomatic-complexity",
      "report cyclomatic complexity of each function");
