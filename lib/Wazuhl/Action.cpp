#include "llvm/Wazuhl/Action.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasAnalysisEvaluator.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/CFLAndersAliasAnalysis.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/IVUsers.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/Analysis/OptimizationDiagnosticInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"

#include "llvm/CodeGen/PreISelIntrinsicLowering.h"
#include "llvm/CodeGen/UnreachableBlockElim.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Target/TargetMachine.h"

#include "llvm/Transforms/GCOVProfiler.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/ConstantMerge.h"
#include "llvm/Transforms/IPO/CrossDSOCFI.h"
#include "llvm/Transforms/IPO/DeadArgumentElimination.h"
#include "llvm/Transforms/IPO/ElimAvailExtern.h"
#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionImport.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/IPO/GlobalOpt.h"
#include "llvm/Transforms/IPO/GlobalSplit.h"
#include "llvm/Transforms/IPO/InferFunctionAttrs.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/IPO/Internalize.h"
#include "llvm/Transforms/IPO/LowerTypeTests.h"
#include "llvm/Transforms/IPO/PartialInlining.h"
#include "llvm/Transforms/IPO/SCCP.h"
#include "llvm/Transforms/IPO/StripDeadPrototypes.h"
#include "llvm/Transforms/IPO/WholeProgramDevirt.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/InstrProfiling.h"
#include "llvm/Transforms/PGOInstrumentation.h"
#include "llvm/Transforms/SampleProfile.h"
#include "llvm/Transforms/Scalar/ADCE.h"
#include "llvm/Transforms/Scalar/AlignmentFromAssumptions.h"
#include "llvm/Transforms/Scalar/BDCE.h"
#include "llvm/Transforms/Scalar/ConstantHoisting.h"
#include "llvm/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include "llvm/Transforms/Scalar/DeadStoreElimination.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/Float2Int.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/GuardWidening.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/JumpThreading.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/LoopDataPrefetch.h"
#include "llvm/Transforms/Scalar/LoopDeletion.h"
#include "llvm/Transforms/Scalar/LoopDistribute.h"
#include "llvm/Transforms/Scalar/LoopIdiomRecognize.h"
#include "llvm/Transforms/Scalar/LoopInstSimplify.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/Transforms/Scalar/LoopSimplifyCFG.h"
#include "llvm/Transforms/Scalar/LoopStrengthReduce.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/LowerAtomic.h"
#include "llvm/Transforms/Scalar/LowerExpectIntrinsic.h"
#include "llvm/Transforms/Scalar/LowerGuardIntrinsic.h"
#include "llvm/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvm/Transforms/Scalar/MergedLoadStoreMotion.h"
#include "llvm/Transforms/Scalar/NaryReassociate.h"
#include "llvm/Transforms/Scalar/NewGVN.h"
#include "llvm/Transforms/Scalar/PartiallyInlineLibCalls.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/Sink.h"
#include "llvm/Transforms/Scalar/SpeculativeExecution.h"
#include "llvm/Transforms/Scalar/TailRecursionElimination.h"
#include "llvm/Transforms/Utils/AddDiscriminators.h"
#include "llvm/Transforms/Utils/BreakCriticalEdges.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Utils/LibCallsShrinkWrap.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LowerInvoke.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/Utils/MemorySSA.h"
#include "llvm/Transforms/Utils/NameAnonGlobals.h"
#include "llvm/Transforms/Utils/SimplifyInstructions.h"
#include "llvm/Transforms/Utils/SymbolRewriter.h"
#include "llvm/Transforms/Vectorize/LoopVectorize.h"
#include "llvm/Transforms/Vectorize/SLPVectorizer.h"

using namespace llvm::wazuhl;

namespace {
  // FIXME: this is a hack specifically made for TargetIRAnalysis
  constexpr llvm::TargetMachine *TM = nullptr;
  const std::string uselessPrefixes[] = {"print", "pgo", "dot"};

  inline bool isActionUsefull(const Action& a) {
    auto &ActionName = a.getName();
    return llvm::none_of(uselessPrefixes, [&ActionName](const std::string &x) {
        return ActionName.startswith(x);
      });
  }
}

namespace llvm {
namespace wazuhl {
  ActionList Action::getAllPossibleActions() {
    ActionList EverySinglePossiblePass {
#define ANALYSIS_TO_PASS(CTR, IR_TYPE)                                         \
      RequireAnalysisPass                                                      \
          <std::remove_reference<decltype(CTR)>::type, IR_TYPE>()
#define MODULE_PASS_OR_ANALYSIS(NAME, CTR)                                     \
      { NAME,                                                                  \
        [] {                                                                   \
          using PassT = decltype(CTR);                                         \
          using WrapperType = detail::PassModel<Module, PassT,                 \
                                                PreservedAnalyses,             \
                                                AnalysisManager<Module>>;      \
          return new WrapperType(CTR);                                         \
        }                                                                      \
      },
#define MODULE_PASS(NAME, CREATE_PASS)                                         \
      MODULE_PASS_OR_ANALYSIS(NAME, CREATE_PASS)
#define MODULE_ANALYSIS(NAME, CREATE_PASS)                                     \
      MODULE_PASS_OR_ANALYSIS(NAME, ANALYSIS_TO_PASS(CREATE_PASS, Module))
#define CGSCC_PASS_OR_ANALYSIS(NAME, CTR)                                      \
      MODULE_PASS_OR_ANALYSIS(NAME, createModuleToPostOrderCGSCCPassAdaptor(CTR))
#define CGSCC_PASS(NAME, CREATE_PASS)                                          \
      CGSCC_PASS_OR_ANALYSIS(NAME, CREATE_PASS)
//FIXME: Couldn't use the only CG level analysis
//#define CGSCC_ANALYSIS(NAME, CREATE_PASS)                                      \
//      CGSCC_PASS_OR_ANALYSIS(NAME, ANALYSIS_TO_PASS(CREATE_PASS, LazyCallGraph::SCC))
#define FUNCTION_PASS_OR_ANALYSIS(NAME, CTR)                                   \
      MODULE_PASS_OR_ANALYSIS(NAME, createModuleToFunctionPassAdaptor(CTR))
#define FUNCTION_PASS(NAME, CREATE_PASS)                                       \
      FUNCTION_PASS_OR_ANALYSIS(NAME, CREATE_PASS)
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS)                                   \
      FUNCTION_PASS_OR_ANALYSIS(NAME, ANALYSIS_TO_PASS(CREATE_PASS, Function))
#define LOOP_PASS_OR_ANALYSIS(NAME, CTR)                                       \
      FUNCTION_PASS_OR_ANALYSIS(NAME, createFunctionToLoopPassAdaptor(CTR))
#define LOOP_PASS(NAME, CREATE_PASS)                                           \
      LOOP_PASS_OR_ANALYSIS(NAME, CREATE_PASS)
#define LOOP_ANALYSIS(NAME, CREATE_PASS)                                       \
      LOOP_PASS_OR_ANALYSIS(NAME, ANALYSIS_TO_PASS(CREATE_PASS, Loop))

#include "PassRegistry.def"
#undef MODULE_PASS_OR_ANALYSIS
#undef CGSCC_PASS_OR_ANALYSIS
#undef FUNCTION_PASS_OR_ANALYSIS
#undef LOOP_PASS_OR_ANALYSIS
      {"terminal", [] { return nullptr; }} /// this is a terminal action
    };
    auto FilteredListOfActions =
      make_filter_range(EverySinglePossiblePass,
                        [] (const Action &a) {
                          return isActionUsefull(a);
                        });
    return {FilteredListOfActions.begin(), FilteredListOfActions.end()};
  }
}
}
