#pragma once
#include <Arduino.h>

struct Frame {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness;
  uint16_t durationMs;
};

extern const Frame modele[];
extern const size_t NUM_FRAMES;