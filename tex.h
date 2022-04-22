#include <stdint.h>
#include <stdbool.h>

enum tex_format {
    tex_format_etc1 = 0x1,
    tex_format_etc2_eac = 0x2,
    tex_format_etc2 = 0x3,
    tex_format_dxt1 = 0xA,
    tex_format_dxt5 = 0xC,
    tex_format_rgba8 = 0x14
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
