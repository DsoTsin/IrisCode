#![allow(dead_code)]
//@see https://www.patreon.com/posts/36000874
use serde::{Serialize, Deserialize};

#[repr(C)]
#[derive(Debug)]
pub enum AttributeType {
    ThemeLook = 0,
    Element = 1,
    Part = 2,
    Size = 3,
    Direction = 4,
    Placeholder = 5,
    Value = 6,
    ThemeAppearance = 7,
    Dimension1 = 8,
    Dimension2 = 9,
    State = 10,
    Layer = 11,
    Scale = 12,
    Unknown13 = 13,
    PresentationState = 14,
    Idiom = 15,
    Subtype = 16,
    Identifier = 17,
    PreviousValue = 18,
    PreviousState = 19,
    HorizontalSizeClass = 20,
    VerticalSizeClass = 21,
    MemoryLevelClass = 22,
    GraphicsFeatureSetClass = 23,
    DisplayGamut = 24,
    DeploymentTarget = 25,
    GlyphWeight = 26,
    GlyphSize = 27,
}

#[repr(C)]
#[derive(Debug)]
pub enum LayoutType {
    TextEffect = 0x007,
    Vector = 0x009,

    Data = 0x3E8,
    ExternalLink = 0x3E9,
    LayerStack = 0x3EA,
    InternalRefernce = 0x3EB,
    PackedImage = 0x3EC,
    NameList = 0x3ED,
    UnknownAddObject = 0x3EE,
    Texture = 0x3EF, // Texture
    TextureImage = 0x3F0,
    Color = 0x3F1,
    MultisizeImage = 0x3F2,
    LayerReference = 0x3F4,
    ContentRendition = 0x3F5,
    RecognitionObject = 0x3F6,
}

#[repr(C)]
#[derive(Debug)]
pub enum TLVType {
    Slices = 0x3E9,
    Metrics = 0x3EB,
    BlendModeAndOpacity = 0x3EC,
    UTI = 0x3ED,
    EXIFOrientation = 0x3EE,
    BytesPerRow = 0x3EF,
    ExternalTags = 0x3F0,
    Frame = 0x3F1,
    Reference = 0x3F2,
    AlphaCroppedFrame = 0x3F3,
    Texture = 0x3F6,
}

#[repr(u32)]
#[derive(Debug)]
pub enum CompressionType {
    Uncompressed,
    RLE,
    Zip,
    LZVN,
    LZFSE,
    JpegLZFSE,
    Blurred,
    ASTC,
    PaletteImg,
    DeepmapLZFSE
}

