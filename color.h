#ifndef _COLOR_H_
#define _COLOR_H_

#include <stdint.h>
#include <stdio.h>

static void __print_color_to_rgba(FILE *stream, uint8_t color) {
    fprintf(stream, "rgba(%d,%d,%d,%d)",
        (color >> 4 & 3) / 3 * 255,
        (color >> 2 & 3) / 3 * 255,
        (color >> 0 & 3) / 3 * 255,
        (color >> 6 & 3) / 3 * 1);
}

#define print_color_to_rgba(stream, color) __print_color_to_rgba(stream, color)

#endif
