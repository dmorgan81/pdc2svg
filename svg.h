#ifndef _SVG_H_
#define _SVG_H_

#include <stdbool.h>
#include <stdio.h>

struct svg;

struct svg *svg_create(FILE *stream, int16_t w, int16_t h);
void svg_finish(struct svg *svg);

struct svg_path;

struct svg_path *svg_create_path(struct svg *svg);
void svg_path_fill_color(struct svg_path *path, uint8_t color);
void svg_path_stroke_color(struct svg_path *path, uint8_t color);
void svg_path_stroke_width(struct svg_path *path, uint8_t width);
void svg_path_move_to(struct svg_path *path, int16_t x, int16_t y);
void svg_path_line_to(struct svg_path *path, int16_t x, int16_t y);
void svg_path_finish(struct svg_path *path, bool open);

#endif
