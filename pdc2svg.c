#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#include "pdc.h"
#include "svg.h"

static const uint8_t LEN_MAGIC = 4;

enum CommandType {
    CommandTypeInvalid = 0,
    CommandTypePath,
    CommandTypeCircle,
    CommandTypePrecisePath
};

static char *prv_read_bytes(void *ctx, FILE *fp, char *name) {
    uint32_t size;
    if (fread(&size, sizeof(uint32_t), 1, fp) != 1) {
        fprintf(stderr, "Error reading file: %s\n", name);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    char *bytes = talloc_array(ctx, char, size);
    if (fread(bytes, 1, size, fp) != size) {
        fprintf(stderr, "Error reading file: %s\n", name);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    return bytes;
}

static void prv_output_command_list_to_svg(struct pdc_list *list, struct svg *svg) {
    for (size_t i = 0; i < list->num_commands; i++) {
        struct pdc command = list->commands[i];

        if (command.type == CommandTypePath || command.type == CommandTypePrecisePath) {
            struct svg_path *path = svg_create_path(svg);
            svg_path_fill_color(path, command.fill_color);
            svg_path_stroke_color(path, command.stroke_color);
            svg_path_stroke_width(path, command.stroke_width);
            svg_path_mark_hidden(path, command.flags & 1);

            uint8_t scale = command.type == CommandTypePrecisePath ? 8 : 1;
            struct point *points = command.points;
            svg_path_move_to(path, points[0].x / scale, points[0].y / scale);
            for (size_t j = 1; j < command.num_points; j++) {
                svg_path_line_to(path, points[j].x / scale, points[j].y / scale);
            }
            svg_path_finish(path, command.path_open_or_radius & 1);
            talloc_free(path);
        } else if (command.type == CommandTypeCircle) {
            struct point point = command.points[0];
            struct svg_circle *circle = svg_create_circle(svg, point.x, point.y, command.path_open_or_radius);
            svg_circle_fill_color(circle, command.fill_color);
            svg_circle_stroke_color(circle, command.stroke_color);
            svg_circle_stroke_width(circle, command.stroke_width);
            svg_circle_mark_hidden(circle, command.flags & 1);
            svg_circle_finish(circle);
            talloc_free(circle);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s PDC\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "r");    
    if (!fp) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char magic[LEN_MAGIC + 1];
    if (fread(magic, sizeof(char), LEN_MAGIC, fp) != LEN_MAGIC) {
        fprintf(stderr, "Error reading file: %s\n", argv[1]);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    magic[LEN_MAGIC] = '\0';

    void *ctx = talloc_autofree_context();
    if (strncmp(magic, "PDCI", LEN_MAGIC) == 0) {
        char *bytes = prv_read_bytes(ctx, fp, argv[1]);
        struct pdc_image *image = pdc_image_create(bytes);
        talloc_steal(ctx, image);
        talloc_free(bytes);

        struct svg *svg = svg_create(stdout, image->viewbox.w, image->viewbox.h);
        talloc_steal(ctx, svg);

        prv_output_command_list_to_svg(&image->command_list, svg);

        svg_finish(svg);

        pdc_image_print(image, stderr);
    } else if (strncmp(magic, "PDCS", LEN_MAGIC) == 0) {
        char *bytes = prv_read_bytes(ctx, fp, argv[1]);
        struct pdc_seq *seq = pdc_seq_create(bytes);
        talloc_steal(ctx, seq);
        talloc_free(bytes);

        int16_t h = seq->viewbox.h * seq->frame_count;

        struct svg *svg = svg_create(stdout, seq->viewbox.w, h);
        talloc_steal(ctx, svg);

        for (size_t i = 0; i < seq->frame_count; i++) {
            struct svg_g *g = svg_create_g(svg, i * seq->viewbox.h);
            prv_output_command_list_to_svg(&seq->frames[i].command_list, svg);
            svg_g_finish(g);
            talloc_free(g);
        }

        svg_finish(svg);

        pdc_seq_print(seq, stderr);
    } else {
        fprintf(stderr, "Not a PDC file: %s\n", argv[1]);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    fp = NULL;

    exit(EXIT_SUCCESS);
}