#[allow(non_camel_case_types)]
#[repr(u32)]
#[derive(Debug)]
pub enum PixelFormat
{
    Invalid = 0,
    A8Unorm = 1,
    R8Unorm = 10,
    R8Unorm_sRGB = 11,
    R8Snorm = 12,
    R8Uint = 13,
    R8Sint = 14,
    R16Unorm = 20,
    R16Snorm = 22,
    R16Uint = 23,
    R16Sint = 24,
    R16Float = 25,
    RG8Unorm = 30,
    RG8Unorm_sRGB = 31,
    RG8Snorm = 32,
    RG8Uint = 33,
    RG8Sint = 34,
    B5G6R5Unorm = 40,
    A1BGR5Unorm = 41,
    ABGR4Unorm = 42,
    BGR5A1Unorm = 43,
    R32Uint = 53,
    R32Sint = 54,
    R32Float = 55,
    RG16Unorm = 60,
    RG16Snorm = 62,
    RG16Uint = 63,
    RG16Sint = 64,
    RG16Float = 65,
    RGBA8Unorm = 70,
    RGBA8Unorm_sRGB = 71,
    RGBA8Snorm = 72,
    RGBA8Uint = 73,
    RGBA8Sint = 74,
    BGRA8Unorm = 80,
    BGRA8Unorm_sRGB = 81,
    RGB10A2Unorm = 90,
    RGB10A2Uint = 91,
    RG11B10Float = 92,
    RGB9E5Float = 93,
    BGR10A2Unorm = 94,
    RG32Uint = 103,
    RG32Sint = 104,
    RG32Float = 105,
    RGBA16Unorm = 110,
    RGBA16Snorm = 112,
    RGBA16Uint = 113,
    RGBA16Sint = 114,
    RGBA16Float = 115,
    RGBA32Uint = 123,
    RGBA32Sint = 124,
    RGBA32Float = 125,
    BC1_RGBA = 130,
    BC1_RGBA_sRGB = 131,
    BC2_RGBA = 132,
    BC2_RGBA_sRGB = 133,
    BC3_RGBA = 134,
    BC3_RGBA_sRGB = 135,
    BC4_RUnorm = 140,
    BC4_RSnorm = 141,
    BC5_RGUnorm = 142,
    BC5_RGSnorm = 143,
    BC6H_RGBFloat = 150,
    BC6H_RGBUfloat = 151,
    BC7_RGBAUnorm = 152,
    BC7_RGBAUnorm_sRGB = 153,
    PVRTC_RGB_2BPP = 160,
    PVRTC_RGB_2BPP_sRGB = 161,
    PVRTC_RGB_4BPP = 162,
    PVRTC_RGB_4BPP_sRGB = 163,
    PVRTC_RGBA_2BPP = 164,
    PVRTC_RGBA_2BPP_sRGB = 165,
    PVRTC_RGBA_4BPP = 166,
    PVRTC_RGBA_4BPP_sRGB = 167,
    EAC_R11Unorm = 170,
    EAC_R11Snorm = 172,
    EAC_RG11Unorm = 174,
    EAC_RG11Snorm = 176,
    EAC_RGBA8 = 178,
    EAC_RGBA8_sRGB = 179,
    ETC2_RGB8 = 180,
    ETC2_RGB8_sRGB = 181,
    ETC2_RGB8A1 = 182,
    ETC2_RGB8A1_sRGB = 183,
    ASTC_4x4_sRGB = 186,
    ASTC_5x4_sRGB = 187,
    ASTC_5x5_sRGB = 188,
    ASTC_6x5_sRGB = 189,
    ASTC_6x6_sRGB = 190,
    ASTC_8x5_sRGB = 192,
    ASTC_8x6_sRGB = 193,
    ASTC_8x8_sRGB = 194,
    ASTC_10x5_sRGB = 195,
    ASTC_10x6_sRGB = 196,
    ASTC_10x8_sRGB = 197,
    ASTC_10x10_sRGB = 198,
    ASTC_12x10_sRGB = 199,
    ASTC_12x12_sRGB = 200,
    ASTC_4x4_LDR = 204,
    ASTC_5x4_LDR = 205,
    ASTC_5x5_LDR = 206,
    ASTC_6x5_LDR = 207,
    ASTC_6x6_LDR = 208,
    ASTC_8x5_LDR = 210,
    ASTC_8x6_LDR = 211,
    ASTC_8x8_LDR = 212,
    ASTC_10x5_LDR = 213,
    ASTC_10x6_LDR = 214,
    ASTC_10x8_LDR = 215,
    ASTC_10x10_LDR = 216,
    ASTC_12x10_LDR = 217,
    ASTC_12x12_LDR = 218,
    ASTC_4x4_HDR = 222,
    ASTC_5x4_HDR = 223,
    ASTC_5x5_HDR = 224,
    ASTC_6x5_HDR = 225,
    ASTC_6x6_HDR = 226,
    ASTC_8x5_HDR = 228,
    ASTC_8x6_HDR = 229,
    ASTC_8x8_HDR = 230,
    ASTC_10x5_HDR = 231,
    ASTC_10x6_HDR = 232,
    ASTC_10x8_HDR = 233,
    ASTC_10x10_HDR = 234,
    ASTC_12x10_HDR = 235,
    ASTC_12x12_HDR = 236,
    GBGR422 = 240,
    BGRG422 = 241,
    Depth16Unorm = 250,
    Depth32Float = 252,
    Stencil8 = 253,
    Depth24Unorm_Stencil8 = 255,
    Depth32Float_Stencil8 = 260,
    X32_Stencil8 = 261,
    X24_Stencil8 = 262,
    BGRA10_XR = 552,
    BGRA10_XR_sRGB = 553,
    BGR10_XR = 554,
    BGR10_XR_sRGB = 555,
}

#[repr(C)]
#[derive(Debug, Default, Serialize, Deserialize)]
pub(crate) struct Attribute {
    key: u16,
    value: u16
}

#[repr(C)]
#[derive(Debug, Default, Serialize, Deserialize)]
pub(crate) struct KeyToken {
    cursor_x: u16,
    cursor_y: u16,
    num_attrs: u16,
}

