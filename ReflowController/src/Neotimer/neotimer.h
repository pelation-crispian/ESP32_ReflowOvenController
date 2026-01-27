#ifndef NEOTIMER_H
#define NEOTIMER_H

#define NEOTIMER_INDEFINITE -1
#define NEOTIMER_UNLIMITED -1

#include <Arduino.h>

class Neotimer {
 public:
  Neotimer();
  Neotimer(long _t);
  ~Neotimer();

  void init();
  boolean done();
  boolean repeat(int times);
  boolean repeat(int times, long _t);
  boolean repeat();
  void repeatReset();
  boolean waiting();
  boolean started();
  void start();
  long stop();
  void restart();
  void reset();
  void set(long t);
  long get();
  boolean debounce(boolean signal);
  int repetitions = NEOTIMER_UNLIMITED;

 private:
  struct myTimer {
    long time;
    long last;
    boolean done;
    boolean started;
  };

  myTimer _timer;
  boolean _waiting;
};

inline Neotimer::Neotimer() {
  this->_timer.time = 1000; // Default 1 second interval if not specified
}

inline Neotimer::Neotimer(long _t) {
  this->_timer.time = _t;
}

inline Neotimer::~Neotimer() {
}

inline void Neotimer::init() {
  this->_waiting = false;
}

inline boolean Neotimer::repeat(int times) {
  if (times != NEOTIMER_UNLIMITED) {
    // First repeat
    if (this->repetitions == NEOTIMER_UNLIMITED) {
      this->repetitions = times;
    }
    // Stop
    if (this->repetitions == 0) {
      return false;
    }

    if (this->repeat()) {
      this->repetitions--;
      return true;
    }
    return false;
  }
  return this->repeat();
}

inline boolean Neotimer::repeat(int times, long _t) {
  this->_timer.time = _t;
  return this->repeat(times);
}

inline boolean Neotimer::repeat() {
  if (this->done()) {
    this->reset();
    return true;
  }
  if (!this->_timer.started) {
    this->_timer.last = millis();
    this->_timer.started = true;
    this->_waiting = true;
  }
  return false;
}

inline void Neotimer::repeatReset() {
  this->repetitions = NEOTIMER_UNLIMITED;
}

inline boolean Neotimer::done() {
  if (!this->_timer.started) return false;
  if ((millis() - this->_timer.last) >= this->_timer.time) {
    this->_timer.done = true;
    this->_waiting = false;
    return true;
  }
  return false;
}

inline void Neotimer::set(long t) {
  this->_timer.time = t;
}

inline long Neotimer::get() {
  return this->_timer.time;
}

inline boolean Neotimer::debounce(boolean signal) {
  if (this->done() && signal) {
    this->start();
    return true;
  }
  return false;
}

inline void Neotimer::reset() {
  this->stop();
  this->_timer.last = millis();
  this->_timer.done = false;
}

inline void Neotimer::start() {
  this->reset();
  this->_timer.started = true;
  this->_waiting = true;
}

inline long Neotimer::stop() {
  this->_timer.started = false;
  this->_waiting = false;
  return millis() - this->_timer.last;
}

inline void Neotimer::restart() {
  if (!this->done()) {
    this->_timer.started = true;
    this->_waiting = true;
  }
}

inline boolean Neotimer::waiting() {
  return (this->_timer.started && !this->done()) ? true : false;
}

inline boolean Neotimer::started() {
  return this->_timer.started;
}

#endif
