#include "asset_catalog.h"
#include "bom/bom.h"
#include "compressor.h"
#include <assert.h>
#include <memory>

typedef std::unique_ptr<struct bom_context, decltype(&bom_free)> unique_ptr_bom;
typedef std::unique_ptr<struct bom_tree_context, decltype(&bom_tree_free)> unique_ptr_bom_tree;

namespace iris {
namespace car {
template <class E, class T>
class TEnumAs {
public:
  TEnumAs(E e) : value((T)e) {}
  TEnumAs() : value() {}

  bool operator==(const TEnumAs& rhs) const { return value == rhs.value; }
  bool operator==(E const& e) const { return value == (T)e; }

  operator E() const { return (E)value; }

private:
  T value;
};

IB_PACKED(struct Header {
  char tag[4]; // 'CTAR'
  uint32_t coreuiVersion;
  uint32_t storageVersion;
  uint32_t storageTimestamp;
  uint32_t renditionCount;
  char mainVersionString[128];
  char versionString[256];
  uint8_t uuid[16];
  uint32_t associatedChecksum;
  uint32_t schemaVersion;
  uint32_t colorSpaceID;
  uint32_t keySemantics;
});

IB_PACKED(struct ExtendedMetadata {
  char tag[4]; // 'META'
  char thinningArguments[256];
  char deploymentPlatformVersion[256];
  char deploymentPlatform[256];
  char authoringTool[256];
});

enum RenditionAttributeType {
  kRenditionAttributeType_ThemeLook = 0,
  kRenditionAttributeType_Element = 1,
  kRenditionAttributeType_Part = 2,
  kRenditionAttributeType_Size = 3,
  kRenditionAttributeType_Direction = 4,
  kRenditionAttributeType_placeholder = 5,
  kRenditionAttributeType_Value = 6,
  kRenditionAttributeType_ThemeAppearance = 7,
  kRenditionAttributeType_Dimension1 = 8,
  kRenditionAttributeType_Dimension2 = 9,
  kRenditionAttributeType_State = 10,
  kRenditionAttributeType_Layer = 11,
  kRenditionAttributeType_Scale = 12,
  kRenditionAttributeType_Unknown13 = 13,
  kRenditionAttributeType_PresentationState = 14,
  kRenditionAttributeType_Idiom = 15,
  kRenditionAttributeType_Subtype = 16,
  kRenditionAttributeType_Identifier = 17,
  kRenditionAttributeType_PreviousValue = 18,
  kRenditionAttributeType_PreviousState = 19,
  kRenditionAttributeType_HorizontalSizeClass = 20,
  kRenditionAttributeType_VerticalSizeClass = 21,
  kRenditionAttributeType_MemoryLevelClass = 22,
  kRenditionAttributeType_GraphicsFeatureSetClass = 23,
  kRenditionAttributeType_DisplayGamut = 24,
  kRenditionAttributeType_DeploymentTarget = 25
};
typedef enum RenditionAttributeType RenditionAttributeType;
const char* RenditionAttributeString(RenditionAttributeType rat) {
  static const char* strs[] = {
      "ThemeLook",
      "Element",
      "Part",
      "Size",
      "Direction",
      "placeholder",
      "Value",
      "Theme Appearance",
      "Dimension1",
      "Dimension2",
      "State",
      "Layer",
      "Scale",
      "Unknown13",
      "Presentation State",
      "Idiom",
      "Subtype",
      "Identifier",
      "Previous Value",
      "Previous State",
      "Horizontal Size Class",
      "Vertical Size Class",
      "Memory Level Class",
      "Graphics Feature Set Class",
      "Display Gamut",
      "Deployment Target",
  };
  return strs[rat];
}

using RenditionAttribKey = TEnumAs<RenditionAttributeType, uint16_t>;
using RenditionAttribKey32 = TEnumAs<RenditionAttributeType, uint32_t>;
static_assert(sizeof(RenditionAttribKey) == sizeof(uint16_t), "size not equal");

IB_PACKED(struct RenditionAttribute {
  RenditionAttribKey name;
  uint16_t value;
});

IB_PACKED(struct Renditionkeytoken {
  struct {
    uint16_t x;
    uint16_t y;
  } cursorHotSpot;
  uint16_t numberOfAttributes;
  RenditionAttribute attributes[];
});

// MARK: - renditionkeyfmt

IB_PACKED(struct Renditionkeyfmt {
  char tag[4]; // 'kfmt'
  uint32_t version;
  uint32_t maximumRenditionKeyTokenCount;
  RenditionAttribKey32 renditionKeyTokens[];
});

// MARK: - csiheader

IB_PACKED(struct RenditionFlags {
  uint32_t isHeaderFlaggedFPO : 1;
  uint32_t isExcludedFromContrastFilter : 1;
  uint32_t isVectorBased : 1;
  uint32_t isOpaque : 1;
  uint32_t bitmapEncoding : 4;
  uint32_t optOutOfThinning : 1;
  uint32_t isFlippable : 1;
  uint32_t isTintable : 1;
  uint32_t preservedVectorRepresentation : 1;
  uint32_t reserved : 20;
});

IB_PACKED(struct csimetadata {
  uint32_t modtime;
  uint16_t layout;
  uint16_t zero;
  char name[128];
});

IB_PACKED(struct csibitmaplist {
  uint32_t count;
  uint32_t zero[];
});

