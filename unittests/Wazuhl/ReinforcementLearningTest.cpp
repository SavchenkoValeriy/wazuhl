#include "llvm/Wazuhl/Q.h"
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
    static std::vector<State> getAllStates() {
      std::vector<State> Result;
      for (int i = -1; i < NumberOfStates - 1; ++i) {
        Result.emplace_back(i);
      }
      return Result;
    }
  };

  class Action {
  public:
    enum Direction { Left, Right };
    Direction direction;

    unsigned getIndex() const {
      return direction;
    }

    using AllDirections = Action[2];
    static const AllDirections &getAllActions() {
      return all;
    }

    static Action getActionByIndex(unsigned Index) {
      return all[Index];
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

class QCore {
public:
  using State         = TestProblem::State;
  using Action        = TestProblem::Action;
  using Result        = double;
  using ResultsVector = Result (&)[2];

  ResultsVector calculate(const State &S) const {
    return values[index(S)];
  }

  void update(const State &S, const Action &A, Result value) {
    values[index(S)][index(A)] = value;
  }

private:
  int index(const State &S) const {
    return S.position + 1;
  }
  int index(const Action &A) const {
    return A.getIndex();
  }

  mutable Result values[NumberOfStates][2] = { {0} };
  friend raw_ostream &operator<< (raw_ostream &, const QCore &);
  FRIEND_TEST(ReinforcementLearning, QLearning);
};

using Q = rl::Q<QCore>;

raw_ostream &operator<< (raw_ostream &out, const QCore &function) {
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

TestProblem::Action::AllDirections
TestProblem::Action::all = { { TestProblem::Action::Left },
                             { TestProblem::Action::Right } };

TEST(ReinforcementLearning, QLearning) {
  TestProblem::Environment env;
  using State = TestProblem::State;
  using Action = TestProblem::Action;
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
  const Action Right{Action::Right}, Left{Action::Left};
  for (auto S : State::getAllStates()) {
    Q::Result RightValue = function(S, Right),
      LeftValue = function(S, Left);
    if (RightValue == 0) {
      EXPECT_EQ(RightValue, LeftValue);
    } else {
      EXPECT_GT(RightValue, LeftValue);
    }
  }
}

class QMock {
public:
  using Result = double;
  using Action = TestProblem::Action;
  using State = TestProblem::State;
  const QMock &operator() (const State &) const {
    return *this;
  }
};

template <>
QMock::Action rl::argmax(const QMock &F) {
  return {QMock::Action::Right};
}

template <>
QMock::Result rl::max(const QMock &F) {
  return 10.0;
}

constexpr int NumberOfIterations = 5000;

using PicksT = SmallVector<int, 2>;
template <class Policy>
PicksT getPicksForPolicy(const Policy &policy) {
  PicksT result{ 0, 0 };
  TestProblem::State S{3};
  for (int i = 0; i < NumberOfIterations; ++i) {
    ++result[policy.pick(S).direction];
  }
  return result;
}

class RandomEq {
public:
  RandomEq(double precision) : epsilon(precision) {}
  bool operator() (double result, double expected) {
    return (result < expected + epsilon * expected) &&
           (result > expected - epsilon * expected);
  }
private:
  double epsilon;
};

TEST(ReinforcementLearning, Greedy) {
  QMock function;
  rl::policies::Greedy<QMock> policy{function};
  auto picks = getPicksForPolicy(policy);
  EXPECT_EQ(picks[QMock::Action::Right], NumberOfIterations);
  EXPECT_EQ(picks[QMock::Action::Left], 0);
}

TEST(ReinforcementLearning, Random) {
  rl::policies::Random<QMock> policy;
  auto picks = getPicksForPolicy(policy);
  EXPECT_PRED2(RandomEq{0.05}, picks[QMock::Action::Right], NumberOfIterations / 2);
  EXPECT_PRED2(RandomEq{0.05}, picks[QMock::Action::Left], NumberOfIterations / 2);
}

TEST(ReinforcementLearning, EpsilonGreedy) {
  QMock function;
  double epsilon = 0.6;
  rl::policies::EpsilonGreedy<QMock> policy{epsilon, function};
  auto picks = getPicksForPolicy(policy);
  EXPECT_PRED2(RandomEq{0.05}, picks[QMock::Action::Right],
               (1 - epsilon / 2) * NumberOfIterations);
  EXPECT_PRED2(RandomEq{0.05}, picks[QMock::Action::Left],
               (epsilon / 2) * NumberOfIterations);
}
