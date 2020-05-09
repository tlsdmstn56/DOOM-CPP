file(GLOB DOOM_SRC ./*.cpp)
add_executable(doom ${DOOM_SRC})
target_compile_definitions(doom 
    PRIVATE NORMALUNIX LINUX
)
target_link_libraries(doom 
    PRIVATE m X11 Xext
)
target_include_directories(doom 
   PRIVATE ${SPDLOG_INCLUDE_DIRS}
)
get_target_property(DOOM_BIN_DIR doom RUNTIME_OUTPUT_DIRECTORY )
file(COPY ./doom.wad DESTINATION ./)
target_compile_options( doom
    PRIVATE -Wall -Wno-write-strings
)