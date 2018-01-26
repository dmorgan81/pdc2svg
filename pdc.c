#include <string.h>
#include <talloc.h>

#include "color.h"
#include "pdc.h"

static inline void prv_read_point(struct point *point, char *bytes) {
    memcpy(&point->x, &bytes[0], sizeof(int16_t));
    memcpy(&point->y, &bytes[2], sizeof(int16_t));
}

static size_t prv_read_pdc(void *ctx, struct pdc *pdc, char *bytes) {
    size_t size = sizeof(struct pdc) - sizeof(struct point*);
    memcpy(pdc, &bytes[0], size);
    pdc->points = talloc_array(ctx, struct point, pdc->num_points);
    for (size_t i = 0; i < pdc->num_points; i++) {
        prv_read_point(&pdc->points[i], bytes + size + (sizeof(struct point) * i));
    }
    return size + (sizeof(struct point) * pdc->num_points);
}

static size_t prv_read_pdc_list(void *ctx, struct pdc_list *list, char *bytes) {
    memcpy(&list->num_commands, &bytes[0], sizeof(uint16_t));
    list->commands = talloc_array(ctx, struct pdc, list->num_commands);
    size_t offset = 2;
    for (size_t i = 0; i < list->num_commands; i++) {
        offset += prv_read_pdc(list->commands, &list->commands[i], bytes + offset);
    }
    return (sizeof(struct pdc_list) - sizeof(struct pdc*)) + (offset - 2);
}

static size_t prv_read_pdc_frame(void *ctx, struct pdc_frame *frame, char *bytes) {
    memcpy(&frame->duration, &bytes[0], sizeof(uint16_t));
    size_t size = sizeof(struct pdc_frame) - sizeof(struct pdc_list);
    return size + prv_read_pdc_list(ctx, &frame->command_list, bytes + 2);
}

static inline void prv_read_viewbox(struct viewbox *viewbox, char *bytes) {
    memcpy(&viewbox->w, &bytes[0], sizeof(int16_t));
    memcpy(&viewbox->h, &bytes[2], sizeof(int16_t));
}

static void prv_print_pdc(struct pdc *p, FILE *stream, uint8_t indent) {
    fprintf(stream, "t:%d r:%d n:%d sw:%d p:%d\n", p->type, p->path_open_or_radius, p->num_points, p->stroke_width, p->num_points);
    for (size_t i = 0; i < indent - 1; i++) fprintf(stream, "\t");
    fprintf(stream, "sc:");
    print_color_to_rgba(stream, p->stroke_color);
    fprintf(stream, " fc:");
    print_color_to_rgba(stream, p->fill_color);
    fprintf(stream, "\n");

    for (size_t i = 0; i < p->num_points; i++) {
        for (size_t j = 0; j < indent; j++) fprintf(stream, "\t");
        fprintf(stream, "x:%d,y:%d\n", p->points[i].x, p->points[i].y);
    }
}

struct pdc_image *pdc_image_create(char *bytes) {
    struct pdc_image *image = talloc(NULL, struct pdc_image);
    image->version = (uint8_t) bytes[0];
    prv_read_viewbox(&image->viewbox, bytes + 2);
    prv_read_pdc_list(image, &image->command_list, bytes + 6);
    return image;
}

void pdc_image_print(struct pdc_image *image, FILE *stream) {
    fprintf(stream, "v:%d w:%d h:%d n:%d\n", image->version, image->viewbox.w, image->viewbox.h, image->command_list.num_commands);

    for (size_t i = 0; i < image->command_list.num_commands; i++) {
        struct pdc command = image->command_list.commands[i];
        fprintf(stream, "\tpdc %ld: ", i);
        prv_print_pdc(&command, stream, 2);
    }
}

struct pdc_seq *pdc_seq_create(char *bytes) {
    struct pdc_seq *seq = talloc(NULL, struct pdc_seq);
    seq->version = (uint8_t) bytes[0];
    prv_read_viewbox(&seq->viewbox, bytes + 2);
    memcpy(&seq->play_count, bytes + 6, sizeof(int16_t));
    memcpy(&seq->frame_count, bytes + 8, sizeof(int16_t));

    seq->frames = talloc_array(seq, struct pdc_frame, seq->frame_count);

    size_t offset = 10;
    for (size_t i = 0; i < seq->frame_count; i++) {
        offset += prv_read_pdc_frame(seq->frames, &seq->frames[i], bytes + offset);
    }

    return seq;
}

void pdc_seq_print(struct pdc_seq *seq, FILE *stream) {
    fprintf(stream, "v:%d w:%d h:%d\n", seq->version, seq->viewbox.w, seq->viewbox.h);
    fprintf(stream, "p:%d f:%d\n", seq->play_count, seq->frame_count);

    for (size_t i = 0; i < seq->frame_count; i++) {
        struct pdc_frame frame = seq->frames[i];
        fprintf(stream, "\tframe %ld: d:%d c:%d\n", i, frame.duration, frame.command_list.num_commands);
        for (size_t j = 0; j < frame.command_list.num_commands; j++) {
            struct pdc command = frame.command_list.commands[j];
            fprintf(stream, "\t\tpdc %ld: ", j);
            prv_print_pdc(&command, stream, 3);
        }
    }
}