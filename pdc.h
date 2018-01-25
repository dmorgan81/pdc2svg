#ifndef _PDC_H_
#define _PDC_H_

#include <stdint.h>

struct point {
    int16_t x;
    int16_t y;
};

struct viewbox {
    int16_t w;
    int16_t h;
};

struct __attribute__((packed)) pdc {
    uint8_t type;
    uint8_t flags;
    int8_t stroke_color;
    uint8_t stroke_width;
    int8_t fill_color;
    union {
        uint8_t path_type;
        uint16_t radius;
    } path_type_or_radius;
    uint16_t num_points;
    struct point *points;
};

struct __attribute__((packed)) pdc_list {
    uint16_t num_commands;
    struct pdc *commands;
};

struct __attribute__((packed)) pdc_frame {
    uint16_t duration;
    struct pdc_list command_list;
};

struct __attribute__((packed)) pdc_image {
    uint8_t version;
    uint8_t reserved;
    struct viewbox viewbox;
    struct pdc_list command_list;
};

struct __attribute__((packed)) pdc_seq {
    uint8_t version;
    uint8_t reserved;
    struct viewbox viewbox;
    int16_t play_count;
    int16_t frame_count;
    struct pdc_frame *frames;
};
#endif

struct pdc_image *pdc_image_create(char *bytes);
void pdc_image_print(struct pdc_image *image, FILE *stream);

struct pdc_seq *pdc_seq_create(char *bytes);
void pdc_seq_print(struct pdc_seq *seq, FILE *stream);
