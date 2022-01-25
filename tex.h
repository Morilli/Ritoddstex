#include <stdint.h>
#include <stdbool.h>

enum tex_format {
    tex_format_d32 = 0x1,
    tex_format_d24s8 = 0x2,
    tex_foramt_d24x8 = 0x3,
    tex_format_dxt1 = 0xA,
    tex_format_dxt5 = 0xC
};
#define tex_magic "TEX"

typedef struct {
    uint8_t magic[4];
    uint16_t image_width;
    uint16_t image_height;
    uint8_t unk1;
    uint8_t tex_format;
    uint8_t unk2;
    bool has_mipmaps;
} TEX_HEADER;
