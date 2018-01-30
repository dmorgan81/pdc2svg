#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "color.h"
#include "svg.h"

struct svg {
    FILE *stream;
};

struct svg *svg_create(FILE *stream, int16_t w, int16_t h, int16_t frame_count, int16_t play_count) {
    struct svg *svg = talloc(NULL, struct svg);
    svg->stream = stream;

    fprintf(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(stream, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" preserveAspectRatio=\"xMidYMid\"");
    fprintf(stream, " viewBox=\"%d %d %d %d\" width=\"%d\" height=\"%d\"", -w/2, -h/2, w * 2, h * 2, w * 3, h * 3);
    if (play_count != 0) fprintf(stream, " onload=\"StartAnimation(evt)\"");
    fprintf(stream, ">\n");

    if (play_count != 0) {
        fprintf(stream, "<script type=\"application/javascript\"><![CDATA[\n");
        fprintf(stream, "var i = 0;\n");
        fprintf(stream, "var frameCount = %d;\n", frame_count);
        fprintf(stream, "var playCount = %d;\n", play_count);
        fprintf(stream, "var groups = [];\n");
        fprintf(stream, "function StartAnimation(evt) {\n");
        fprintf(stream, "\tif (playCount === 0) return;\n");
        fprintf(stream, "\tgroups = document.getElementsByTagName('g');\n");
        fprintf(stream, "\tgroups[0].style.display = 'inline';\n");
        fprintf(stream, "\twindow.setTimeout(Animate, groups[0].dataset.duration);\n");
        fprintf(stream, "}\n");
        fprintf(stream, "function Animate() {\n");
        fprintf(stream, "\tgroups[i++].style.display = 'none';\n");
        fprintf(stream, "\tif (i === frameCount && playCount !== 0) {\n");
        fprintf(stream, "\t\ti = 0;\n");
        fprintf(stream, "\t\tif (playCount !== -1) playCount -= 1;\n");
        fprintf(stream, "\t}\n");
        fprintf(stream, "\tif (playCount === 0) return;\n");
        fprintf(stream, "\tgroups[i].style.display = 'inline';\n");
        fprintf(stream, "\twindow.setTimeout(Animate, groups[i].dataset.duration);\n");
        fprintf(stream, "}\n");
        fprintf(stream, "]]></script>\n");
    }

    return svg;
}

void svg_finish(struct svg *svg) {
    fprintf(svg->stream, "</svg>\n");
}

struct svg_path {
    FILE *stream;
    char *d;
};

struct svg_path *svg_create_path(struct svg *svg) {
    struct svg_path *path = talloc(svg, struct svg_path);
    path->stream = svg->stream;
    path->d = NULL;    
    fprintf(svg->stream, "\t<path stroke-linecap=\"round\" stroke-linejoin=\"round\"");
    return path;
}

void svg_path_fill_color(struct svg_path *path, uint8_t color) {
    if (color == 0) {
        fprintf(path->stream, " fill=\"rgba(0,0,0,0)\"");
    } else {
        fprintf(path->stream, " fill=\"");
        print_color_to_rgba(path->stream, color);
        fprintf(path->stream, "\"");
    }
}

void svg_path_stroke_color(struct svg_path *path, uint8_t color) {
    if (color == 0) {
        fprintf(path->stream, " stroke=\"rgba(0,0,0,0)\"");
    } else {
        fprintf(path->stream, " stroke=\"");
        print_color_to_rgba(path->stream, color);
        fprintf(path->stream, "\"");
    }
}

void svg_path_stroke_width(struct svg_path *path, uint8_t width) {
    fprintf(path->stream, " stroke-width=\"%d\"", width);
}

void svg_path_move_to(struct svg_path *path, int16_t x, int16_t y) {
    if (!path->d) path->d = talloc_strdup(path, "");
    path->d = talloc_asprintf_append(path->d, "M%d,%d", x, y);
}

void svg_path_line_to(struct svg_path *path, int16_t x, int16_t y) {
    if (!path->d) path->d = talloc_strdup(path, "");
    path->d = talloc_asprintf_append(path->d, "L%d,%d", x, y);
}

void svg_path_mark_hidden(struct svg_path *path, bool hidden) {
    if (hidden) fprintf(path->stream, " display=\"none\"");
}

void svg_path_finish(struct svg_path *path, bool open) {
    if (path->d) {
        fprintf(path->stream, " d=\"%s%s\"", path->d, open ? "" : "Z");
        talloc_free(path->d);
        path->d = NULL;
    }
    fprintf(path->stream, "/>\n");
}

struct svg_circle {
    FILE *stream;
};

struct svg_circle *svg_create_circle(struct svg *svg, int16_t x, int16_t y, uint16_t r) {
    struct svg_circle *circle = talloc(svg, struct svg_circle);
    circle->stream = svg->stream;
    fprintf(circle->stream, "\t<circle cx=\"%d\" cy=\"%d\" r=\"%d\"", x, y, r);
    return circle;
}

void svg_circle_fill_color(struct svg_circle *circle, uint8_t color) {
    if (color == 0) {
        fprintf(circle->stream, " fill=\"rgba(0,0,0,0)\"");
    } else {
        fprintf(circle->stream, " fill=\"");
        print_color_to_rgba(circle->stream, color);
        fprintf(circle->stream, "\"");
    }
}

void svg_circle_stroke_color(struct svg_circle *circle, uint8_t color) {
    if (color == 0) {
        fprintf(circle->stream, " stroke=\"rgba(0,0,0,0)\"");
    } else {
        fprintf(circle->stream, " stroke=\"");
        print_color_to_rgba(circle->stream, color);
        fprintf(circle->stream, "\"");
    }
}

void svg_circle_stroke_width(struct svg_circle *circle, uint8_t width) {
    fprintf(circle->stream, " stroke-width=\"%d\"", width);
}

void svg_circle_mark_hidden(struct svg_circle *circle, bool hidden) {
    if (hidden) fprintf(circle->stream, " display=\"none\"");
}

void svg_circle_finish(struct svg_circle *circle) {
    fprintf(circle->stream, "/>\n");
}

struct svg_g {
    FILE *stream;
};

struct svg_g *svg_create_g(struct svg *svg, uint16_t duration) {
    struct svg_g *g = talloc(svg, struct svg_g);
    g->stream = svg->stream;
    fprintf(g->stream, "<g style=\"display:none\" data-duration=\"%d\">\n", duration);
    return g;
}

void svg_g_finish(struct svg_g *g) {
    fprintf(g->stream, "</g>\n");
}
