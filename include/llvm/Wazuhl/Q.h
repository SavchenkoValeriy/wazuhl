#ifndef LLVM_WAZUHL_Q_H
#define LLVM_WAZUHL_Q_H

#include <algorithm>
#include <utility>

namespace llvm {
namespace wazuhl {
namespace rl {

  template <class QCore>
  class Q {
  public:
    using State           = typename QCore::State;
    using Action          = typename QCore::Action;
    using Result          = typename QCore::Result;
    using ResultsVector   = typename QCore::ResultsVector;
    using ActionValuePair = typename std::pair<Action, Result>;

    class CurriedQ;

    class QValue {
    public:
      using Result = Result;

      operator Result () {
        return Value;
      }

      void operator= (Result value) {
        Original->update(S, A, value);
      }
    private:
      QValue(QCore *original, const State &s, const Action &a, Result value) :
        Original(original), S(s), A(a), Value(value) {}

      QValue(const QCore *original, const State &s, const Action &a, Result value) :
        Original(original), S(s), A(a), Value(value) {}

      mutable QCore *Original;
      const State &S;
      const Action &A;
      Result Value;

      friend class CurriedQ;
    };

    class CurriedQ {
    public:
      using State  = State;
      using Action = Action;
      using Result = Result;

      QValue operator() (const Action &A) {
        unsigned ActionIndex{A.getIndex()};
        return {Original, S, A, Results[ActionIndex]};
      }

      const QValue operator() (const Action &A) const {
        unsigned ActionIndex{A.getIndex()};
        return {Original, S, A, Results[ActionIndex]};
      }

      ActionValuePair
      max_pair() const {
        auto Begin = std::begin(Results),
             End   = std::end(Results);
        auto MaxIterator = std::max_element(Begin, End);
        unsigned Index = MaxIterator - Begin;
        Action A = Action::getActionByIndex(Index);
        return {A, *MaxIterator};
      }
    private:
      CurriedQ(QCore *original, const State &s, ResultsVector &&results) :
        Original(original), S(s), Results(results) {}

      CurriedQ(const QCore *original, const State &s, ResultsVector &&results) :
        Original(original), S(s), Results(results) {}

      mutable QCore *Original;
      const State &S;
      ResultsVector Results;

      friend class Q<QCore>;
    };

    CurriedQ operator() (const State &S) {
      return {&Original, S, Original.calculate(S)};
    }

    const CurriedQ operator() (const State &S) const {
      return {&Original, S, Original.calculate(S)};
    }

    QValue operator() (const State &S, const Action &A) {
      return (*this)(S)(A);
    }

    const QValue operator() (const State &S, const Action &A) const {
      return (*this)(S)(A);
    }

    Q() = default;
    Q(const Q<QCore> &) = delete;
    Q(Q<QCore> &&) = default;

    Q<QCore> &operator=(const Q<QCore> &) = delete;
    Q<QCore> &operator=(Q<QCore> &&) = default;

  private:
    mutable QCore Original;
};

  template <class QCore>
  using QS = typename Q<QCore>::CurriedQ;

  template <class Q>
  typename Q::Action argmax(const Q &Function) {
    return Function.max_pair().first;
  }
  template <class Q>
  typename Q::Result max(const Q &Function) {
    return Function.max_pair().second;
  }
}
}
}

#endif /* LLVM_WAZUHL_Q_H */
