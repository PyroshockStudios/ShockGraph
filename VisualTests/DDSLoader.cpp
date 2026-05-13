#include "DDSLoader.hpp"
#include <fstream>

namespace VisualTests {
    static constexpr uint32_t DDS_MAGIC = 0x20534444; // "DDS "

    static constexpr uint32_t MakeFourCC(char ch0, char ch1, char ch2, char ch3) {
        return ((uint32_t)(uint8_t)(ch0)) |
               ((uint32_t)(uint8_t)(ch1) << 8) |
               ((uint32_t)(uint8_t)(ch2) << 16) |
               ((uint32_t)(uint8_t)(ch3) << 24);
    }
    typedef enum DXGI_FORMAT {
        DXGI_FORMAT_UNKNOWN = 0,
        DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
        DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
        DXGI_FORMAT_R32G32B32A32_UINT = 3,
        DXGI_FORMAT_R32G32B32A32_SINT = 4,
        DXGI_FORMAT_R32G32B32_TYPELESS = 5,
        DXGI_FORMAT_R32G32B32_FLOAT = 6,
        DXGI_FORMAT_R32G32B32_UINT = 7,
        DXGI_FORMAT_R32G32B32_SINT = 8,
        DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
        DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
        DXGI_FORMAT_R16G16B16A16_UNORM = 11,
        DXGI_FORMAT_R16G16B16A16_UINT = 12,
        DXGI_FORMAT_R16G16B16A16_SNORM = 13,
        DXGI_FORMAT_R16G16B16A16_SINT = 14,
        DXGI_FORMAT_R32G32_TYPELESS = 15,
        DXGI_FORMAT_R32G32_FLOAT = 16,
        DXGI_FORMAT_R32G32_UINT = 17,
        DXGI_FORMAT_R32G32_SINT = 18,
        DXGI_FORMAT_R32G8X24_TYPELESS = 19,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
        DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
        DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
        DXGI_FORMAT_R10G10B10A2_UNORM = 24,
        DXGI_FORMAT_R10G10B10A2_UINT = 25,
        DXGI_FORMAT_R11G11B10_FLOAT = 26,
        DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
        DXGI_FORMAT_R8G8B8A8_UNORM = 28,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
        DXGI_FORMAT_R8G8B8A8_UINT = 30,
        DXGI_FORMAT_R8G8B8A8_SNORM = 31,
        DXGI_FORMAT_R8G8B8A8_SINT = 32,
        DXGI_FORMAT_R16G16_TYPELESS = 33,
        DXGI_FORMAT_R16G16_FLOAT = 34,
        DXGI_FORMAT_R16G16_UNORM = 35,
        DXGI_FORMAT_R16G16_UINT = 36,
        DXGI_FORMAT_R16G16_SNORM = 37,
        DXGI_FORMAT_R16G16_SINT = 38,
        DXGI_FORMAT_R32_TYPELESS = 39,
        DXGI_FORMAT_D32_FLOAT = 40,
        DXGI_FORMAT_R32_FLOAT = 41,
        DXGI_FORMAT_R32_UINT = 42,
        DXGI_FORMAT_R32_SINT = 43,
        DXGI_FORMAT_R24G8_TYPELESS = 44,
        DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
        DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
        DXGI_FORMAT_R8G8_TYPELESS = 48,
        DXGI_FORMAT_R8G8_UNORM = 49,
        DXGI_FORMAT_R8G8_UINT = 50,
        DXGI_FORMAT_R8G8_SNORM = 51,
        DXGI_FORMAT_R8G8_SINT = 52,
        DXGI_FORMAT_R16_TYPELESS = 53,
        DXGI_FORMAT_R16_FLOAT = 54,
        DXGI_FORMAT_D16_UNORM = 55,
        DXGI_FORMAT_R16_UNORM = 56,
        DXGI_FORMAT_R16_UINT = 57,
        DXGI_FORMAT_R16_SNORM = 58,
        DXGI_FORMAT_R16_SINT = 59,
        DXGI_FORMAT_R8_TYPELESS = 60,
        DXGI_FORMAT_R8_UNORM = 61,
        DXGI_FORMAT_R8_UINT = 62,
        DXGI_FORMAT_R8_SNORM = 63,
        DXGI_FORMAT_R8_SINT = 64,
        DXGI_FORMAT_A8_UNORM = 65,
        DXGI_FORMAT_R1_UNORM = 66,
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
        DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
        DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
        DXGI_FORMAT_BC1_TYPELESS = 70,
        DXGI_FORMAT_BC1_UNORM = 71,
        DXGI_FORMAT_BC1_UNORM_SRGB = 72,
        DXGI_FORMAT_BC2_TYPELESS = 73,
        DXGI_FORMAT_BC2_UNORM = 74,
        DXGI_FORMAT_BC2_UNORM_SRGB = 75,
        DXGI_FORMAT_BC3_TYPELESS = 76,
        DXGI_FORMAT_BC3_UNORM = 77,
        DXGI_FORMAT_BC3_UNORM_SRGB = 78,
        DXGI_FORMAT_BC4_TYPELESS = 79,
        DXGI_FORMAT_BC4_UNORM = 80,
        DXGI_FORMAT_BC4_SNORM = 81,
        DXGI_FORMAT_BC5_TYPELESS = 82,
        DXGI_FORMAT_BC5_UNORM = 83,
        DXGI_FORMAT_BC5_SNORM = 84,
        DXGI_FORMAT_B5G6R5_UNORM = 85,
        DXGI_FORMAT_B5G5R5A1_UNORM = 86,
        DXGI_FORMAT_B8G8R8A8_UNORM = 87,
        DXGI_FORMAT_B8G8R8X8_UNORM = 88,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
        DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
        DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
        DXGI_FORMAT_BC6H_TYPELESS = 94,
        DXGI_FORMAT_BC6H_UF16 = 95,
        DXGI_FORMAT_BC6H_SF16 = 96,
        DXGI_FORMAT_BC7_TYPELESS = 97,
        DXGI_FORMAT_BC7_UNORM = 98,
        DXGI_FORMAT_BC7_UNORM_SRGB = 99,
        DXGI_FORMAT_AYUV = 100,
        DXGI_FORMAT_Y410 = 101,
        DXGI_FORMAT_Y416 = 102,
        DXGI_FORMAT_NV12 = 103,
        DXGI_FORMAT_P010 = 104,
        DXGI_FORMAT_P016 = 105,
        DXGI_FORMAT_420_OPAQUE = 106,
        DXGI_FORMAT_YUY2 = 107,
        DXGI_FORMAT_Y210 = 108,
        DXGI_FORMAT_Y216 = 109,
        DXGI_FORMAT_NV11 = 110,
        DXGI_FORMAT_AI44 = 111,
        DXGI_FORMAT_IA44 = 112,
        DXGI_FORMAT_P8 = 113,
        DXGI_FORMAT_A8P8 = 114,
        DXGI_FORMAT_B4G4R4A4_UNORM = 115,

        DXGI_FORMAT_P208 = 130,
        DXGI_FORMAT_V208 = 131,
        DXGI_FORMAT_V408 = 132,


        DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
        DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,

        DXGI_FORMAT_A4B4G4R4_UNORM = 191,


        DXGI_FORMAT_FORCE_UINT = 0xffffffff
    } DXGI_FORMAT;

