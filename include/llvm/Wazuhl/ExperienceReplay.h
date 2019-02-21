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

  using ExperienceUnit = std::pair<DQNCore::State, DQNCore::ResultsVector>;
  using State = DQNCore::State;
  using RecalledExperience = SmallVector<ExperienceUnit, config::MinibatchSize>;

  void addToExperience(ExperienceUnit, unsigned index);
  RecalledExperience replay();

private:
  std::unique_ptr<ExperienceReplayImpl> pImpl;
};

} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_EXPERIENCEREPLAY_H */