 IB_PACKED(struct csiheader {
  char tag[4]; // 'CTSI'
  uint32_t version;
  struct RenditionFlags renditionFlags;
  uint32_t width;
  uint32_t height;
  uint32_t scaleFactor;
  uint32_t pixelFormat;
  struct {
    uint32_t colorSpaceID : 4;
    uint32_t reserved : 28;
  } colorSpace;
  struct csimetadata csimetadata;
  uint32_t tlv_length;
  struct csibitmaplist csibitmaplist;
});

// MARK: - Layout

enum RenditionLayoutType {
  kRenditionLayoutType_TextEffect = 0x007,
  kRenditionLayoutType_Vector = 0x009,

  kRenditionLayoutType_Data = 0x3E8,
  kRenditionLayoutType_ExternalLink = 0x3E9,
  kRenditionLayoutType_LayerStack = 0x3EA,
  kRenditionLayoutType_InternalReference = 0x3EB,
  kRenditionLayoutType_PackedImage = 0x3EC,
  kRenditionLayoutType_NameList = 0x3ED,
  kRenditionLayoutType_UnknownAddObject = 0x3EE,
  kRenditionLayoutType_Texture = 0x3EF, // Texture
  kRenditionLayoutType_TextureImage = 0x3F0,
  kRenditionLayoutType_Color = 0x3F1,
  kRenditionLayoutType_MultisizeImage = 0x3F2,
  kRenditionLayoutType_LayerReference = 0x3F4,
  kRenditionLayoutType_ContentRendition = 0x3F5,
  kRenditionLayoutType_RecognitionObject = 0x3F6,
};
typedef enum RenditionLayoutType RenditionLayoutType;

enum CoreThemeImageSubtype {
  kCoreThemeOnePartFixedSize = 10,
  kCoreThemeOnePartTile = 11,
  kCoreThemeOnePartScale = 12,
  kCoreThemeThreePartHTile = 20,
  kCoreThemeThreePartHScale = 21,
  kCoreThemeThreePartHUniform = 22,
  kCoreThemeThreePartVTile = 23,
  kCoreThemeThreePartVScale = 24,
  kCoreThemeThreePartVUniform = 25,
  kCoreThemeNinePartTile = 30,
  kCoreThemeNinePartScale = 31,
  kCoreThemeNinePartHorizontalUniformVerticalScale = 32,
  kCoreThemeNinePartHorizontalScaleVerticalUniform = 33,
  kCoreThemeNinePartEdgesOnly = 34,
  kCoreThemeManyPartLayoutUnknown = 40,
  kCoreThemeAnimationFilmstrip = 50
};
typedef enum CoreThemeImageSubtype CoreThemeImageSubtype;

// MARK: - TLV

// As seen in -[CSIGenerator writeResourcesToData:]
enum RenditionTLVType {
  kRenditionTLVType_Slices = 0x3E9,
  kRenditionTLVType_Metrics = 0x3EB,
  kRenditionTLVType_BlendModeAndOpacity = 0x3EC,
  kRenditionTLVType_UTI = 0x3ED,
  kRenditionTLVType_EXIFOrientation = 0x3EE, // bitmap info
  kRenditionTLVType_BytesPerRow = 0x3EF,
  kRenditionTLVType_ExternalTags = 0x3F0,
  kRenditionTLVType_Frame = 0x3F1,
  kRenditionTLVType_Reference = 0x3F2,
  kRenditionTLVType_AlphaCroppedFrame = 0x3F3,
  kRenditionTLVType_Texture = 0x3F6,
};
typedef enum RenditionTLVType RenditionTLVType;
//
const char* RenditionTLVTypeString(RenditionTLVType tlvt) {
  switch (tlvt) {
  case kRenditionTLVType_Slices:
    return "Slices";
  case kRenditionTLVType_Metrics:
    return "Metrics";
  case kRenditionTLVType_BlendModeAndOpacity:
    return "Blend Mode And Opacity";
  case kRenditionTLVType_UTI:
    return "UTI";
  case kRenditionTLVType_EXIFOrientation:
    return "EXIFOrientation";
  case kRenditionTLVType_BytesPerRow: // 1007 for CUIThemeTextureImageRendition
    return "BytesPerRow";
  case kRenditionTLVType_ExternalTags:
    return "ExternalTags";
  case kRenditionTLVType_Frame:
    return "Frame";
  case kRenditionTLVType_Texture:
    return "Texture";
  default:
    return "<null>";
  }
}

// MARK: - CUIRawDataRendition

IB_PACKED(struct CUIRawDataRendition {
  char tag[4]; // RAWD
  uint32_t version;
  uint32_t rawDataLength;
  uint8_t rawData[];
});

// MARK: - CUIRawPixelRendition

IB_PACKED(struct CUIRawPixelRendition {
  char tag[4]; // RAWD
  uint32_t version;
  uint32_t rawDataLength;
  uint8_t rawData[];
});

// MARK: - csicolor

IB_PACKED(struct csicolor {
  char tag[4]; // COLR
  uint32_t version;
  struct {
    uint32_t colorSpaceID : 8;
    uint32_t unknown0 : 3;
    uint32_t reserved : 21;
  } colorSpace;
  uint32_t numberOfComponents;
  double components[];
});

// MARK: - CUIThemePixelRendition

// As seen in _CUIConvertCompressionTypeToString
enum RenditionCompressionType {
  kRenditionCompressionType_uncompressed = 0,
  kRenditionCompressionType_rle,
  kRenditionCompressionType_zip,
  kRenditionCompressionType_lzvn,
  kRenditionCompressionType_lzfse,
  kRenditionCompressionType_jpeg_lzfse,
  kRenditionCompressionType_blurred,
  kRenditionCompressionType_astc,
  kRenditionCompressionType_palette_img,
  kRenditionCompressionType_deepmap_lzfse,
};
typedef enum RenditionCompressionType RenditionCompressionType;

using CompressionType = TEnumAs<RenditionCompressionType, uint32_t>;

IB_PACKED(struct CUIThemePixelRendition {
  char tag[4]; // 'CELM'
  uint32_t version;
  CompressionType compressionType;
  uint32_t rawDataLength;
  uint8_t rawData[];
});

IB_PACKED(struct CUIThemeTextureRendition {
  char tag[4]; // RTXT
  uint32_t version;
  uint32_t pixel_format; // textureFormat, 186, 0xba ?
  uint32_t _0;           // fixed value =1, faces, cubeMap ?
  uint32_t _1;           // ... =1, alphaInfo
  uint16_t array_length;
  uint16_t mips_count;
});

} // namespace car

AssetCatalog::AssetCatalog() {}
AssetCatalog::~AssetCatalog() {}
// @see https://blog.timac.org/2018/1018-reverse-engineering-the-car-file-format/
// @see https://github.com/Timac/QLCARFiles/blob/master/Common/CoreUI.h
// @see _CSIRenditionBlockData expandCSIBitmapData:fromSlice:makeReadOnly:
// @see CSIGenerator writeBitmap:toData:compress: with KCBC
bool AssetCatalog::load(const char* filename) {
  auto bom = std::make_unique<bom::Context>(filename);
  size_t data_len = 0;

  auto data = bom->get<car::Header>("CARHEADER", data_len);
  auto ext_meta = bom->get<car::ExtendedMetadata>("EXTENDED_METADATA", data_len);
  auto kfmt = bom->get<car::Renditionkeyfmt>("KEYFORMAT", data_len);
  if (kfmt) {
    for (int index = 0; index < kfmt->maximumRenditionKeyTokenCount; index++) {
      const car::RenditionAttribKey32& keytok = kfmt->renditionKeyTokens[index];
      printf("RKey: %s\n", RenditionAttributeString(keytok));
    }
  }
  bom->iterate(
      [](const std::string_view& key, const std::string_view& value) {
        printf("part info: %d, %s\n", (int)key.length(), key.data());
      },
      "PART_INFO");
  bom->iterate(
      [](const std::string_view& key, const std::string_view& value) {
        printf("elem info: %d, %s\n", (int)key.length(), key.data());
      },
      "ELEMENT_INFO");

  bom->iterate(
      [](const std::string_view& key, const std::string_view& value) {
        const char* name = key.data();
        car::Renditionkeytoken* token = (car::Renditionkeytoken*)value.data();
        printf("attributes: %d, %s\n", token->numberOfAttributes, name);
        if (token->numberOfAttributes > 0 && name) {
          for (int index = 0; index < token->numberOfAttributes; index++) {
            car::RenditionAttribute& attrib = token->attributes[index];
            printf("attrib: %s, value: %d\n", RenditionAttributeString(attrib.name), attrib.value);
          }
        }
      },
      "FACETKEYS");

  bom->iterate(
      [](const std::string_view& key, const std::string_view& value) {
        const char* name = key.data();
        uint16_t v = *(uint16_t*)value.data();
        printf("AK %s, AV: %d\n", name, v);
      },
      "APPEARANCEKEYS");
  int img_cnt = 0;
  int t_data_len = 0;
  bom->iterate(
      [&img_cnt,&t_data_len](const std::string_view& key, const std::string_view& value) {
        const char* name = key.data();
        printf("Rendition: (kl:%d) %s\n", (int)key.length(), name);
        car::csiheader* chdr = (car::csiheader*)value.data();
        auto size = sizeof(car::csiheader);
        printf("\tcsiHeader: version: %d\n", chdr->version);
        auto tlv_len = chdr->tlv_length;
        t_data_len += value.length();
        if (tlv_len > 0) {
          const char* tlvBytes
              = value.data() + sizeof(car::csiheader) + 4 * chdr->csibitmaplist.count + 4;

          const uint32_t* lptr = (const uint32_t*)(value.data() + sizeof(car::csiheader)
                                                   + 4 * chdr->csibitmaplist.count);

          printf("\tBom data length:%d, rendition length:%d, bitmaps count: %d\n",
                 (int)value.length(), *lptr, (int)chdr->csibitmaplist.count);

          for (int i = 0; i < chdr->csibitmaplist.count; i++) {
            printf("\t\tbitmap id: %d, flags: %d, 0x%0x\n", i, chdr->csibitmaplist.zero[i],
                   chdr->csibitmaplist.zero[i]);
          }

          const char* tlvPos = tlvBytes;

        // properties
          while (tlvBytes + tlv_len > tlvPos) {
            uint32_t tlvTag = *(uint32_t*)tlvPos;
            uint32_t tlvLength = *(uint32_t*)(tlvPos + 4);

            fprintf(stderr,
                    "\t\t\t\t %s: ", car::RenditionTLVTypeString((car::RenditionTLVType)tlvTag));

            if (tlvLength <= 12) {
              for (uint32_t valuePos = 0; valuePos < tlvLength; valuePos++) {
                uint8_t value = *(uint8_t*)(tlvPos + 8 + valuePos);
                fprintf(stderr, "%02X ", value);
              }
            } else {
              fprintf(stderr, "unknwon length: %d", tlvLength);
            }
            
            fprintf(stderr, "\n");

            tlvPos += 8 + tlvLength;
          }
        }
        // rendition property CUIThemeRendition _initializePropertiesFromCSIData:
        size_t offset
            = sizeof(car::csiheader) + chdr->tlv_length + 4 + 4 * chdr->csibitmaplist.count;

        const char* rendition_data = value.data() + offset;
        if (chdr->pixelFormat == 'ARGB') {
          const car::CUIThemePixelRendition* themePixelRendition
              = (const car::CUIThemePixelRendition*)rendition_data;
          //_CUIThemeTextureRendition _initWithCSIHeader:
          if (!strncmp(themePixelRendition->tag, "RTXT", 4)) {
            printf("\tThis is a texture.\n");
            const car::CUIThemeTextureRendition* textureRendition
                = (const car::CUIThemeTextureRendition*)rendition_data;
            printf("\tMipCount : %d\n", textureRendition->mips_count);
            size_t sz_rd = sizeof(car::CUIThemeTextureRendition);
            // keylist data
            const char* ptr = (const char*)rendition_data + sizeof(car::CUIThemeTextureRendition);
            size_t remain = value.size() - offset - sz_rd;
            const char* ptr_end = ptr + remain;
            int cnt = 0;
            while (ptr != ptr_end) {
              assert(ptr <= ptr_end);
              cnt++;
              auto nx_sz = *(uint32_t*)ptr;
              ptr += sizeof(uint32_t);
              auto nx_sz2 = *(uint32_t*)ptr;
              ptr += sizeof(uint32_t);
              // RenditionKey
              ptr += nx_sz; // 23x4 = 92 28 bytes = 7 uint32_ts
            }
            printf("\tto end\n");
          } else if (!strncmp(themePixelRendition->tag, "MLEC", 4)
                     || !strncmp(themePixelRendition->tag, "CELM", 4)) {
            printf("\tThis is an image.\n");
            if (themePixelRendition->version & 1) {
              if (*(const uint32_t*)(themePixelRendition->rawData) == 'KCBC') // 5,6?
              {
                uint32_t compressed_length = *((const uint32_t*)(themePixelRendition->rawData) + 4);
                const char* compressed_data
                    = (const char*)((const uint32_t*)(themePixelRendition->rawData) + 5);
              }
            }
            car::RenditionCompressionType compressionType = themePixelRendition->compressionType;
            uint32_t rawDataLength = themePixelRendition->rawDataLength;
            const uint8_t* rawDataBytes = themePixelRendition->rawData;
            printf("\traw_data length: %d\n", rawDataLength);
            switch (compressionType) {
            case car::kRenditionCompressionType_uncompressed:
              printf("\tuncompressed\n");
              break;
            case car::kRenditionCompressionType_rle: //
              printf("\trle\n");
              break;
            case car::kRenditionCompressionType_zip: //
              printf("\tzip\n");
              break;
            case car::kRenditionCompressionType_lzvn: //
              printf("\tlzvn\n");
              break;
            case car::kRenditionCompressionType_lzfse: { // bvx2.. length
              auto o_len = *(uint32_t*)(rawDataBytes + 4);

              // compression_decode_buffer()
              printf("\tlzfse o: %d, c: %d\n", o_len, rawDataLength);
              break;
            }
            case car::kRenditionCompressionType_jpeg_lzfse:
              printf("\tjpeg lzfse\n");
              break;
            case car::kRenditionCompressionType_blurred:
              printf("\tblurred\n");
              break;
            case car::kRenditionCompressionType_astc:
              printf("\tastc\n");
              break;
            default:
              printf("\tunkn flag, \n");
              break;
            }
            img_cnt++;
          } else {
            printf("\tunknown tag: %4s\n", themePixelRendition->tag);
          }
        } else if (chdr->pixelFormat == 'GA8\x1F') {
          printf("\tunknown format GA8\n");       
        } else if (chdr->pixelFormat == 'GA8 ') {
          printf("\tunknown format GA8\n");
        } else if (chdr->pixelFormat == 'RGBW') {
          printf("\tunknown format RGBW\n");
        } else if (chdr->pixelFormat == 'GA16') {
          printf("\tunknown format GA16\n");
        }
        else {
          const char* format = (const char*)&chdr->pixelFormat;
          printf("\tunknown format\n");
        }
      },
      "RENDITIONS");
  float t_MB = t_data_len / 1024.0f / 1024.0f;
  printf("%.2f MB", t_MB);
  return false;
}

void AssetCatalog::save(const char* filename) {}

} // namespace iris
