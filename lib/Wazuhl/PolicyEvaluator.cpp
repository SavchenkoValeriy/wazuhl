#include "llvm/Wazuhl/PolicyEvaluator.h"
#include "llvm/Wazuhl/Environment.h"
#include "llvm/Wazuhl/DQN.h"
#include "llvm/Wazuhl/ReinforcementLearning.h"

namespace llvm {
namespace wazuhl {
  void PolicyEvaluator::evaluate() {
    DQN Q;
    rl::policies::Greedy<DQN> policy{Q};
    rl::NonLearning<Environment, DQN, decltype(policy)>
      learner{OptimizationEnv, Q, policy};
    learner.learn();
  }

  void LearningPolicyEvaluator::evaluate() {
    DQN Q;
    rl::policies::EpsilonGreedy<DQN> policy{0.7, Q};
    rl::QLearning<Environment, DQN, decltype(policy)>
      learner{OptimizationEnv, Q, policy, 0.1, 0.99};
    learner.learn();
  }
}
}
