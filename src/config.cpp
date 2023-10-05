#include "config.hpp"

const uint8_t filamentBrightnesses[FILAMENT_BRIGHTNESSES] = {
        255,
        127
};
const uint32_t stripColors[STRIP_COLORS] = {
        0xFF0000, // red
        0x00FF00, // green
        0x0000FF, // blue
        0xC0C0C0, // 75% white (over-current safety)
        0xFF7000, // orange
        0xFFFF00, // yellow
        0x00FFFF, // aqua / cyan
        0xFF00FF, // purple
};
