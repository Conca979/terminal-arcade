/*
 * Fast C ANSI TrueColor serializer for terminal ASCII rendering.
 * Provides high-throughput run-length encoding with color delta thresholding.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

/*
 * Encode a 32-bit Unicode code point (UCS-4) into UTF-8 bytes.
 * Returns number of bytes written (1 to 4).
 */
static inline int encode_utf8(uint32_t cp, char *out) {
    if (cp == 0) {
        out[0] = ' ';
        return 1;
    }
    if (cp <= 0x7F) {
        out[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp <= 0xFFFF) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp <= 0x10FFFF) {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    out[0] = '?';
    return 1;
}

/*
 * Fast format for an 8-bit unsigned integer (0-255) into ASCII decimal digits.
 * Returns number of digits written (1 to 3).
 */
static inline int write_u8_ascii(char *p, int val) {
    if (val >= 100) {
        p[0] = (char)('0' + (val / 100));
        p[1] = (char)('0' + ((val / 10) % 10));
        p[2] = (char)('0' + (val % 10));
        return 3;
    } else if (val >= 10) {
        p[0] = (char)('0' + (val / 10));
        p[1] = (char)('0' + (val % 10));
        return 2;
    } else {
        p[0] = (char)('0' + val);
        return 1;
    }
}

/*
 * Serialize RGB pixel array and glyph array into ANSI 24-bit TrueColor string.
 *
 * Args:
 *   rgb: Contiguous uint8 array of shape [height, width, 3].
 *   chars: Contiguous uint32 array of shape [height, width] (UCS-4 codepoints).
 *   height: Frame rows.
 *   width: Frame columns.
 *   threshold: Color delta threshold (0 = exact match, >0 = squared Euclidean threshold).
 *   out: Pre-allocated output buffer.
 *   max_len: Total byte capacity of out buffer.
 *
 * Returns:
 *   Number of bytes written, or -1 if buffer was insufficient.
 */
EXPORT int serialize_truecolor(
    const uint8_t *rgb,
    const uint32_t *chars,
    int height,
    int width,
    int threshold,
    char *out,
    int max_len
) {
    if (!rgb || !chars || !out || height <= 0 || width <= 0 || max_len < 64) {
        return -1;
    }

    char *p = out;
    char *end = out + max_len - 64; /* Reserve margin for trailer */
    int thresh_sq = threshold * threshold;

    for (int r = 0; r < height; r++) {
        const uint8_t *row_rgb = rgb + (size_t)r * (size_t)width * 3;
        const uint32_t *row_chars = chars + (size_t)r * (size_t)width;

        /* Reset color state at the beginning of each line */
        int last_r = -1000;
        int last_g = -1000;
        int last_b = -1000;

        for (int c = 0; c < width; c++) {
            if (p >= end) {
                return -1; /* Buffer overflow guard */
            }

            int cr = row_rgb[c * 3 + 0];
            int cg = row_rgb[c * 3 + 1];
            int cb = row_rgb[c * 3 + 2];

            int dr = cr - last_r;
            int dg = cg - last_g;
            int db = cb - last_b;

            int change = (c == 0);
            if (!change) {
                if (threshold <= 0) {
                    change = (dr != 0 || dg != 0 || db != 0);
                } else {
                    change = (dr * dr + dg * dg + db * db > thresh_sq);
                }
            }

            if (change) {
                /* Write ANSI TrueColor sequence: \x1b[38;2;R;G;Bm */
                p[0] = '\x1b';
                p[1] = '[';
                p[2] = '3';
                p[3] = '8';
                p[4] = ';';
                p[5] = '2';
                p[6] = ';';
                p += 7;

                p += write_u8_ascii(p, cr);
                *p++ = ';';
                p += write_u8_ascii(p, cg);
                *p++ = ';';
                p += write_u8_ascii(p, cb);
                *p++ = 'm';

                last_r = cr;
                last_g = cg;
                last_b = cb;
            }

            /* Write character glyph (1 to 4 UTF-8 bytes) */
            p += encode_utf8(row_chars[c], p);
        }

        /* Append line-clear escape sequence: \x1b[K */
        p[0] = '\x1b';
        p[1] = '[';
        p[2] = 'K';
        p += 3;

        /* Append newline between lines */
        if (r < height - 1) {
            *p++ = '\n';
        }
    }

    /* Append terminal reset and line clear: \x1b[0m\x1b[K */
    p[0] = '\x1b';
    p[1] = '[';
    p[2] = '0';
    p[3] = 'm';
    p[4] = '\x1b';
    p[5] = '[';
    p[6] = 'K';
    p += 7;

    *p = '\0';
    return (int)(p - out);
}