#[repr(C)]
#[derive(Debug, Default, Serialize, Deserialize)]
pub(crate) struct KeyFmt {
    tag: [u8;4],
    version: u32,
    max_key_tok_cnt: u32,
}

#[repr(C)]
//#[derive(Debug, Default, Serialize, Deserialize)]
pub(crate) struct CSIMetaData {
    modtime: u32,
    layout: u16,
    zero: u16,
    name: [u8;128]
}

impl std::fmt::Debug for CSIMetaData {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        use std::ffi::CStr;
        let name = unsafe { CStr::from_ptr(self.name.as_ptr() as _) };
        f.debug_struct("CSIMetaData")
            .field("modtime", &self.modtime)
            .field("layout", &self.layout)
            .field("name", &name)
            .finish()
    }
}

#[repr(C)]
#[derive(Debug, Default, Serialize, Deserialize)]
pub(crate) struct CSIBitmapList {
    pub count: u32,
}

#[repr(C)]
pub(crate) struct CSIHeader {
    tag: [u8;4],
    version: u32,
    /// Rendition Flags
    pub flags: u32,
    pub width: u32,
    pub height: u32,
    pub scale_factor: u32,
    /// DATA, ARGB, JPEG, HEIF
    pub pixel_format: [u8;4],
    pub color_space: u32,
    meta_data: CSIMetaData,
    pub tlv_length: u32,
    pub bitmaps: CSIBitmapList
}

impl std::fmt::Debug for CSIHeader {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let tag = std::str::from_utf8(&self.tag).unwrap();
        let pxfmt = std::str::from_utf8(&self.pixel_format).unwrap();
        f.debug_struct("CSIHeader")
            .field("tag", &tag)
            .field("version", &self.version)
            .field("flags", &self.flags)
            .field("width", &self.width)
            .field("height", &self.height)
            .field("scale_factor", &self.scale_factor)
            .field("pixel_format", &pxfmt)
            .field("color_space", &self.color_space)
            .field("meta_data", &self.meta_data)
            .field("tlv_length", &self.tlv_length)
            .field("bitmaps", &self.bitmaps)
            .finish()
    }
}

#[repr(C)]
#[derive(Debug)]
pub (crate) struct CUIThemePixelRendition {
    version: u32,
    compress: CompressionType,
    length: u32
}

/// mipLevel, textureFace, flipped
#[repr(C)]
#[derive(Debug)]
pub (crate) struct CUIThemeTextureRendition {
    version: u32,
    pixel_format: PixelFormat, // 8
    dimensions: u32, // 12 TXRTextureInfo setDimensions
    array_length: u32, // 16
    cubemap: u16, // :20 == 5
    mips: u16, // 22
    key_size: u32, // 24
}
/*
struct CUIRenditionKey
{
  NSObject super;
  _renditionkeytoken _stackKey[22];
  _renditionkeytoken *_key;
  unsigned __int16 _highwaterKeyCount;
};

struct _renditionkeytoken
{
  unsigned __int16 identifier;
  unsigned __int16 value;
};


_CUIThemeTextureRendition _initWithCSIHeader:
Dimensions
*(double *)
_mm_shuffle_ps(
    (__m128)*(unsigned __int64 *)&hdr->width,
    _mm_shuffle_ps(
        (__m128)*(unsigned int *)(buffer + 12), 
        (__m128)*(unsigned __int64 *)&hdr->width,
        48
    ),
    132
).i64

    rendition_unk = (unsigned int *)(buffer2 + 24); value is 24 bytes, key is 12 bytes
    v18 = 0;
    v25 = "addObject:";
    do
    {
      memset(keylist, 0, 92); // 16x6, 23x4
      v19 = *rendition_unk; // = 24
      if ( v19 >= 88 )
        v19 = 88LL;
      __memcpy_chk(keylist, rendition_unk + 2, v19, 92LL);
      v20 = [CUIRenditionKey alloc];
      v21 = [v20 initWithKeyList: keylist];
      [*((id *)v28 + 31) addObject:v21];
      objc_release(v21);
      rendition_unk = (unsigned int *)((char *)rendition_unk + *rendition_unk + 8);
      ++v18;
    }
    while ( v18 < mips );
 */

#[repr(C)]
#[derive(Debug)]
pub (crate) struct CUIRawPixelRendition {
    version: u32,
    length: u32
}