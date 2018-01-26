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
void svg_path_mark_hidden(struct svg_path *path, bool hidden);
void svg_path_finish(struct svg_path *path, bool open);
void svg_path_close(struct svg_path *path);

struct svg_circle;

struct svg_circle *svg_create_circle(struct svg *svg, int16_t x, int16_t y, uint16_t r);
void svg_circle_fill_color(struct svg_circle *circle, uint8_t color);
void svg_circle_stroke_color(struct svg_circle *circle, uint8_t color);
void svg_circle_stroke_width(struct svg_circle *circle, uint8_t width);
void svg_circle_mark_hidden(struct svg_circle *circle, bool hidden);
void svg_circle_finish(struct svg_circle *circle);
void svg_circle_close(struct svg_circle *circle);

struct svg_g;

struct svg_g *svg_create_g(struct svg *svg, int16_t y);
void svg_g_finish(struct svg_g *g);

#endif
