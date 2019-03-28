#ifndef LLVM_WAZUHL_REINFORCEMENTLEARNING_H
#define LLVM_WAZUHL_REINFORCEMENTLEARNING_H

#include "llvm/Wazuhl/Random.h"

namespace llvm {
namespace wazuhl {
namespace rl {
template <class EnvironmentT, class QType, class PolicyT> class QLearning {
public:
  using Action = typename EnvironmentT::Action;
  using State = typename EnvironmentT::State;
  QLearning(EnvironmentT &Environment, QType &Q, const PolicyT &Policy,
            double alpha, double gamma)
      : Environment(Environment), Q(Q), Policy(Policy), alpha(alpha),
        gamma(gamma) {}

  void learn() {
    State S = Environment.getState();
    while (!Environment.isInTerminalState()) {
      Action A = Policy.pick(S);
      Environment.takeAction(A);
      State newS = Environment.getState();
      auto R = Environment.getReward();
      Q(S, A) = Q(S, A) + alpha * (R + gamma * max(Q(newS)) - Q(S, A));
      S = newS;
    }
  }

private:
  EnvironmentT &Environment;
  QType &Q;
  const PolicyT &Policy;
  double alpha, gamma;
};

template <class EnvironmentT, class QType, class PolicyT> class NonLearning {
public:
  using Action = typename EnvironmentT::Action;
  using State = typename EnvironmentT::State;
  NonLearning(EnvironmentT &Environment, QType &Q, const PolicyT &Policy)
      : Environment(Environment), Q(Q), Policy(Policy) {}

  void learn() {
    State S;
    do {
      S = Environment.getState();
      Action A = Policy.pick(S);
      Environment.takeAction(A);
    } while (!Environment.isInTerminalState());
  }

private:
  EnvironmentT &Environment;
  QType &Q;
  const PolicyT &Policy;
};

template <template <class...> class Learner, class EnvironmentT,
          class FunctionT, class PolicyT, class... Args>
Learner<EnvironmentT, FunctionT, PolicyT>
createLearner(EnvironmentT &Environment, FunctionT &ValueFunction,
              const PolicyT &Policy, Args &&... args) {
  return Learner<EnvironmentT, FunctionT, PolicyT>{
      Environment, ValueFunction, Policy, std::forward<Args>(args)...};
}

template <template <class...> class Learner, class EnvironmentT,
          class FunctionT, class PolicyT, class MemoryT, class... Args>
Learner<EnvironmentT, FunctionT, PolicyT, MemoryT>
createDeepLearner(EnvironmentT &Environment, FunctionT &ValueFunction,
                  FunctionT &TargetValueFunction, const PolicyT &Policy,
                  MemoryT &Memory, Args &&... args) {
  return Learner<EnvironmentT, FunctionT, PolicyT, MemoryT>{
      Environment, ValueFunction, TargetValueFunction,
      Policy,      Memory,        std::forward<Args>(args)...};
}

template <class EnvironmentT, class QType, class PolicyT, class MemoryT>
class DeepQLearning {
public:
  using Action = typename EnvironmentT::Action;
  using State = typename EnvironmentT::State;
  DeepQLearning(EnvironmentT &Environment, QType &Q, QType &T,
                const PolicyT &Policy, MemoryT &Memory, double gamma,
                unsigned C)
      : Environment(Environment), Q(Q), Target(T), Policy(Policy),
        Memory(Memory), gamma(gamma), C(C) {}

