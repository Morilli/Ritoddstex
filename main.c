#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "dds.h"
#include "tex.h"

int input_path_length;

#ifdef max
#undef max
#endif
#define max(a, b) __extension__ ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b; \
})

int get_bytes_per_block(uint8_t format)
{
    switch (format)
    {
        case tex_format_dxt1: return 8;
        case tex_format_dxt5: return 16;
        case tex_format_rgba8: return 4;
    }
    __builtin_unreachable();
}

int get_block_size(uint8_t format)
{
    switch (format)
    {
        case tex_format_dxt1:
        case tex_format_dxt5:
            return 4;
        case tex_format_rgba8:
            return 1;
    }
    __builtin_unreachable();
}


void dds2tex(const char* dds_path)
{
    FILE* dds_file = fopen(dds_path, "rb");
    if (!dds_file) {
        fprintf(stderr, "Error: Failed to open input dds file \"%s\"\n", dds_path);
        return;
    }

    fseek(dds_file, 0, SEEK_END);
    uint32_t file_size = ftell(dds_file);
    rewind(dds_file);

    char magic[4];
    if (fread(magic, 1, 4, dds_file) != 4
     || file_size < sizeof(DDS_HEADER) + 4 || memcmp(magic, dds_magic, 4) != 0) {
        fprintf(stderr, "Error: Not a valid dds file!\n");
        fclose(dds_file);
        return;
    }

    DDS_HEADER dds_header;
    assert(fread(&dds_header, sizeof(DDS_HEADER), 1, dds_file) == 1);

    TEX_HEADER tex_header = {
        .magic = tex_magic,
        .image_width = dds_header.dwWidth,
        .image_height = dds_header.dwHeight,
        .unk1 = 1 // this is always set to 1 in original tex files; no idea what it stands for
    };
    if (memcmp(dds_header.ddspf.dwFourCC, "DXT1", 4) == 0) {
        tex_header.tex_format = tex_format_dxt1;
    } else if (memcmp(dds_header.ddspf.dwFourCC, "DXT5", 4) == 0) {
        tex_header.tex_format = tex_format_dxt5;
    } else if ((dds_header.ddspf.dwFlags & DDS_RGBA) == DDS_RGBA) {
        // sanity checks; probably not strictly necessary but I don't wanna handle anything else
        if (dds_header.ddspf.dwRGBBitCount != 32
         || dds_header.ddspf.dwRBitMask != 0x000000ff
         || dds_header.ddspf.dwGBitMask != 0x0000ff00
         || dds_header.ddspf.dwBBitMask != 0x00ff0000
         || dds_header.ddspf.dwABitMask != 0xff000000) {
            fprintf(stderr, "Error: The File is not in exact RGBA8 format.\n");
            fclose(dds_file);
            return;
        }
        tex_header.tex_format = tex_format_rgba8;
    } else {
        fprintf(stderr, "Error: dds file needs to be in either DXT1, DXT5 or uncompressed RGBA8 format!\n");
        fclose(dds_file);
        return;
    }
    if (dds_header.dwMipMapCount > 1) { // this value may be set to 1, which is equivalent to leaving it at 0 (no mipmaps)
        tex_header.has_mipmaps = true;
        if (dds_header.dwMipMapCount != 32u - __builtin_clz(max(dds_header.dwWidth, dds_header.dwHeight))) {
            fprintf(stderr, "Error: DDS mipmap count mismatch; expected %u mipmaps, got %u\n", 32 - __builtin_clz(max(dds_header.dwWidth, dds_header.dwHeight)), dds_header.dwMipMapCount);
            fclose(dds_file);
            return;
        } else {
            printf("Info: DDS file has mipmaps\n");
        }
    }

    char tex_path[input_path_length];
    memcpy(tex_path, dds_path, input_path_length);
    memcpy(tex_path + input_path_length - 4, ".tex", 5);
    FILE* tex_file = fopen(tex_path, "wb");
    if (!tex_file) {
        fprintf(stderr, "Error: Failed to open output tex file \"%s\"\n", tex_path);
        fclose(dds_file);
        return;
    }
    fwrite(&tex_header, sizeof(TEX_HEADER), 1, tex_file);

    printf("Info: Converting %ux%u DDS file \"%s\" to TEX file \"%s\".\n", tex_header.image_width, tex_header.image_height, dds_path, tex_path);

    // the raw dds image data
    uint8_t* data_buffer = malloc(file_size - sizeof(DDS_HEADER) - 4);
    assert(fread(data_buffer, 1, file_size - sizeof(DDS_HEADER) - 4, dds_file) == file_size - sizeof(DDS_HEADER) - 4);
    if (tex_header.has_mipmaps) {
        int32_t current_offset = file_size - sizeof(DDS_HEADER) - 4;
        printf("Writing %u mipmaps to TEX file...\n", dds_header.dwMipMapCount);
        const uint32_t block_size = get_block_size(tex_header.tex_format);
        const uint32_t bytes_per_block = get_bytes_per_block(tex_header.tex_format);
        for (int32_t i = dds_header.dwMipMapCount-1; i >= 0; i--) {
            uint32_t current_image_width = max(tex_header.image_width / (1 << i), 1);
            uint32_t current_image_height = max(tex_header.image_height / (1 << i), 1);
            uint32_t current_image_size = max(bytes_per_block * current_image_width * current_image_height / (block_size * block_size), bytes_per_block);
            printf("Writing mipmap %u with size %ux%u\n", i, current_image_width, current_image_height);
            current_offset -= current_image_size;
            if (current_offset < 0) {
                fprintf(stderr, "Error when attempting to write mipmap %u: Not enough data to read mipmap!\n", i);
                break;
            }
            fwrite(data_buffer + current_offset, 1, current_image_size, tex_file);
        }
    } else {
        fwrite(data_buffer, 1, file_size - sizeof(DDS_HEADER) - 4, tex_file);
    }
    free(data_buffer);

    fclose(dds_file);
    fclose(tex_file);
}

