#ifndef LLVM_WAZUHL_CONFIG_H
#define LLVM_WAZUHL_CONFIG_H

#include "llvm/ADT/StringRef.h"

namespace llvm {
namespace wazuhl {
namespace config {

void ensureConfig();
StringRef getWazuhlConfigPath();
StringRef getTrainingNetFile();
StringRef getTargetNetFile();

constexpr unsigned MinibatchSize = 32;
#define LAST_OTHER_INST(num) constexpr auto NumberOfRawIRFeatures = num;
#include "llvm/IR/Instruction.def"

constexpr unsigned NumberOfIRFeatures = NumberOfRawIRFeatures * 4,
                   DiffIRFeaturesOffset = NumberOfIRFeatures,
                   TimeIndex = NumberOfIRFeatures * 2,
                   ActionOffset = TimeIndex + 1, NumberOfActions = 107,
                   NumberOfFeatures = NumberOfIRFeatures * 2 + 1,
                   ExperienceSize = 10000, MinimalExperienceSize = 500,
                   ContextSize = 30, ContextEmbeddingSize = 8,
                   ContextLSTMSize = 32, ActionHiddenSize = 128,
                   EncodedIRFeaturesSize = 64,
                   EncodedStateSize = EncodedIRFeaturesSize + ContextLSTMSize,
                   StepsBeforeUpdate = 500, FinalAnnealingStep = 50000;

constexpr double InitialEpsilon = 0.9, FinalEpsilon = 0.1;

constexpr auto EncoderLayerSizes = {128, 64};

constexpr bool UseRepeatingPolicy = false;

} // namespace config
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_CONFIG_H */
