#ifndef LLVM_WAZUHL_EXPERIENCEREPLAY_H
#define LLVM_WAZUHL_EXPERIENCEREPLAY_H

#include "llvm/Wazuhl/DQN.h"
#include "llvm/ADT/SmallVector.h"
#include <utility>

namespace llvm { namespace wazuhl {

  class FeatureVector;

  class ExperienceReplay {
  public:
    static constexpr unsigned int MinibatchSize = 32;

    using ExperienceUnit = std::pair<DQNCore::State,
                                     DQNCore::ResultsVector>;
    using RecalledExperience = SmallVector<ExperienceUnit,
                                           MinibatchSize>;

    void load();
    void addToExperience(ExperienceUnit);
    RecalledExperience replay();
  };

} }

#endif /* LLVM_WAZUHL_EXPERIENCEREPLAY_H */