    PYRO_FORCEINLINE static constexpr Format ToFormat(DXGI_FORMAT dxgiFormat) {
        switch (dxgiFormat) {
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return Format::BGRA4Unorm;
        case DXGI_FORMAT_B5G6R5_UNORM:
            return Format::BGR565Unorm;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return Format::BGR5A1Unorm;
        case DXGI_FORMAT_R8_UNORM:
            return Format::R8Unorm;
        case DXGI_FORMAT_R8_SNORM:
            return Format::R8Snorm;
        case DXGI_FORMAT_R8_UINT:
            return Format::R8Uint;
        case DXGI_FORMAT_R8_SINT:
            return Format::R8Sint;
        case DXGI_FORMAT_R8G8_UNORM:
            return Format::RG8Unorm;
        case DXGI_FORMAT_R8G8_SNORM:
            return Format::RG8Snorm;
        case DXGI_FORMAT_R8G8_UINT:
            return Format::RG8Uint;
        case DXGI_FORMAT_R8G8_SINT:
            return Format::RG8Sint;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return Format::RGBA8Unorm;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return Format::RGBA8Snorm;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return Format::RGBA8Uint;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return Format::RGBA8Sint;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return Format::RGBA8Srgb;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return Format::BGRA8Unorm;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return Format::BGRA8Srgb;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return Format::A2RGB10Unorm;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return Format::A2RGB10Uint;
        case DXGI_FORMAT_R16_UNORM:
            return Format::R16Unorm;
        case DXGI_FORMAT_R16_SNORM:
            return Format::R16Snorm;
        case DXGI_FORMAT_R16_UINT:
            return Format::R16Uint;
        case DXGI_FORMAT_R16_SINT:
            return Format::R16Sint;
        case DXGI_FORMAT_R16_FLOAT:
            return Format::R16Sfloat;
        case DXGI_FORMAT_R16G16_UNORM:
            return Format::RG16Unorm;
        case DXGI_FORMAT_R16G16_SNORM:
            return Format::RG16Snorm;
        case DXGI_FORMAT_R16G16_UINT:
            return Format::RG16Uint;
        case DXGI_FORMAT_R16G16_SINT:
            return Format::RG16Sint;
        case DXGI_FORMAT_R16G16_FLOAT:
            return Format::RG16Sfloat;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return Format::RGBA16Unorm;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return Format::RGBA16Snorm;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return Format::RGBA16Uint;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return Format::RGBA16Sint;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return Format::RGBA16Sfloat;
        case DXGI_FORMAT_R32_UINT:
            return Format::R32Uint;
        case DXGI_FORMAT_R32_SINT:
            return Format::R32Sint;
        case DXGI_FORMAT_R32_FLOAT:
            return Format::R32Sfloat;
        case DXGI_FORMAT_R32G32_UINT:
            return Format::RG32Uint;
        case DXGI_FORMAT_R32G32_SINT:
            return Format::RG32Sint;
        case DXGI_FORMAT_R32G32_FLOAT:
            return Format::RG32Sfloat;
        case DXGI_FORMAT_R32G32B32_UINT:
            return Format::RGB32Uint;
        case DXGI_FORMAT_R32G32B32_SINT:
            return Format::RGB32Sint;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return Format::RGB32Sfloat;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return Format::RGBA32Uint;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return Format::RGBA32Sint;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return Format::RGBA32Sfloat;
        case DXGI_FORMAT_D16_UNORM:
            return Format::D16Unorm;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return Format::D24UnormS8Uint;
        case DXGI_FORMAT_D32_FLOAT:
            return Format::D32Sfloat;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return Format::D32SfloatS8Uint;
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return Format::S8Uint;

        // Block Compressed Formats
        case DXGI_FORMAT_BC1_UNORM:
            return Format::BC1RGBAUnormBlock;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return Format::BC1RGBASrgbBlock;
        case DXGI_FORMAT_BC2_UNORM:
            return Format::BC2UnormBlock;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return Format::BC2SrgbBlock;
        case DXGI_FORMAT_BC3_UNORM:
            return Format::BC3UnormBlock;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return Format::BC3SrgbBlock;
        case DXGI_FORMAT_BC4_UNORM:
            return Format::BC4UnormBlock;
        case DXGI_FORMAT_BC4_SNORM:
            return Format::BC4SnormBlock;
        case DXGI_FORMAT_BC5_UNORM:
            return Format::BC5UnormBlock;
        case DXGI_FORMAT_BC5_SNORM:
            return Format::BC5SnormBlock;
        case DXGI_FORMAT_BC6H_UF16:
            return Format::BC6HUfloatBlock;
        case DXGI_FORMAT_BC6H_SF16:
            return Format::BC6HSfloatBlock;
        case DXGI_FORMAT_BC7_UNORM:
            return Format::BC7UnormBlock;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return Format::BC7SrgbBlock;

        // Packed Formats
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return Format::E5BGR9Ufloat;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return Format::B10GR11Ufloat;

        default:
            return Format::Undefined;
        }
    }

