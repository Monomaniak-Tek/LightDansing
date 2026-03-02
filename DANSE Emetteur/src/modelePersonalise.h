#pragma once
#include <Arduino.h>

static const uint8_t FRAME_GROUP_MASK_ALL = 0x3F; // bits groupes 0..5

// Ecriture "humaine" des groupes:
// GROUPS(0134) -> groupes 0,1,3,4
// GROUPS(25)   -> groupes 2,5
// Les caracteres hors 0..5 sont ignores.
constexpr uint8_t groupMaskFromDigits(const char* s, uint8_t acc = 0) {
  return (*s == '\0')
             ? acc
             : groupMaskFromDigits(
                   s + 1,
                   (uint8_t)((*s >= '0' && *s <= '5') ? (acc | (uint8_t)(1U << (*s - '0'))) : acc));
}
#define GROUPS(x) groupMaskFromDigits(#x)

struct Frame {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness;
  uint16_t durationMs;
  uint8_t groupMask;

  // groupMask est optionnel: par defaut, la frame cible tous les groupes (0..5).
  constexpr Frame(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t brightness_, uint16_t durationMs_,
                  uint8_t groupMask_ = FRAME_GROUP_MASK_ALL)
      : r(r_),
        g(g_),
        b(b_),
        brightness(brightness_),
        durationMs(durationMs_),
        groupMask(groupMask_) {}
};

extern const Frame modele[];
extern const size_t NUM_FRAMES;
