#ifndef LLVM_WAZUHL_EXPERIENCEREPLAY_H
#define LLVM_WAZUHL_EXPERIENCEREPLAY_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Wazuhl/DQN.h"

#include <utility>

namespace llvm {
namespace wazuhl {

class ExperienceReplayImpl;

class ExperienceReplay {
public:
  ExperienceReplay();
  ~ExperienceReplay();

  using State = DQNCore::State;
  using Result = DQNCore::Result;

  struct ExperienceUnit {
    State state;
    unsigned actionIndex;
    Result value;
  };

  using RecalledExperience = SmallVector<ExperienceUnit, config::MinibatchSize>;

  void addToExperience(ExperienceUnit);
  RecalledExperience replay();

private:
  std::unique_ptr<ExperienceReplayImpl> pImpl;
};

} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_EXPERIENCEREPLAY_H */
