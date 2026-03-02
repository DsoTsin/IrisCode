use lzfse_rust::*;

#[repr(C)]
pub enum CompressionAlgorithm {
    LZ4      = 0x100,       // available starting OS X 10.11, iOS 9.0
    ZLIB     = 0x205,       // available starting OS X 10.11, iOS 9.0
    LZMA     = 0x306,       // available starting OS X 10.11, iOS 9.0
    LZ4RAW   = 0x101,       // available starting OS X 10.11, iOS 9.0

    /* Apple-specific algorithm */
    LZFSE    = 0x801,       // available starting OS X 10.11, iOS 9.0
    BROTLI   = 0xB02,
}

#[inline(always)]
pub fn encode_buffer(dst: &mut [u8], src: &[u8], _scratch: *mut u8, algorithm: CompressionAlgorithm) -> usize {
    match algorithm {
        // https://developer.apple.com/documentation/compression/compression_algorithm/compression_lz4?language=objc
        CompressionAlgorithm::LZ4 => {
            lz4_flex::compress_into(src, dst).unwrap() as _
        },
        CompressionAlgorithm::ZLIB => {
            // Level 5
            unsafe {
                let mut ds = 0;
                libz_sys::compress2(dst.as_mut_ptr(), &mut ds, src.as_ptr(), src.len() as _, 5);
                ds as _
            }
        },
        CompressionAlgorithm::LZMA => {
            // Level 6
            let encode_options = lzma_rs::compress::Options {
                unpacked_size: lzma_rs::compress::UnpackedSize::WriteToHeader(Some(src.len() as u64)),
            };
            let mut compressed: Vec<u8> = Vec::new();
            lzma_rs::lzma_compress_with_options(
                &mut std::io::Cursor::new(src),
                &mut compressed,
                &encode_options,
            ).unwrap();
            0
        },
        CompressionAlgorithm::LZ4RAW => todo!(),
        CompressionAlgorithm::LZFSE => {
            let mut encoder = LzfseRingEncoder::default();
            let (_u, v) = encoder.encode(
                &mut std::io::Cursor::new(src), 
                &mut std::io::Cursor::new(dst)
            ).unwrap();
            v as _
        },
        CompressionAlgorithm::BROTLI => todo!(),
    }
}

#[inline(always)]
pub fn decode_buffer(dst: &mut [u8], src: &[u8], _scratch: *mut u8, algorithm: CompressionAlgorithm) -> usize {
    match algorithm {
        CompressionAlgorithm::LZ4 => {
            lz4_flex::decompress_into(src, dst).unwrap() as _
        },
        CompressionAlgorithm::ZLIB => {
            let mut ds = 0;
            unsafe {
                libz_sys::uncompress(dst.as_mut_ptr(), &mut ds, src.as_ptr(), src.len() as _);
                ds as _
            }
        },
        CompressionAlgorithm::LZMA => {
            let decode_options = lzma_rs::decompress::Options {
                unpacked_size: lzma_rs::decompress::UnpackedSize::ReadFromHeader,
                ..Default::default()
            };
            let mut bf = std::io::BufReader::new(src);
            let mut decomp: Vec<u8> = Vec::new();
            lzma_rs::lzma_decompress_with_options(
                &mut bf, 
                &mut decomp,
                &decode_options
            ).unwrap();
            0
        },
        // LZ4 without block header
        CompressionAlgorithm::LZ4RAW => {
            0
        },
        CompressionAlgorithm::LZFSE => {
            let mut decoder = LzfseRingDecoder::default();
            let (_u, v) = decoder.decode(
                &mut std::io::Cursor::new(src), 
                &mut std::io::Cursor::new(dst)
            ).unwrap();
            v as _
        },
        CompressionAlgorithm::BROTLI => todo!(),
    }
}

#[no_mangle]
pub extern "C" fn compression_decode_scratch_buffer_size(_algorithm: CompressionAlgorithm) -> usize {
    0
}

#[no_mangle]
pub extern "C" fn compression_encode_scratch_buffer_size(_algorithm: CompressionAlgorithm) -> usize {
    0
}

#[no_mangle]
pub extern "C" fn compression_encode_buffer(d: *mut u8, ds: usize, s:*const u8, ss: usize, sb: *mut u8, algorithm: CompressionAlgorithm) -> usize {
    unsafe {
        encode_buffer(std::slice::from_raw_parts_mut(d, ds), std::slice::from_raw_parts(s, ss), sb, algorithm)
    }
}

#[no_mangle]
pub extern "C" fn compression_decode_buffer(d: *mut u8, ds: usize, s:*const u8, ss: usize, sb: *mut u8, algorithm: CompressionAlgorithm) -> usize {
    unsafe {
        decode_buffer(std::slice::from_raw_parts_mut(d, ds), std::slice::from_raw_parts(s, ss), sb, algorithm)
    }
}