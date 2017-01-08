#ifndef LLVM_WAZUHL_ACTION_H
#define LLVM_WAZUHL_ACTION_H

#include "llvm/IR/PassManagerInternal.h"

namespace llvm {

class Module;
class PassInfo;

namespace wazuhl {

  using ActionResult = detail::PassConcept<Module, AnalysisManager<Module>>;
  class Action;
  using ActionList = std::vector<Action>;

  class Action {
  private:
    using PassConstructorT = std::function<ActionResult *()>;
    PassConstructorT PassConstructor;
    Action(PassConstructorT ctor) : PassConstructor(ctor) {}
  public:

    ActionResult *takeAction() const {
      return PassConstructor();
    }

    static ActionList getAllPossibleActions();
  };
}
}

#endif /* LLVM_WAZUHL_ACTION_H */
