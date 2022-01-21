#include <stdint.h>

const uint8_t tex_dxt1_format[4] = {1, 10, 0, 0};
const uint8_t tex_dxt5_format[4] = {1, 12, 0, 0};

#define tex_magic "TEX"

typedef struct {
    uint8_t magic[4];
    uint16_t image_width;
    uint16_t image_height;
    uint8_t tex_format[4];
} TEX_HEADER;
