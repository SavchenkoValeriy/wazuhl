#ifndef LLVM_WAZUHL_REINFORCEMENTLEARNING_H
#define LLVM_WAZUHL_REINFORCEMENTLEARNING_H

#include "llvm/Wazuhl/Random.h"

namespace llvm {
namespace wazuhl {
  template <class Function>
  typename Function::Action argmax(const Function &);
  template <class Function>
  typename Function::Result max(const Function&);

  template <class ProblemT, class EnvironmentT, class QType, class PolicyT>
  class QLearning {
  public:
    using Action = typename ProblemT::Action;
    using State  = typename ProblemT::State;
    QLearning(EnvironmentT &Environment, QType &Q, const PolicyT &Policy,
              double alpha, double gamma) :
      Environment(Environment), Q(Q), Policy(Policy), alpha(alpha), gamma(gamma) {}

    void learn() {
      State S = Environment.getState();
      while(Environment.isInTerminalState()) {
        Action A = Policy.pick(S);
        Environment.takeAction(A);
        State newS = Environment.getState();
        auto R = Environment.getReward();
        auto oldQValue = Q(S, A);
        auto newQValue = oldQValue + alpha * (R + gamma * max(Q(newS)) - oldQValue);
        Q.update(S, A, newQValue);
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
      using Action = typename Function::Action;
      using State  = typename Function::State;

      Greedy(const Function &valueFuncion) :
        value(valueFuncion) {}
      Action pick(const State &s) const {
        return argmax(value(s));
      }
    private:
      const Function &value;
    };

    template <class Function>
    class Random {
    public:
      using Action = typename Function::Action;
      using State  = typename Function::State;

      Action pick(const State &s) {
        // TODO implement a random picking strategy
        return {};
      }
    };

    template <class Function>
    class EpsilonGreedy : private Greedy<Function>, private Random<Function> {
    public:
      using Action = typename Function::Action;
      using State  = typename Function::State;

      EpsilonGreedy(double epsilon, const Function & valueFuncion) :
        epsilon(epsilon), Greedy<Function>(valueFuncion), Random<Function>() {}
      Action pick(const State &s) {
        if (random::flipACoin(epsilon))
          return Random<Function>::pick(s);
        return Greedy<Function>::pick(s);
      }
    private:
      double epsilon;
    };
  }
}
}

#endif /* LLVM_WAZUHL_REINFORCEMENTLEARNING_H */
