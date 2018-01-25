#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

static const uint8_t LEN_MAGIC = 4;

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

static void prv_print_pdc(struct pdc *p) {
    printf("%d %d\n", p->type, p->num_points);
    for (size_t i = 0; i < p->num_points; i++) {
        printf("\t\t%d,%d\n", p->points[i].x, p->points[i].y);
    }
}

static struct pdc_image *prv_read_image(char *bytes) {
    struct pdc_image *image = talloc(bytes, struct pdc_image);
    image->version = (uint8_t) bytes[0];
    prv_read_viewbox(&image->viewbox, bytes + 2);
    prv_read_pdc_list(image, &image->command_list, bytes + 6);
    return image;
}

static void prv_print_image(struct pdc_image *image) {
    printf("%d %d %d\n", image->version, image->viewbox.w, image->viewbox.h);
    printf("%d\n", image->command_list.num_commands);

    for (size_t i = 0; i < image->command_list.num_commands; i++) {
        struct pdc command = image->command_list.commands[i];
        printf("\tpdc %ld: ", i);
        prv_print_pdc(&command);
    }
}

static struct pdc_seq *prv_read_seq(char *bytes) {
    struct pdc_seq *seq = talloc(bytes, struct pdc_seq);
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

static void prv_print_seq(struct pdc_seq *seq) {
    printf("%d %d %d\n", seq->version, seq->viewbox.w, seq->viewbox.h);
    printf("%d %d\n", seq->play_count, seq->frame_count);

    for (size_t i = 0; i < seq->frame_count; i++) {
        struct pdc_frame frame = seq->frames[i];
        printf("frame %ld: %d\n", i, frame.duration);
        for (size_t j = 0; j < frame.command_list.num_commands; j++) {
            struct pdc command = frame.command_list.commands[j];
            printf("\tpdc %ld: ", j);
            prv_print_pdc(&command);
        }
    }
}

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s PDC\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *fp;
    fp = fopen(argv[1], "r");
    
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
    char *bytes = NULL;
    if (strncmp(magic, "PDCI", LEN_MAGIC) == 0) {
        bytes = prv_read_bytes(ctx, fp, argv[1]);
        struct pdc_image *image = prv_read_image(bytes);
        prv_print_image(image);
    } else if (strncmp(magic, "PDCS", LEN_MAGIC) == 0) {
        bytes = prv_read_bytes(ctx, fp, argv[1]);
        struct pdc_seq *seq = prv_read_seq(bytes);
        prv_print_seq(seq);
    } else {
        fprintf(stderr, "Not a PDC file: %s\n", argv[1]);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    talloc_report_full(ctx, stderr);

    fclose(fp);
    fp = NULL;

    exit(EXIT_SUCCESS);
}
