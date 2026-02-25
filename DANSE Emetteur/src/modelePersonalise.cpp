#include "modelePersonalise.h"
#include <Arduino.h>

const Frame modele[] = {
    {0, 255, 255, 50, 3000},
    {255, 0, 0, 128, 2000},
    {0, 0, 255, 64, 1000},
    {255, 255, 0, 50, 5000},
    {255, 255, 0, 128, 2000},
    {255, 0, 0, 64, 2000},
    {0, 0, 0, 0, 3000},
};

const size_t NUM_FRAMES = sizeof(modele) / sizeof(modele[0]);