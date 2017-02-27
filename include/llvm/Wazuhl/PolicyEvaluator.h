#ifndef LLVM_WAZUHL_POLICYEVALUATOR_H
#define LLVM_WAZUHL_POLICYEVALUATOR_H

namespace llvm {
namespace wazuhl {
  class Environment;
  class PolicyEvaluator {
  public:
    PolicyEvaluator(Environment &env) : OptimizationEnv(env) {}
    void evaluate();
  private:
    Environment &OptimizationEnv;
  };

  class LearningPolicyEvaluator {
  public:
    LearningPolicyEvaluator(Environment &env) : OptimizationEnv(env) {}
    void evaluate();
  private:
    Environment &OptimizationEnv;
  };
}
}

#endif /* LLVM_WAZUHL_POLICYEVALUATOR_H */