    struct DDS_PIXELFORMAT {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
    };

    struct DDS_HEADER {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        uint32_t dwPitchOrLinearSize;
        uint32_t dwDepth;
        uint32_t dwMipMapCount;
        uint32_t dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        uint32_t dwCaps, dwCaps2, dwCaps3, dwCaps4;
        uint32_t dwReserved2;
    };

    struct DDS_HEADER_DXT10 {
        DXGI_FORMAT dxgiFormat;
        uint32_t resourceDimension;
        uint32_t miscFlag;
        uint32_t arraySize;
        uint32_t miscFlags2;
    };
    TextureLoadResult LoadDDS(const char* filepath) {
        TextureLoadResult result = {};

        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file)
            return result;

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        result.rawBuffer.resize(size);
        if (!file.read(reinterpret_cast<char*>(result.rawBuffer.data()), size))
            return result;

        uint32_t magic = *reinterpret_cast<uint32_t*>(result.rawBuffer.data());
        if (magic != DDS_MAGIC)
            return result;

        DDS_HEADER* header = reinterpret_cast<DDS_HEADER*>(result.rawBuffer.data() + sizeof(uint32_t));

        // Default to 1 if not specified
        uint32_t mipCount = std::max(1u, header->dwMipMapCount);
        uint32_t arraySize = 1;
        Format format = Format::Undefined;

