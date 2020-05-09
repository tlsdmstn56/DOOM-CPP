file(GLOB SOUND_SERVER_SRC ./*.cpp)
add_executable(doom_sound_server ${SOUND_SERVER_SRC})
# target_compile_definitions(doomipx 
#     PRIVATE NORMALUNIX LINUX
# )
# target_link_libraries(doom 
#     PRIVATE m X11 Xext
# )