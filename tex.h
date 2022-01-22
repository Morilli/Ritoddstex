#include <stdint.h>
#include <stdbool.h>

const uint8_t tex_dxt1_format[2] = {1, 10};
const uint8_t tex_dxt5_format[2] = {1, 12};

#define tex_magic "TEX"

typedef struct {
    uint8_t magic[4];
    uint16_t image_width;
    uint16_t image_height;
    uint8_t tex_format[2];
    uint8_t unk1;
    bool has_mipmaps;
} TEX_HEADER;
