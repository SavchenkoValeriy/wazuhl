#ifndef LLVM_WAZUHL_EXPERIENCEREPLAY_H
#define LLVM_WAZUHL_EXPERIENCEREPLAY_H

#include "llvm/ADT/SmallVector.h"
#include <utility>

namespace llvm { namespace wazuhl {

  class FeatureVector;

  class ExperienceReplay {
  public:
    static constexpr unsigned int MinibatchSize = 32;

    using Result = double;
    using ExperienceUnit = std::pair<const FeatureVector &,
                                     Result>;
    using RecalledExperience = SmallVector<ExperienceUnit,
                                           MinibatchSize>;

    void load();
    void addToExperience(ExperienceUnit);
    RecalledExperience replay();
  };

} }

#endif /* LLVM_WAZUHL_EXPERIENCEREPLAY_H */