        size_t dataOffset = sizeof(uint32_t) + sizeof(DDS_HEADER);

        // Check for DX10 extension (Standard for modern formats/BCn)
        if (header->ddspf.dwFlags & 0x4 /* DDPF_FOURCC */ && header->ddspf.dwFourCC == 0x30315844 /* "DX10" */) {
            DDS_HEADER_DXT10* dxt10Header = reinterpret_cast<DDS_HEADER_DXT10*>(result.rawBuffer.data() + dataOffset);
            dataOffset += sizeof(DDS_HEADER_DXT10);
            arraySize = std::max(1u, dxt10Header->arraySize);
            format = ToFormat(dxt10Header->dxgiFormat);
        } else if (header->ddspf.dwFlags & 0x4 /* DDPF_FOURCC */) {
            switch (header->ddspf.dwFourCC) {
            case MakeFourCC('D', 'X', 'T', '1'):
                format = Format::BC1RGBAUnormBlock;
                break;
            case MakeFourCC('D', 'X', 'T', '3'):
                format = Format::BC2UnormBlock;
                break;
            case MakeFourCC('D', 'X', 'T', '5'):
                format = Format::BC3UnormBlock;
                break;
            case MakeFourCC('A', 'T', 'I', '1'):
            case MakeFourCC('B', 'C', '4', 'U'):
                format = Format::BC4UnormBlock;
                break;
            case MakeFourCC('A', 'T', 'I', '2'):
            case MakeFourCC('B', 'C', '5', 'U'):
                format = Format::BC5UnormBlock;
                break;
            default:
                format = Format::Undefined;
                break; // Unsupported legacy compressed
            }
        } else {
            // Legacy Uncompressed RGB/RGBA formats
            format = Format::Undefined;
        }
        if (format == Format::Undefined) {
            result.success = false;
            return result;
        }

        result.info.size = { header->dwWidth, header->dwHeight, std::max(1u, header->dwDepth) };
        result.info.mipLevelCount = mipCount;
        result.info.arrayLayerCount = arraySize;
        result.info.format = format;
        result.info.dimensions = result.info.size.depth > 1 ? ImageDimensions::e3D : ImageDimensions::e2D;

        const RHIUtil::FormatBlockInfo blockInfo = RHIUtil::GetFormatBlockInfo(format);
        const uint8_t* pSrcData = result.rawBuffer.data() + dataOffset;

        for (uint32_t layer = 0; layer < arraySize; ++layer) {
            for (uint32_t mip = 0; mip < mipCount; ++mip) {
                u32 mipWidth = std::max(1u, header->dwWidth >> mip);
                u32 mipHeight = std::max(1u, header->dwHeight >> mip);
                u32 mipDepth = std::max(1u, header->dwDepth >> mip);

                u32 blocksX = (mipWidth + blockInfo.blockWidth - 1) / blockInfo.blockWidth;
                u32 blocksY = (mipHeight + blockInfo.blockHeight - 1) / blockInfo.blockHeight;

                u32 rowPitch = blocksX * blockInfo.bytesPerBlock;
                u32 slicePitch = rowPitch * blocksY;
                u32 mipSizeBytes = slicePitch * mipDepth;

                result.subresources.push_back({
                    .pixels = { pSrcData, mipSizeBytes },
                    .rowPitch = rowPitch,
                    .slicePitch = slicePitch,
                });

                pSrcData += mipSizeBytes;
            }
        }

        result.success = true;
        return result;
    }

} // namespace VisualTests