#ifndef LLVM_WAZUHL_Q_H
#define LLVM_WAZUHL_Q_H

#include "llvm/ADT/Sequence.h"
#include <algorithm>
#include <utility>

namespace llvm {
namespace wazuhl {
namespace rl {

template <class QCore> class Q {
public:
  using State = typename QCore::State;
  using Action = typename QCore::Action;
  using Result = typename QCore::Result;
  template <class T> using Batch = typename QCore::template Batch<T>;
  using ResultsVector = typename QCore::ResultsVector;

  template <class ResultT, class StateT, class ActionT> class CurriedQ;

  template <class ResultT, class StateT, class ActionT> class QValue {
  public:
    operator ResultT() const {
      auto Values = Original->calculate(S);
      return getValues(Values, A);
    }

    void operator=(ResultT value) { Original->update(S, A, value); }

  private:
    QValue(QCore *original, const StateT &s, const ActionT &a)
        : Original(original), S(s), A(a) {}

    QValue(const QCore *original, const StateT &s, const ActionT &a)
        : Original(original), S(s), A(a) {}

    mutable QCore *Original;
    const StateT &S;
    const ActionT &A;

    friend class CurriedQ<ResultT, StateT, ActionT>;
  };

  template <class ResultT, class StateT, class ActionT> class CurriedQ {
  public:
    QValue<ResultT, StateT, ActionT> operator()(const ActionT &A) {
      return {Original, S, A};
    }

    const QValue<ResultT, StateT, ActionT> operator()(const ActionT &A) const {
      return {Original, S, A};
    }

  private:
    CurriedQ(QCore *original, const State &s) : Original(original), S(s) {}

    CurriedQ(const QCore *original, const State &s)
        : Original(original), S(s) {}

    mutable QCore *Original;
    const State &S;

    friend class Q<QCore>;

  public:
    auto max() const -> decltype(this->Original->max(this->S)) {
      return Original->max(S);
    }

    auto argmax() const -> decltype(this->Original->argmax(this->S)) {
      return Original->argmax(S);
    }
  };

  template <template <class...> class T>
  using OneValue = T<Result, State, Action>;

  template <template <class...> class T>
  using BatchValue = T<ResultsVector, Batch<State>, Batch<Action>>;

  OneValue<CurriedQ> operator()(const State &S) { return {&Original, S}; }

  BatchValue<CurriedQ> operator()(const Batch<State> &S) {
    return {&Original, S};
  }

  const OneValue<CurriedQ> operator()(const State &S) const {
    return {&Original, S};
  }

  const BatchValue<CurriedQ> operator()(const Batch<State> &S) const {
    return {&Original, S};
  }

  OneValue<QValue> operator()(const State &S, const Action &A) {
    return (*this)(S)(A);
  }

  BatchValue<QValue> operator()(const Batch<State> &S, const Batch<Action> &A) {
    return (*this)(S)(A);
  }

  const OneValue<QValue> operator()(const State &S, const Action &A) const {
    return (*this)(S)(A);
  }

  const BatchValue<QValue> operator()(const Batch<State> &S,
                                      const Batch<Action> &A) const {
    return (*this)(S)(A);
  }

  Q() = default;
  Q(const Q<QCore> &) = delete;
  Q(Q<QCore> &&) = default;

  Q<QCore> &operator=(const Q<QCore> &) = delete;
  Q<QCore> &operator=(Q<QCore> &&) = default;

private:
  static Result getValues(const ResultsVector &V, const Action &A) {
    return V[A.getIndex()];
  }

  static Batch<Result> getValues(const Batch<ResultsVector> &V,
                                 const Batch<Action> &A) {
    Batch<Result> Values;
    for (auto i : seq<unsigned>(V.size())) {
      Values.push_back(V[i][A[i].getIndex()]);
    }
    return Values;
  }

  mutable QCore Original;
};

template <class QCore> using QS = typename Q<QCore>::CurriedQ;

template <class Q>
auto argmax(const Q &Function) -> decltype(Function.argmax()) {
  return Function.argmax();
}
template <class Q> auto max(const Q &Function) -> decltype(Function.max()) {
  return Function.max();
}
} // namespace rl
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_Q_H */
