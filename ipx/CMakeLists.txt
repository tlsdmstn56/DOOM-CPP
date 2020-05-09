file(GLOB IPX_SRC ./*.c)
add_library(doomipx SHARED ${IPX_SRC})
set_property(TARGET doomipx PROPERTY POSITION_INDEPENDENT_CODE ON)
# target_compile_definitions(doomipx 
#     PRIVATE NORMALUNIX LINUX
# )
# target_link_libraries(doom 
#     PRIVATE m X11 Xext
# )