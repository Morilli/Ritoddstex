#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "dds.h"
#include "tex.h"

int input_path_length;

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

    const uint8_t* tex_format;
    if (memcmp(dds_header.ddspf.dwFourCC, "DXT1", 4) == 0) {
        tex_format = tex_dxt1_format;
    } else if (memcmp(dds_header.ddspf.dwFourCC, "DXT5", 4) == 0) {
        tex_format = tex_dxt5_format;
    } else {
        fprintf(stderr, "Error: dds file needs to be in either DXT1 or DXT5 format!\n");
        fclose(dds_file);
        return;
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
    TEX_HEADER tex_header = {
        .magic = tex_magic,
        .image_width = dds_header.dwWidth,
        .image_height = dds_header.dwHeight,
    };
    memcpy(tex_header.tex_format, tex_format, 4);
    fwrite(&tex_header, sizeof(TEX_HEADER), 1, tex_file);

    printf("Info: Converting %ux%u DDS file \"%s\" to TEX file \"%s\".\n", tex_header.image_width, tex_header.image_height, dds_path, tex_path);

    // the raw dds image data
    uint8_t* data_buffer = malloc(file_size - sizeof(DDS_HEADER) - 4);
    assert(fread(data_buffer, 1, file_size - sizeof(DDS_HEADER) - 4, dds_file) == file_size - sizeof(DDS_HEADER) - 4);
    fwrite(data_buffer, 1, file_size - sizeof(DDS_HEADER) - 4, tex_file);
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

    const char* dds_format;
    if (memcmp(tex_header.tex_format, tex_dxt1_format, 4) == 0) {
        dds_format = "DXT1";
    } else if (memcmp(tex_header.tex_format, tex_dxt5_format, 4) == 0) {
        dds_format = "DXT5";
    } else {
        fprintf(stderr, "Error: tex file needs to be in either DXT1 or DXT5 format!\n");
        fclose(tex_file);
        return;
    }

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
        .dwFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE,
        .dwHeight = tex_header.image_height,
        .dwWidth = tex_header.image_width,
        .dwPitchOrLinearSize = tex_header.image_width * tex_header.image_height,
        .ddspf = {
            .dwSize = sizeof(DDS_PIXELFORMAT),
            .dwFlags = DDS_FOURCC
        },
        .dwCaps = DDS_SURFACE_FLAGS_TEXTURE
    };
    memcpy(dds_header.ddspf.dwFourCC, dds_format, 4);
    fwrite(dds_magic, 1, 4, dds_file);
    fwrite(&dds_header, sizeof(DDS_HEADER), 1, dds_file);

    printf("Info: Converting %ux%u TEX file \"%s\" to DDS file \"%s\".\n", tex_header.image_width, tex_header.image_height, tex_path, dds_path);

    // the raw dds image data
    uint8_t* data_buffer = malloc(file_size - sizeof(TEX_HEADER));
    assert(fread(data_buffer, 1, file_size - sizeof(TEX_HEADER), tex_file) == file_size - sizeof(TEX_HEADER));
    fwrite(data_buffer, 1, file_size - sizeof(TEX_HEADER), dds_file);
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
