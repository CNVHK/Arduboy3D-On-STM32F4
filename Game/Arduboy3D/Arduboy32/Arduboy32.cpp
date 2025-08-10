#include "main.h"

unsigned long millis() {
    return HAL_GetTick();
}