add_library(lobby STATIC
    api.cpp
    api.h
)

create_target_directory_groups(lobby)

get_directory_property(OPENSSL_LIBS
        DIRECTORY ${PROJECT_SOURCE_DIR}/externals/libressl
        DEFINITION OPENSSL_LIBS)
target_compile_definitions(lobby PRIVATE -DCPPHTTPLIB_OPENSSL_SUPPORT)
target_link_libraries(lobby PRIVATE common json-headers ${OPENSSL_LIBS} httplib)
