/* A small PNG writer for --screenshot: RGB, stored (uncompressed) deflate
 * blocks, so it needs no zlib. Files are about 1.2MB each. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

static uint32_t crc_table[256];

static void crc_init(void)
{
    for(uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for(int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}

static uint32_t crc(uint32_t c, const uint8_t * buf, size_t len)
{
    c ^= 0xFFFFFFFFu;
    for(size_t i = 0; i < len; i++) c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static void put32(uint8_t * p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void chunk(FILE * f, const char * tag, const uint8_t * data, uint32_t len)
{
    uint8_t hdr[8];
    put32(hdr, len);
    memcpy(hdr + 4, tag, 4);
    fwrite(hdr, 1, 8, f);
    if(len) fwrite(data, 1, len, f);
    uint32_t c = crc(0, (const uint8_t *)tag, 4);
    c = crc(c, data, len);
    uint8_t t[4];
    put32(t, c);
    fwrite(t, 1, 4, f);
}

bool platform_write_png(lv_display_t * disp, const char * path)
{
    lv_draw_buf_t * buf = lv_display_get_buf_active(disp);
    if(!buf || !buf->data) return false;
    uint32_t w = buf->header.w, h = buf->header.h, stride = buf->header.stride;
    uint32_t bpp = lv_color_format_get_size(buf->header.cf);
    if(bpp != 4) {
        fprintf(stderr, "keel: screenshot expects a 32-bit frame buffer\n");
        return false;
    }
    if(!crc_table[1]) crc_init();

    /* Raw scanlines: filter byte + RGB. */
    size_t raw_len = (size_t)h * (1 + w * 3);
    uint8_t * raw = malloc(raw_len);
    for(uint32_t y = 0; y < h; y++) {
        uint8_t * o = raw + (size_t)y * (1 + w * 3);
        const uint8_t * s = buf->data + (size_t)y * stride;
        *o++ = 0;
        for(uint32_t x = 0; x < w; x++, s += 4) {
            *o++ = s[2]; /* memory order is B G R X */
            *o++ = s[1];
            *o++ = s[0];
        }
    }

    /* zlib stream of stored blocks, then Adler-32. */
    size_t nblocks = (raw_len + 65534) / 65535;
    size_t z_len = 2 + raw_len + nblocks * 5 + 4;
    uint8_t * z = malloc(z_len);
    uint8_t * o = z;
    *o++ = 0x78;
    *o++ = 0x01;
    uint32_t a = 1, b = 0;
    for(size_t off = 0; off < raw_len; off += 65535) {
        uint16_t n = (uint16_t)LV_MIN(65535, raw_len - off);
        *o++ = off + n >= raw_len ? 1 : 0;
        *o++ = (uint8_t)n;
        *o++ = (uint8_t)(n >> 8);
        *o++ = (uint8_t)~n;
        *o++ = (uint8_t)(~n >> 8);
        memcpy(o, raw + off, n);
        o += n;
    }
    for(size_t i = 0; i < raw_len; i++) {
        a = (a + raw[i]) % 65521;
        b = (b + a) % 65521;
    }
    put32(o, (b << 16) | a);

    FILE * f = fopen(path, "wb");
    if(!f) {
        free(raw);
        free(z);
        return false;
    }
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13];
    put32(ihdr, w);
    put32(ihdr + 4, h);
    ihdr[8] = 8;  /* bit depth */
    ihdr[9] = 2;  /* RGB */
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, (uint32_t)z_len);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(z);
    return true;
}
