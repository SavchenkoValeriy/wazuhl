#include "llvm/Wazuhl/PolicyEvaluator.h"
#include "llvm/Wazuhl/DQN.h"
#include "llvm/Wazuhl/Environment.h"
#include "llvm/Wazuhl/ExperienceReplay.h"
#include "llvm/Wazuhl/ReinforcementLearning.h"

namespace llvm {
namespace wazuhl {
namespace {
std::vector<PassAction> getO2Actions() {
  constexpr auto names = {"forceattrs",
                          "inferattrs",
                          "simplify-cfg",
                          "sroa",
                          "early-cse",
                          "lower-expect",
                          "gvn-hoist",
                          "ipsccp",
                          "globalopt",
                          "mem2reg",
                          "deadargelim",
                          "instcombine",
                          "simplify-cfg",
                          "globals-aa",
                          "inline",
                          "function-attrs",
                          "sroa",
                          "early-cse",
                          "speculative-execution",
                          "jump-threading",
                          "correlated-propagation",
                          "simplify-cfg",
                          "instcombine",
                          "libcalls-shrinkwrap",
                          "tailcallelim",
                          "simplify-cfg",
                          "reassociate",
                          "opt-remark-emit",
                          "rotate",
                          "licm",
                          "simplify-cfg",
                          "instcombine",
                          "indvars",
                          "loop-idiom",
                          "loop-deletion",
                          "unroll-full",
                          "mldst-motion",
                          "gvn",
                          "memcpyopt",
                          "ipsccp",
                          "bdce",
                          "instcombine",
                          "jump-threading",
                          "correlated-propagation",
                          "dse",
                          "licm",
                          "adce",
                          "simplify-cfg",
                          "instcombine",
                          "elim-avail-extern",
                          "rpo-functionattrs",
                          "globals-aa",
                          "float2int",
                          "rotate",
                          "loop-distribute",
                          "loop-vectorize",
                          "loop-load-elim",
                          "instcombine",
                          "slp-vectorizer",
                          "simplify-cfg",
                          "instcombine",
                          "unroll",
                          "instcombine",
                          "opt-remark-emit",
                          "licm",
                          "alignment-from-assumptions",
                          "loop-sink",
                          "instsimplify",
                          "globaldce",
                          "constmerge",
                          "terminal"};
  std::vector<PassAction> actions;
  actions.reserve(names.size());
  for (const auto &name : names) {
    actions.push_back(PassAction::getActionByName(name));
  }
  return actions;
}

} // namespace

void PolicyEvaluator::evaluate() {
  DQN Q;
  rl::policies::Greedy<DQN> policy{Q};
  auto learner = rl::createLearner<rl::NonLearning>(OptimizationEnv, Q, policy);
  learner.learn();
}

void LearningPolicyEvaluator::evaluate() {
  DQN Q;
  ExperienceReplay Memory;
  rl::policies::EpsilonGreedy<DQN> policy{0.9, Q};
  auto learner = rl::createDeepLearner<rl::DeepDoubleQLearning>(
      OptimizationEnv, Q, policy, Memory, 0.99, (unsigned)10);
  learner.learn();
}
} // namespace wazuhl
} // namespace llvm
