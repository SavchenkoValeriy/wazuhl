#include "llvm/Wazuhl/PolicyEvaluator.h"
#include "llvm/Wazuhl/DQN.h"
#include "llvm/Wazuhl/Environment.h"
#include "llvm/Wazuhl/ReinforcementLearning.h"

namespace llvm {
namespace wazuhl {
void PolicyEvaluator::evaluate() {
  DQN Q;
  rl::policies::Greedy<DQN> policy{Q};
  auto learner = rl::createLearner<rl::NonLearning>(OptimizationEnv, Q, policy);
  learner.learn();
}

void LearningPolicyEvaluator::evaluate() {
  DQN Q;
  rl::policies::EpsilonGreedy<DQN> policy{0.9, Q};
  auto learner =
      rl::createLearner<rl::QLearning>(OptimizationEnv, Q, policy, 1.0, 0.99);
  learner.learn();
}
} // namespace wazuhl
} // namespace llvm
