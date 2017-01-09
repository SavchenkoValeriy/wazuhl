#ifndef LLVM_WAZUHL_REINFORCEMENTLEARNING_H
#define LLVM_WAZUHL_REINFORCEMENTLEARNING_H

namespace llvm {
namespace wazuhl {
  class A {};
  class State {};

  template <class SimulatorT, class QType, class PolicyT>
  class QLearning {
  public:
    QLearning(SimulatorT &simulator, QType &Q, const PolicyT &policy,
              double alpha, double gamma) :
      simulator(simulator), Q(Q), policy(policy), alpha(alpha), gamma(gamma) {}

    void learn() {
      State S = simulator.getState();
      while(simulator.isInTerminalState()) {
        A action = policy.pick(S);
        simulator.takeAction(action);
        State newS = simulator.getState();
        auto R = simulator.getReward();
        auto oldQValue = Q(S, action);
        auto newQValue = oldQValue + alpha * (R + gamma * max(Q(newS)) - oldQValue);
        Q.update(S, action, newQValue);
        S = newS;
      }
    }
  private:
    SimulatorT &simulator;
    QType &Q;
    const PolicyT &policy;
    double alpha, gamma;
  };

  namespace policies {
    template <class Function>
    class Greedy {
    public:
      Greedy(const Function &valueFuncion) :
        value(valueFuncion) {}
      A pick(const State &s) const {
        return argmax(value(s));
      }
    private:
      const Function &value;
    };

    template <class Function>
    class EpsilonGreedy : private Greedy<Function> {
    public:
      EpsilonGreedy(double epsilon, const Function & valueFuncion) :
        epsilon(epsilon), Greedy<Function>(valueFuncion) {}
      A pick(const State &s) {
        return Greedy<Function>::pick(s);
      }
    private:
      double epsilon;
    };
  }
}
}

#endif /* LLVM_WAZUHL_REINFORCEMENTLEARNING_H */