  void learn() {
    State S = Environment.getState();
    unsigned UpdateCounter = config::getOverallNumberOfSteps();

    while (!Environment.isInTerminalState()) {
      Action A = Policy.pick(S);
      llvm::errs() << "Wazuhl prefers '" << argmax(Q(S)).getName()
                   << "' with value " << max(Q(S)) << "\n";

      Environment.takeAction(A);
      State newS = Environment.getState();
      auto R = Environment.getReward();

      Memory.push(S, A, R, newS);

      S = newS;

      if (Memory.isBigEnoughForReplay()) {
        auto Experience = Memory.sample();

        auto &Ss = Experience.S;
        auto &As = Experience.A;
        auto &Rs = Experience.R;
        auto &newSs = Experience.newS;

        auto Ys = Rs + gamma * max(Target(newSs));

        for (auto i : seq<unsigned>(0, Experience.size())) {
          if (Experience.isTerminal[i]) {
            llvm::errs() << "Wazuhl has a terminal state in a batch! (" << Rs[i]
                         << ")\n";
            Ys[i] = Rs[i];
          }
        }
        Q(Ss, As) = Ys;
      }

      if (UpdateCounter++ % C == 0) {
        Target = Q;
      }
    }

    config::saveOverallNumberOfSteps(UpdateCounter);
  }

private:
  EnvironmentT &Environment;
  QType &Q;
  QType Target;
  const PolicyT &Policy;
  MemoryT &Memory;
  double gamma;
  const unsigned C;
};

template <class EnvironmentT, class QType, class PolicyT, class MemoryT>
class DeepDoubleQLearning {
public:
  using Action = typename EnvironmentT::Action;
  using State = typename EnvironmentT::State;
  DeepDoubleQLearning(EnvironmentT &Environment, QType &Q, QType &T,
                      const PolicyT &Policy, MemoryT &Memory, double gamma,
                      unsigned C)
      : Environment(Environment), Q(Q), Target(T), Policy(Policy),
        Memory(Memory), gamma(gamma), C(C) {}

  void learn() {
    State S = Environment.getState();
    unsigned UpdateCounter = config::getOverallNumberOfSteps();
    llvm::errs() << "Step counter: " << UpdateCounter << "\n";

    while (!Environment.isInTerminalState()) {
      Action A = Policy.pick(S);
      llvm::errs() << "Wazuhl prefers '" << argmax(Q(S)).getName()
                   << "' with value " << max(Q(S)) << "\n";

      Environment.takeAction(A);
      State newS = Environment.getState();
      auto R = Environment.getReward();

      Memory.push(S, A, R, newS);

      S = newS;

      if (Memory.isBigEnoughForReplay()) {
        auto Experience = Memory.sample();

        auto &Ss = Experience.S;
        auto &As = Experience.A;
        auto &Rs = Experience.R;
        auto &newSs = Experience.newS;

        auto Ys = Rs + gamma * Target(newSs, argmax(Q(newSs)));

        for (auto i : seq<unsigned>(0, Experience.size())) {
          if (Experience.isTerminal[i]) {
            llvm::errs() << "Wazuhl has a terminal state in a batch! (" << Rs[i]
                         << ")\n";
            Ys[i] = Rs[i];
          }
        }
        Q(Ss, As) = Ys;
      }

      if (UpdateCounter++ % C == 0) {
        llvm::errs() << "Time for Wazuhl to update the target network\n";
        Target = Q;
      }
    }

    config::saveOverallNumberOfSteps(UpdateCounter);
  }

private:
  EnvironmentT &Environment;
  QType &Q;
  QType &Target;
  const PolicyT &Policy;
  MemoryT &Memory;
  double gamma;
  const unsigned C;
};

namespace policies {
template <class Function> class Greedy {
public:
  using Action = typename Function::Action;
  using State = typename Function::State;

  Greedy(const Function &valueFuncion) : value(valueFuncion) {}
  Action pick(const State &s) const { return argmax(value(s)); }

private:
  const Function &value;
};

template <class Function> class Random {
public:
  using Action = typename Function::Action;
  using State = typename Function::State;

  Action pick(const State &s) const {
    static const auto &allActions = Action::getAllPossibleActions();
    return random::pickOutOf(allActions);
  }
};

template <class Function> class Repeating {
public:
  using Action = typename Function::Action;
  using State = typename Function::State;

  Repeating(const Function &valueFunction,
            const std::vector<Action> &&plannedActions)
      : value(valueFunction), actions(std::move(plannedActions)),
        nextActionIndex(0) {}
  Action pick(const State &s) const { return actions[nextActionIndex++]; }

private:
  const Function &value;
  const std::vector<Action> actions;
  mutable unsigned nextActionIndex;
};

template <class Function>
class EpsilonGreedy : private Greedy<Function>, private Random<Function> {
public:
  using Action = typename Function::Action;
  using State = typename Function::State;

  EpsilonGreedy(double epsilon, const Function &valueFuncion)
      : Greedy<Function>(valueFuncion), Random<Function>(), epsilon(epsilon) {}
  Action pick(const State &s) const {
    if (random::flipACoin(epsilon))
      return Random<Function>::pick(s);
    return Greedy<Function>::pick(s);
  }

private:
  double epsilon;
};

} // namespace policies
} // namespace rl
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_REINFORCEMENTLEARNING_H */
