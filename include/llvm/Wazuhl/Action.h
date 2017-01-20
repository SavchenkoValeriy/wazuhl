#ifndef LLVM_WAZUHL_ACTION_H
#define LLVM_WAZUHL_ACTION_H

#include "llvm/IR/PassManagerInternal.h"
#include <string>
#include <vector>

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

    const std::string Name;
    const PassConstructorT PassConstructor;
    const unsigned Index;
  public:
    Action(const std::string &name, PassConstructorT ctor, unsigned index) :
      Name(name), PassConstructor(ctor), Index(index) {}

    ActionResult *takeAction() const {
      return PassConstructor();
    }

    const StringRef getName() const {
      return Name;
    }

    unsigned getIndex() const {
      return Index;
    }

    static ActionList getAllPossibleActions();
    static Action getActionByIndex(unsigned Index);
    static const Action &getActionByName(const StringRef);
  };
}
}

#endif /* LLVM_WAZUHL_ACTION_H */
