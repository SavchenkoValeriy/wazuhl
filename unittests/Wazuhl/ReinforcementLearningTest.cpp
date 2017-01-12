#include "llvm/Wazuhl/ReinforcementLearning.h"
#include "llvm/Support/raw_ostream.h"
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

    using AllDirections = Action[2];
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
    Result operator() (const Action &A) const {
      return values[index(A)];
    }
  private:
    int index(const Action &A) const {
      return A.direction == Action::Left ? 1 : 0;
    }

    Result (&values)[2];

    friend Action rl::argmax<CurriedQ>(const CurriedQ &&);
    friend Result rl::max<CurriedQ>(const CurriedQ &&);
  };

  CurriedQ operator() (const State &S) {
    return {values[index(S)]};
  }
  const CurriedQ operator() (const State &S) const {
    return {values[index(S)]};
  }
  Result &operator() (const State &S, const Action &A) {
    return (*this)(S)(A);
  }
  Result operator() (const State &S, const Action &A) const {
    return (*this)(S)(A);
  }
  void update(const State &S, const Action &A, Result value) {
    (*this)(S, A) = value;
  }

private:
  int index(const State &S) const {
    return S.position + 1;
  }

  mutable Result values[NumberOfStates][2] = { {0} };
  friend raw_ostream &operator<< (raw_ostream &, const Q &);
  FRIEND_TEST(ReinforcementLearning, QLearning);
};

raw_ostream &operator<< (raw_ostream &out, const Q &function) {
  auto printAction = [&](const StringRef name, int offset) {
    out << name << ": ";
    for (int i = 0; i < NumberOfStates - 1; ++i) {
      out << function.values[i][offset];
      out << ", ";
    }
    out << function.values[NumberOfStates - 1][offset] << "\n";
  };
  printAction("Right", 0);
  printAction("Left", 1);
  return out;
}

template <>
Q::CurriedQ::Action rl::argmax(const Q::CurriedQ &&F) {
  return {F.values[0] > F.values[1] ? Q::Action::Right : Q::Action::Left};
}

template <>
Q::CurriedQ::Result rl::max(const Q::CurriedQ &&F) {
  return *std::max_element(std::begin(F.values), std::end(F.values));
}

TestProblem::Action::AllDirections
TestProblem::Action::all = { { TestProblem::Action::Left },
                             { TestProblem::Action::Right } };

TEST(ReinforcementLearning, QLearning) {
  TestProblem::Environment env;
  Q function;
  rl::policies::EpsilonGreedy<Q> policy{0.7, function};
  rl::QLearning<TestProblem, TestProblem::Environment, Q, decltype(policy)>
    learner{env, function, policy, 1, 0.99};
  double TotalReward = 0;
  for (int i = 0; i < 1000; ++i) {
    learner.learn();
    TotalReward += env.getReward();
    env.reset();
  }
  constexpr int right = 0, left = 1;
  for (auto Q_s : function.values) {
    if (Q_s[right] == 0) {
      EXPECT_EQ(Q_s[right], Q_s[left]);
    } else {
      EXPECT_GT(Q_s[right], Q_s[left]);
    }
  }
}
