#ifndef LLVM_WAZUHL_DQN_H
#define LLVM_WAZUHL_DQN_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Wazuhl/PassAction.h"
#include "llvm/Wazuhl/Q.h"
#include "llvm/Wazuhl/StateFeatures.h"
#include <memory>
#include <vector>

namespace llvm {
namespace wazuhl {

class DQNCoreImpl;

class DQNCore {
public:
  using State = StateFeatures;
  using Action = PassAction;
  using Result = double;
  template <class T> using Vector = std::vector<T>;
  using ResultsVector = Vector<Result>;
  template <class T> using Batch = std::vector<T>;

  ResultsVector calculate(const State &S) const;
  Result max(const State &S) const;
  Action argmax(const State &S) const;

  Batch<ResultsVector> calculate(const Batch<State> &S) const;
  Batch<Result> max(const Batch<State> &S) const;
  Batch<Action> argmax(const Batch<State> &S) const;

  void update(const State &S, const Action &A, Result value);
  void update(const Batch<State> &S, const Batch<Action> &A,
              const Batch<Result> &value);

  DQNCore();
  ~DQNCore();

private:
  std::unique_ptr<DQNCoreImpl> pImpl;
};

using DQN = rl::Q<DQNCore>;
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_DQN_H */
