#pragma once
namespace Navigation {
class InputCapture {
  public:
    void Open() { armed = false; }
    void Close() { pendingRelease = true; }
    bool CanToggle() const { return !pendingRelease; }
    bool Ready(bool held) {
        if (!held) armed = true;
        return armed;
    }
    // The neutral release frame is still consumed; gameplay resumes next frame.
    bool Consume(bool open, bool held) {
        const bool consume = open || pendingRelease;
        if (!open && !held) pendingRelease = false;
        return consume;
    }
    bool BlocksGame(bool open) const { return open || pendingRelease; }
  private:
    bool armed = false, pendingRelease = false;
};
}
