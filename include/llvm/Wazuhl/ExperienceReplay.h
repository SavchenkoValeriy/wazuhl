#ifndef LLVM_WAZUHL_EXPERIENCEREPLAY_H
#define LLVM_WAZUHL_EXPERIENCEREPLAY_H

#include "llvm/Wazuhl/DQN.h"
#include "llvm/ADT/SmallVector.h"
#include <utility>

namespace llvm { namespace wazuhl {

  class ExperienceReplayImpl;

  class ExperienceReplay {
  public:
    static constexpr unsigned int MinibatchSize = 32;

    ExperienceReplay();
    ~ExperienceReplay();

    using ExperienceUnit = std::pair<DQNCore::State,
                                     DQNCore::ResultsVector>;
    using RecalledExperience = SmallVector<ExperienceUnit,
                                           MinibatchSize>;

    void addToExperience(ExperienceUnit);
    RecalledExperience replay();

  private:
    std::unique_ptr<ExperienceReplayImpl> pImpl;
  };

} }

#endif /* LLVM_WAZUHL_EXPERIENCEREPLAY_H */
