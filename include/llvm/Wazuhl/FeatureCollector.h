#ifndef LLVM_WAZUHL_FEATURECOLLECTOR_H
#define LLVM_WAZUHL_FEATURECOLLECTOR_H

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Wazuhl/Config.h"

#include <utility>

namespace llvm {
namespace wazuhl {
using RawFeatureVector = SmallVector<double, config::NumberOfRawIRFeatures>;
using FeatureVector = SmallVector<double, config::NumberOfIRFeatures>;

class RawIRFeatures {
public:
  RawIRFeatures(unsigned N);
  RawIRFeatures() = default;

  void addVectorForFunction(Function *F, const RawFeatureVector &V) {
    Matrix.emplace_back(V);
    // we consider that functions don't move around and their pointers
    // could be used as their unique identifiers
    FunctionToIndexMap[F] = Matrix.size() - 1;
  }

  RawIRFeatures operator-(const RawIRFeatures &RHS) const {
    RawIRFeatures Result;
    RawFeatureVector TempVector;

    for (auto FI : RHS.FunctionToIndexMap) {
      auto FJ = FunctionToIndexMap.find(FI.first);

      if (FJ == FunctionToIndexMap.end()) {
        continue;
      }

      auto i = FI.second, j = FJ->second;
      for (auto k : seq<unsigned>(0, config::NumberOfRawIRFeatures)) {
        TempVector.push_back(Matrix[j][k] - RHS.Matrix[i][k]);
      }

      Result.addVectorForFunction(FI.first, std::move(TempVector));
      TempVector.clear();
    }

    return Result;
  }

  inline FeatureVector operator/(const RawIRFeatures &RHS) const;

private:
  static constexpr auto AverageNumberOfFunctions = 100;

  struct Stats {
    double min, max, mean, variance;

    inline static Stats calculate(const SmallVectorImpl<double> &);
  };

  using MatrixType = SmallVector<RawFeatureVector, AverageNumberOfFunctions>;
  MatrixType Matrix;
  DenseMap<Function *, unsigned> FunctionToIndexMap;
};

class FunctionFeatureCollector
    : public AnalysisInfoMixin<FunctionFeatureCollector> {
  friend AnalysisInfoMixin<FunctionFeatureCollector>;
  static AnalysisKey Key;

public:
  using Result = RawFeatureVector;
  Result run(Function &F, FunctionAnalysisManager &AM);
};

class ModuleFeatureCollector
    : public AnalysisInfoMixin<ModuleFeatureCollector> {
  friend AnalysisInfoMixin<ModuleFeatureCollector>;
  static AnalysisKey Key;

public:
  using Result = RawIRFeatures;
  Result run(Module &M, ModuleAnalysisManager &AM);
};

FeatureVector RawIRFeatures::operator/(const RawIRFeatures &RHS) const {
  FeatureVector Result;

  SmallVector<std::pair<unsigned, unsigned>, AverageNumberOfFunctions>
      Functions;

  for (auto FI : RHS.FunctionToIndexMap) {
    auto FJ = FunctionToIndexMap.find(FI.first);

    if (FJ == FunctionToIndexMap.end()) {
      continue;
    }

    Functions.push_back(std::make_pair(FJ->second, FI.second));
  }

  SmallVector<double, AverageNumberOfFunctions> DistSingleScaled;
  double SingleScaler = 1, OverallScaler = 1;
  for (auto i : seq<unsigned>(0, config::NumberOfRawIRFeatures)) {
    for (auto Pair : Functions) {
      SingleScaler = RHS.Matrix[Pair.second][i];
      OverallScaler = RHS.Matrix[Pair.second][0];

      if (SingleScaler == 0) {
        SingleScaler = OverallScaler;
      }

      DistSingleScaled.push_back(Matrix[Pair.first][i] / SingleScaler);
    }

    auto SingleStats = Stats::calculate(DistSingleScaled);

    Result.push_back(SingleStats.min);
    Result.push_back(SingleStats.max);
    Result.push_back(SingleStats.mean);
    Result.push_back(SingleStats.variance);

    DistSingleScaled.clear();
  }

  return Result;
}

RawIRFeatures::Stats
RawIRFeatures::Stats::calculate(const SmallVectorImpl<double> &Dist) {
  Stats Result;
  assert(not Dist.empty());

  Result.min = Dist[0];
  Result.max = Dist[0];

  for (double value : Dist) {
    Result.min = std::min(Result.min, value);
    Result.max = std::max(Result.min, value);
    Result.mean += value;
    Result.variance += value * value;
  }

  auto N = Dist.size();
  Result.mean /= N;
  auto delimeter = N >= 2 ? N - 1 : N;
  Result.variance =
      (Result.variance - N * Result.mean * Result.mean) / delimeter;

  return Result;
}

} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_FEATURECOLLECTOR_H */
