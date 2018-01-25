#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#include "pdc.h"

static const uint8_t LEN_MAGIC = 4;

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
        pdc_image_print(image, stdout);
    } else if (strncmp(magic, "PDCS", LEN_MAGIC) == 0) {
        char *bytes = prv_read_bytes(ctx, fp, argv[1]);
        struct pdc_seq *seq = pdc_seq_create(bytes);
        talloc_steal(ctx, seq);
        talloc_free(bytes);
        pdc_seq_print(seq, stdout);
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
