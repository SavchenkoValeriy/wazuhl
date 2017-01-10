#include "llvm/Wazuhl/ReinforcementLearning.h"
#include "gtest/gtest.h"
#include <vector>

using namespace llvm;
using namespace wazuhl;

constexpr int NumberOfStates = 12;

class TestProblem {
public:
  class State {
  public:
    State (int x) : position(x) {}
    State (const State &) = default;
    State (State &&) = default;
    State &operator=(const State &) = default;
    int position = 0;
  };

  class Action {
  public:
    enum Direction { Left, Right };
    Direction direction;

    using AllDirections = Direction[2];
    static const AllDirections &getAllActions() {
      return all;
    }
  private:
    static AllDirections all;
  };

  class Environment {
  public:
    void reset() {
      current = def;
    }
    bool isInTerminalState() {
      return current.position == final.position ||
        current.position < 0;
    }
    void takeAction(const Action &A) {
      if (A.direction == Action::Left) {
        --current.position;
      } else {
        ++current.position;
      }
    }
    State getState() {
      return current;
    }
    int getReward() {
      if (current.position < 0)
        return -10;
      if (current.position == final.position)
        return 50;
      return 0;
    }
  private:
    State def = {3};
    State current = def;
    State final = {NumberOfStates - 2};
  };
};

class Q {
private:

public:
  using State  = TestProblem::State;
  using Action = TestProblem::Action;
  using Result = double;

  class CurriedQ {
  public:
    using State  = Q::State;
    using Action = Q::Action;
    using Result = Q::Result;

    CurriedQ(Result (&values)[2]) : values(values) {}
    Result &operator() (const Action &A) {
      return values[index(A)];
    }
  private:
    int index(const Action &A) {
      return A.direction == Action::Left ? 1 : 0;
    }

    Result (&values)[2];

    friend Action rl::argmax<CurriedQ>(const CurriedQ &);
  };

  CurriedQ operator() (const State &S) {
    return {values[index(S)]};
  }
  Result &operator() (const State &S, const Action &A) {
    return (*this)(S)(A);
  }
  void update(const State &S, const Action &A, Result value) {
    (*this)(S, A) = value;
  }
private:
  int index(const State &S) {
    return S.position + 1;
  }

  Result values[NumberOfStates][2] = { {0} };
};

template <>
Q::CurriedQ::Action rl::argmax(const Q::CurriedQ &F) {
  return {F.values[0] > F.values[1] ? Q::Action::Right : Q::Action::Left};
}

template <>
Q::CurriedQ::Result rl::max(const Q::CurriedQ &F) {
  return std::max(F.values);
}

TestProblem::Action::AllDirections
TestProblem::Action::all = { TestProblem::Action::Left,
                             TestProblem::Action::Right };

TEST(ReinforcementLearning, QLearning) {
  TestProblem::Environment env;
  Q function;
  rl::policies::EpsilonGreedy<Q> policy{0.1, function};
  rl::QLearning<TestProblem, TestProblem::Environment, Q, decltype(policy)> learner{env, function, policy, 0.8, 0.99};
  for (int i = 0; i < 1000; ++i) {
    learner.learn();
    env.reset();
  }
}
