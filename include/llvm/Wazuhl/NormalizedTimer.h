#ifndef LLVM_WAZUHL_NORMALIZEDTIMER_H
#define LLVM_WAZUHL_NORMALIZEDTIMER_H

#include "llvm/Support/Timer.h"

namespace llvm {
namespace wazuhl {
class NormalizedTimer {
public:
  static double getTime();
  static double getNormalizedTime();
  static void init();

private:
  static NormalizedTimer &getTimer();
  double getTimeImpl();
  double getNormalizedTimeImpl();
  double getTotalTimeImpl();
  double getNormalizedTotalTimeImpl();
  void startInnerTimer();
  void stopInnerTimer();

  NormalizedTimer();
  NormalizedTimer(const NormalizedTimer &) = delete;
  NormalizedTimer(NormalizedTimer &&) = delete;

  NormalizedTimer &operator=(const NormalizedTimer &) = delete;
  NormalizedTimer &operator=(NormalizedTimer &&) = delete;

  Timer InnerTimer;
  TimeRecord Total;
  double NormalizationTime;
  static bool IsInitialized;
};
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_NORMALIZEDTIMER_H */
