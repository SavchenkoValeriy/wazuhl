#ifndef LLVM_WAZUHL_NORMALIZEDTIMER_H
#define LLVM_WAZUHL_NORMALIZEDTIMER_H

#include "llvm/Support/Timer.h"

namespace llvm {
namespace wazuhl {
  class NormalizedTimer {
  public:
    class Initializer {
    public:
      Initializer() = default;
      Initializer(NormalizedTimer &timer) : Master(&timer) {}

      Initializer(const Initializer &) = delete;
      Initializer(Initializer &&) = default;

      Initializer &operator = (const Initializer &) = delete;
      Initializer &operator = (Initializer &&) = delete;

      ~Initializer();

    private:
      NormalizedTimer *Master = nullptr;
    };

    static Initializer init();
    static double getTime();
    static double getNormalizedTime();
  private:
    static NormalizedTimer &getTimer();
    double getTimeImpl();
    double getNormalizedTimeImpl();
    void startInnerTimer();
    void stopInnerTimer();

    NormalizedTimer();
    NormalizedTimer(const NormalizedTimer &) = delete;
    NormalizedTimer(NormalizedTimer &&) = delete;

    NormalizedTimer &operator = (const NormalizedTimer &) = delete;
    NormalizedTimer &operator = (NormalizedTimer &&) = delete;

    Timer InnerTimer;
    TimeRecord Total;
    double NormalizationFactor;
    static bool IsInitialized;
    friend class Initializer;
  };
}
}

#endif /* LLVM_WAZUHL_NORMALIZEDTIMER_H */
