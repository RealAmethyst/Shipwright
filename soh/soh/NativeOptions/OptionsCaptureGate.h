#pragma once
#include <cstdint>

namespace NativeOptions {
class CaptureGate {
  public:
    enum class Step { Wait, Arm, Poll, Cancel };
    explicit CaptureGate(uint32_t now) : deadline(now + 10000) {}
    Step Update(uint32_t now, bool neutral, bool cancel) {
        if (cancel || static_cast<int32_t>(now - deadline) >= 0) return Step::Cancel;
        if (armed) return Step::Poll;
        if (neutral) {
            armed = true;
            return Step::Arm;
        }
        return Step::Wait;
    }
  private:
    uint32_t deadline;
    bool armed = false;
};
}
