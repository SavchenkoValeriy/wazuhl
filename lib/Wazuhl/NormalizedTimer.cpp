#include "llvm/Wazuhl/NormalizedTimer.h"

namespace {
double getCPUTime(llvm::TimeRecord &time) {
  return time.getUserTime() + time.getSystemTime();
}
} // namespace

namespace llvm {
namespace wazuhl {
bool NormalizedTimer::IsInitialized = false;

NormalizedTimer::Initializer NormalizedTimer::init() {
  if (IsInitialized)
    return {};
  return {getTimer()};
}

NormalizedTimer::NormalizedTimer()
    : InnerTimer(), Total(), NormalizationFactor(0.0) {
  InnerTimer.init(
      "Wazuhl's timer",
      "The purpose of this timer is to add time into Wazuhl's states");
  startInnerTimer();
}

double NormalizedTimer::getTime() { return getTimer().getTimeImpl(); }

double NormalizedTimer::getTimeImpl() {
  stopInnerTimer();
  Total += InnerTimer.getTotalTime();
  startInnerTimer();
  return getCPUTime(Total);
}

double NormalizedTimer::getNormalizedTime() {
  return getTimer().getNormalizedTimeImpl();
}

double NormalizedTimer::getNormalizedTimeImpl() {
  if (!IsInitialized)
    return 1.0;
  assert(NormalizationFactor != 0 && "Normalization factor shouldn't be 0!");
  return getTimeImpl() / NormalizationFactor;
}

NormalizedTimer &NormalizedTimer::getTimer() {
  static NormalizedTimer singleton;
  return singleton;
}

void NormalizedTimer::startInnerTimer() { InnerTimer.startTimer(); }

void NormalizedTimer::stopInnerTimer() { InnerTimer.stopTimer(); }

NormalizedTimer::Initializer::~Initializer() {
  if (Master) {
    Master->NormalizationFactor = NormalizedTimer::getTime();
    NormalizedTimer::IsInitialized = true;
  }
}
} // namespace wazuhl
} // namespace llvm
