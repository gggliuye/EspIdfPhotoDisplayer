set(srcs
    "JPEGDEC/src/JPEGDEC.cpp"
    "JPEGDEC/src/jpeg.inl"
    "JPEGDEC/src/s3_simd_420.S"
    "JPEGDEC/src/s3_simd_444.S"
    "JPEGDEC/src/s3_simd_dequant.S"
)
idf_component_register(SRCS ${srcs}
                    REQUIRES "esp-dsp"
                    INCLUDE_DIRS "JPEGDEC/src"
)
