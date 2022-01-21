#include <stdint.h>

// structs taken from microsoft docs at https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
// defines stolen from https://github.com/Microsoft/DirectXTex/blob/main/DirectXTex/DDS.h

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE

#define dds_magic ( (uint8_t[4]) {'D', 'D', 'S', ' '} )

typedef struct {
  uint32_t dwSize;
  uint32_t dwFlags;
  uint8_t  dwFourCC[4];
  uint32_t dwRGBBitCount;
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;
  uint32_t dwABitMask;
} DDS_PIXELFORMAT;

typedef struct {
  uint32_t        dwSize;
  uint32_t        dwFlags;
  uint32_t        dwHeight;
  uint32_t        dwWidth;
  uint32_t        dwPitchOrLinearSize;
  uint32_t        dwDepth;
  uint32_t        dwMipMapCount;
  uint32_t        dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32_t        dwCaps;
  uint32_t        dwCaps2;
  uint32_t        dwCaps3;
  uint32_t        dwCaps4;
  uint32_t        dwReserved2;
} DDS_HEADER;
