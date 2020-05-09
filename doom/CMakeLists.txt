##########################
# build doom
# 
file(GLOB DOOM_SRC ./*.cpp)
add_executable(doom ${DOOM_SRC})
target_compile_definitions(doom 
    PRIVATE NORMALUNIX LINUX
    PUBLIC VERSION=110
)
target_link_libraries(doom 
    PRIVATE m X11 Xext spdlog
)
target_compile_options( doom
    PRIVATE -Wall
)

##########################
# copy doom.wad
# 
get_target_property(DOOM_BIN_DIR doom RUNTIME_OUTPUT_DIRECTORY )
file(COPY ./doom.wad DESTINATION ./)
