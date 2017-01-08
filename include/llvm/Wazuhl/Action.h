#ifndef LLVM_WAZUHL_ACTION_H
#define LLVM_WAZUHL_ACTION_H

#include "llvm/IR/PassManagerInternal.h"

namespace llvm {

class Module;
class PassInfo;

namespace wazuhl {

  using ActionResult = detail::PassConcept<Module, AnalysisManager<Module>>;

  class Action {
  private:
    using PassConstructorT = std::function<ActionResult *()>;
    PassConstructorT PassConstructor;
    friend class ActionList;
  public:
    Action(PassConstructorT ctor) : PassConstructor(ctor) {}

    ActionResult *takeAction() const {
      return PassConstructor();
    }
  };

  class ActionList {
  private:
    using ListType = std::vector<Action>;
    ListType PossibleActions;
  public:
    ActionList();
    using iterator = ListType::iterator;
    using const_iterator = ListType::const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_t size() const;
  };
}
}

#endif /* LLVM_WAZUHL_ACTION_H */
