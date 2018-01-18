#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

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

static void prv_read_pdc(struct pdc *pdc, char *bytes) {
    memcpy(pdc, &bytes[0], 9);
    pdc->points = malloc(sizeof(struct point) * pdc->num_points);
    for (size_t i = 0; i < pdc->num_points; i++) {
        prv_read_point(&pdc->points[i], bytes + 9 + (4 * i));
    }
}

static void prv_read_pdc_list(struct pdc_list *list, char *bytes) {
    memcpy(&list->num_commands, &bytes[0], sizeof(uint16_t));
    list->commands = malloc(sizeof(struct pdc) * list->num_commands);
    size_t offset = 2;
    for (size_t i = 0; i < list->num_commands; i++) {
        prv_read_pdc(&list->commands[i], bytes + offset);
        offset += 9 + (sizeof(struct point) * list->commands[i].num_points);
    }
}

static inline void prv_read_viewbox(struct viewbox *viewbox, char *bytes) {
    memcpy(&viewbox->w, &bytes[0], sizeof(int16_t));
    memcpy(&viewbox->h, &bytes[2], sizeof(int16_t));
}

static void prv_read_image(char *bytes) {
    struct pdc_image image;
    image.version = (uint8_t) bytes[0];
    prv_read_viewbox(&image.viewbox, bytes + 2);

    printf("%d %d %d\n", image.version, image.viewbox.w, image.viewbox.h);

    prv_read_pdc_list(&image.command_list, bytes + 6);
    printf("%d\n", image.command_list.num_commands);

    for (size_t i = 0; i < image.command_list.num_commands; i++) {
        struct pdc command = image.command_list.commands[i];
        printf("%ld:\t%d %d\n", i, command.type, command.num_points);
        for (size_t j = 0; j < command.num_points; j++) {
            printf("\t\t%d %d\n", command.points[j].x, command.points[j].y);
        }

        free(command.points);
    }
    free(image.command_list.commands);
}

static void prv_read_seq(char *bytes) {}

static char *prv_read_bytes(FILE *fp, char *name) {
    uint32_t size;
    if (fread(&size, sizeof(uint32_t), 1, fp) != 1) {
        fprintf(stderr, "Error reading file: %s\n", name);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    char *bytes = malloc(size);
    if (fread(bytes, 1, size, fp) != size) {
        fprintf(stderr, "Error reading file: %s\n", name);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    return bytes;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: pdc-reader PDC\n");
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

    char *bytes = NULL;
    if (strncmp(magic, "PDCI", LEN_MAGIC) == 0) {
        bytes = prv_read_bytes(fp, argv[1]);
        prv_read_image(bytes);
    } else if (strncmp(magic, "PDCS", LEN_MAGIC) == 0) {
        bytes = prv_read_bytes(fp, argv[1]);
        prv_read_seq(bytes);
    } else {
        fprintf(stderr, "Not a PDC file: %s\n", argv[1]);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    free(bytes);

    fclose(fp);
    fp = NULL;

    exit(EXIT_SUCCESS);
}