void tex2dds(const char* tex_path)
{
    FILE* tex_file = fopen(tex_path, "rb");
    if (!tex_file) {
        fprintf(stderr, "Error: Failed to open input tex file \"%s\"\n", tex_path);
        return;
    }

    fseek(tex_file, 0, SEEK_END);
    uint32_t file_size = ftell(tex_file);
    rewind(tex_file);

    TEX_HEADER tex_header;
    if (fread(tex_header.magic, 1, 4, tex_file) != 4
     || file_size < sizeof(TEX_HEADER) || memcmp(tex_header.magic, tex_magic, 4) != 0) {
        fprintf(stderr, "Error: Not a valid tex file!\n");
        fclose(tex_file);
        return;
    }

    assert(fread((uint8_t*) &tex_header + 4, sizeof(TEX_HEADER) - 4, 1, tex_file) == 1);

    DDS_PIXELFORMAT ddspf = {.dwSize = sizeof(DDS_PIXELFORMAT)};
    const char* dds_format;
    switch (tex_header.tex_format)
    {
        case tex_format_dxt1: dds_format = "DXT1"; ddspf.dwFlags = DDS_FOURCC; break;
        case tex_format_dxt5: dds_format = "DXT5"; ddspf.dwFlags = DDS_FOURCC; break;
        case tex_format_rgba8:
            dds_format = "\0\0\0"; // no format, just uncompressed RGBA8
            ddspf.dwFlags = DDS_RGBA;
            ddspf.dwRGBBitCount = 8 * 4;
            ddspf.dwRBitMask = 0x000000ff;
            ddspf.dwGBitMask = 0x0000ff00;
            ddspf.dwBBitMask = 0x00ff0000;
            ddspf.dwABitMask = 0xff000000;
            break;
        default:
            fprintf(stderr, "Error: tex file needs to be in either DXT1, DXT5 or RGBA8 format!\n");
            fclose(tex_file);
            return;
    }
    memcpy(ddspf.dwFourCC, dds_format, 4);

    char dds_path[input_path_length];
    memcpy(dds_path, tex_path, input_path_length);
    memcpy(dds_path + input_path_length - 4, ".dds", 5);
    FILE* dds_file = fopen(dds_path, "wb");
    if (!dds_file) {
        fprintf(stderr, "Error: Failed to open output dds file \"%s\"\n", dds_path);
        fclose(tex_file);
        return;
    }
    DDS_HEADER dds_header = {
        .dwSize = sizeof(DDS_HEADER),
        .dwFlags = DDS_HEADER_FLAGS_TEXTURE,
        .dwHeight = tex_header.image_height,
        .dwWidth = tex_header.image_width,
        .ddspf = ddspf,
        .dwCaps = DDS_SURFACE_FLAGS_TEXTURE
    };
    if (tex_header.has_mipmaps) {
        printf("Info: TEX file has mipmaps\n");
        dds_header.dwFlags |= DDS_HEADER_FLAGS_MIPMAP;
        dds_header.dwCaps |= DDS_SURFACE_FLAGS_MIPMAP;
        dds_header.dwMipMapCount = 32 - __builtin_clz(max(tex_header.image_width, tex_header.image_height));
    }
    fwrite(dds_magic, 1, 4, dds_file);
    fwrite(&dds_header, sizeof(DDS_HEADER), 1, dds_file);

    printf("Info: Converting %ux%u TEX file \"%s\" to DDS file \"%s\".\n", tex_header.image_width, tex_header.image_height, tex_path, dds_path);

    // the raw dds image data
    uint8_t* data_buffer = malloc(file_size - sizeof(TEX_HEADER));
    assert(fread(data_buffer, 1, file_size - sizeof(TEX_HEADER), tex_file) == file_size - sizeof(TEX_HEADER));
    if (tex_header.has_mipmaps) {
        int32_t current_offset = file_size - sizeof(TEX_HEADER);
        printf("Writing %u mipmaps to DDS file...\n", dds_header.dwMipMapCount);
        const uint32_t block_size = get_block_size(tex_header.tex_format);
        const uint32_t bytes_per_block = get_bytes_per_block(tex_header.tex_format);
        for (uint32_t i = 0; i < dds_header.dwMipMapCount; i++) {
            uint32_t current_image_width = max(tex_header.image_width / (1 << i), 1);
            uint32_t current_image_height = max(tex_header.image_height / (1 << i), 1);
            uint32_t current_image_size = max(bytes_per_block * current_image_width * current_image_height / (block_size * block_size), bytes_per_block);
            printf("Writing mipmap %u with size %ux%u\n", i, current_image_width, current_image_height);
            current_offset -= current_image_size;
            if (current_offset < 0) {
                fprintf(stderr, "Error when attempting to write mipmap %u: Not enough data to read mipmap!\n", i);
                break;
            }
            fwrite(data_buffer + current_offset, 1, current_image_size, dds_file);
        }
    } else {
        fwrite(data_buffer, 1, file_size - sizeof(TEX_HEADER), dds_file);
    }
    free(data_buffer);

    fclose(tex_file);
    fclose(dds_file);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Error: Provide at least one input file!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        input_path_length = strlen(argv[i]);
        if (input_path_length > 4) {
            if (strcmp(argv[i] + input_path_length - 4, ".dds") == 0) {
                dds2tex(argv[i]);
            } else if (strcmp(argv[i] + input_path_length - 4, ".tex") == 0) {
                tex2dds(argv[i]);
            } else {
                fprintf(stderr, "Error: \"%s\" is neither a .dds or .tex file!\n", argv[i]);
                continue;
            }
        }
    }
}
