#ifndef LLVM_WAZUHL_CONFIG_H
#define LLVM_WAZUHL_CONFIG_H

#include "llvm/ADT/StringRef.h"

namespace llvm {
namespace wazuhl {
namespace config {

StringRef getCaffeModelPath();
StringRef getCaffeSolverPath();
StringRef getWazuhlConfigPath();
StringRef getTrainedNetFile();

constexpr unsigned MinibatchSize = 32;
#define LAST_OTHER_INST(num) constexpr auto NumberOfRawIRFeatures = num;
#include "llvm/IR/Instruction.def"

constexpr unsigned NumberOfIRFeatures = NumberOfRawIRFeatures * 4,
                   DiffIRFeaturesOffset = NumberOfIRFeatures,
                   TimeIndex = NumberOfIRFeatures * 2,
                   ActionOffset = TimeIndex + 1, NumberOfActions = 107,
                   NumberOfFeatures = NumberOfIRFeatures * 2 + 1,
                   ExperienceSize = 3000, MinimalExperienceSize = 1000,
                   ContextSize = 30, ContextEmbeddingSize = 8,
                   ContextLSTMSize = 32, ActionHiddenSize = 128,
                   EncodedIRFeaturesSize = 64,
                   EncodedStateSize = EncodedIRFeaturesSize + ContextLSTMSize;

constexpr auto EncoderLayerSizes = {128, 64};

constexpr bool UseRepeatingPolicy = false;

} // namespace config
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_CONFIG_H */
