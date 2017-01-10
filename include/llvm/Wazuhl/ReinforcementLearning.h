#ifndef LLVM_WAZUHL_REINFORCEMENTLEARNING_H
#define LLVM_WAZUHL_REINFORCEMENTLEARNING_H

#include "llvm/Wazuhl/Random.h"

namespace llvm {
namespace wazuhl {
  class A {};
  class State {};

  template <class Function>
  A argmax(const Function &);
  template <class Function>
  typename Function::ResultT max(const Function&);

  template <class EnvironmentT, class QType, class PolicyT>
  class QLearning {
  public:
    QLearning(EnvironmentT &Environment, QType &Q, const PolicyT &Policy,
              double alpha, double gamma) :
      Environment(Environment), Q(Q), Policy(Policy), alpha(alpha), gamma(gamma) {}

    void learn() {
      State S = Environment.getState();
      while(Environment.isInTerminalState()) {
        A action = Policy.pick(S);
        Environment.takeAction(action);
        State newS = Environment.getState();
        auto R = Environment.getReward();
        auto oldQValue = Q(S, action);
        auto newQValue = oldQValue + alpha * (R + gamma * max(Q(newS)) - oldQValue);
        Q.update(S, action, newQValue);
        S = newS;
      }
    }
  private:
    EnvironmentT &Environment;
    QType &Q;
    const PolicyT &Policy;
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

    class Random {
    public:
      A pick(const State &s) {
        // TODO implement a random picking strategy
        return {};
      }
    };
    template <class Function>
    class EpsilonGreedy : private Greedy<Function>, private Random {
    public:
      EpsilonGreedy(double epsilon, const Function & valueFuncion) :
        epsilon(epsilon), Greedy<Function>(valueFuncion), Random() {}
      A pick(const State &s) {
        if (random::flipACoin(epsilon))
          return Random::pick(s);
        return Greedy<Function>::pick(s);
      }
    private:
      double epsilon;
    };
  }
}
}

#endif /* LLVM_WAZUHL_REINFORCEMENTLEARNING_H */
