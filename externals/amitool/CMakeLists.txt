add_library(amitool STATIC
    amiibo.c
    amiitool.c
    drbg.c
    keygen.c
    util.c
    version.c
    gitversion.h
    util.h
    mbedtls/aes.c
    mbedtls/md_wrap.c
    mbedtls/md.c
    mbedtls/sha256.c
    include/mbedtls/aes.h
    include/mbedtls/check_config.h
    include/mbedtls/config.h
    include/mbedtls/md_internal.h
    include/mbedtls/md.h
    include/mbedtls/platform.h
    include/mbedtls/sha256.h
    include/nfc3d/amiibo.h
    include/nfc3d/amitool.h
    include/nfc3d/drbg.h
    include/nfc3d/keygen.h
    include/nfc3d/version.h
)

create_target_directory_groups(amitool)
target_include_directories(amitool PUBLIC "include/")
