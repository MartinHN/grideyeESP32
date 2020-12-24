struct Timeout {
  Timeout(int _interval) : interval(_interval) {}
  bool update(unsigned long long newTime) {
    if (newTime - lastValidTime > interval) {
      lastValidTime = newTime;
      return true;
    }
    return false;
  }
  unsigned long long lastValidTime = 0;
  int interval;
};
