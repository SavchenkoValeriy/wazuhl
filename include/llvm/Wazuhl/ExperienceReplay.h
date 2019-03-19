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
  using Action = DQNCore::Action;
  using Result = DQNCore::Result;
  template <class T> using Batch = DQNCore::Batch<T>;

  struct RecalledExperience {
    Batch<State> S;
    Batch<Action> A;
    Batch<Result> R;
    Batch<State> newS;
    Batch<bool> isTerminal;

    unsigned size() { return S.size(); }
  };

  void push(const State &, const Action &, Result R, const State &);
  RecalledExperience sample();
  bool isBigEnoughForReplay();

private:
  std::unique_ptr<ExperienceReplayImpl> pImpl;
};

} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_EXPERIENCEREPLAY_H */
