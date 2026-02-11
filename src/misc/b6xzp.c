#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* ==========================================================================
   B6X ROM ZERO PAGE UTILITY
   ========================================================================== */

#define POKE2(addr, mem, mask, value) \
    { uint16_t v = value;             \
      mem[(addr) & (mask)] = v >> 8;  \
      mem[((addr)+1) & (mask)] = v; } \

static uint8_t page[256];

static void show_usage(char **argv) {
    fprintf(stderr, "Usage: %s [flags] <input> <output>\n", argv[0]);
    fprintf(stderr, "Append B6X header to input UXN ROM.\n");
    fprintf(stderr, "Extend ROM to the nearest whole page.\n\n");

    fprintf(stderr,
        "Flags:\n"
        "  -h            Show this help message\n"
        "  -t  <title>   Title (up to 48 chars)\n"
        "  -c  <author>  Author string (up to 32 chars)\n"
        "  -v  <version> Target B6X version (16-bit HEX, default: %04x)\n"
        " [-i] <input>   Input ROM (use '-' for stdin)\n"
        " [-o] <output>  Output ROM (required)\n\n", VERSION
    );

}

int main (int argc, char **argv) {
    char *in_fname = NULL;
    char *out_fname = NULL;
    char *title = "";
    char *author = "";

    uint16_t target_ver = VERSION;

    for (int argi = 1; argi < argc; argi++) {
        if (argi + 1 >= argc) goto latest_arg;

        if (!strcmp(argv[argi], "-t")) { title     = argv[++argi]; continue; }
        if (!strcmp(argv[argi], "-c")) { author    = argv[++argi]; continue; }
        if (!strcmp(argv[argi], "-i")) { in_fname  = argv[++argi]; continue; }
        if (!strcmp(argv[argi], "-o")) { out_fname = argv[++argi]; continue; }

        if (!strcmp(argv[argi], "-v")) {
            char *endptr;
            target_ver = strtol(argv[++argi], &endptr, 16);
            if (*endptr) {
                fprintf(stderr, "ERROR: Invalid target version: %s\n",
                                                            argv[argi]);
                return 1;
            }
            continue;
        }

latest_arg:

        if (!strcmp(argv[argi], "-h")) { show_usage(argv); return 0; }

        if (!in_fname)   { in_fname = argv[argi]; continue; }
        if (!out_fname) { out_fname = argv[argi]; continue; }

        fprintf(stderr, "ERROR: Invalid argument: %s\n\n", argv[argi]);

        show_usage(argv);
        return 1;
    }


     if (!out_fname) {
        fprintf(stderr, "ERROR: Output filename is missing.\n\n");
        show_usage(argv);
        return 1;
     }

    FILE *input = stdin;
    if (in_fname && strcmp(in_fname, "-") != 0) {
        input = fopen(in_fname, "rb");
        if (!input) { perror("ERROR: Can't open input file"); return 1; }
    }

    FILE *output = fopen(out_fname, "wb");
    if (!output) {
        perror("ERROR: Can't open output file");
        if (input != stdin) fclose(input);
        return 1;
    }

    if (fwrite(page, 256, 1, output) != 1) {
        perror("Write error at page 0");
        goto error_cleanup;
    }

    uint16_t checksum = 0;
    size_t page_count = 1, bytes_read;

    while ((bytes_read = fread(page, 1, 256, input)) > 0) {
        if (page_count >= 65536) {
            fprintf(stderr, "Warning: maximum number of pages exceeded \n");
            break;
        }
        if (bytes_read < 256) memset(page+bytes_read, 0, 256-bytes_read);

        for (size_t i = 0; i < 256; i += 2)
            checksum ^= (page[i] << 8) | page[i + 1];

        if (fwrite(page, 1, 256, output) != 256) {
            fprintf(stderr, "Write error at page 0x%04lx", page_count);
            goto error_cleanup;
        }

        page_count++;
    }

    if (ferror(input)) { perror("Input file read error"); goto error_cleanup; }

    memset(page, 0, 256);

    memcpy(page, "UXNR", 4);              /* 0x00: Signature */
    memcpy(page+4, "B6X", 3);             /* 0x04: Target system */
    POKE2(14, page, 255, target_ver);     /* 0x0E: Target version */
    strncpy((char *)page+16, title, 48);  /* 0x10: Title */
    strncpy((char *)page+64, author, 32); /* 0x40: Author */
    POKE2(96, page, 255, checksum);       /* 0x60: Checksum */
    POKE2(98, page, 255, page_count);     /* 0x62: Page count */
    time_t t = time(NULL);
    memcpy(page+112, &t, sizeof(time_t)); /* 0x70: Time */

    fseek(output, 0, SEEK_SET);

    if (fwrite(page, 256, 1, output) != 1) {
        perror("Zero page overwrite error");
        goto error_cleanup;
    }

    uint8_t code = 0;
    goto cleanup;

error_cleanup:
    code = 1;
cleanup:
    if (input != stdin) fclose(input);
    if (output) fclose(output);

    return code;
}
