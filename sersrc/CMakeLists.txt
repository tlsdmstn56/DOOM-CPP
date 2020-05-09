file(GLOB SERIAL_SRC ./*.c)
add_library(doomserial SHARED ${SERIAL_SRC})
set_property(TARGET doomserial PROPERTY POSITION_INDEPENDENT_CODE ON)
# target_compile_definitions(doomipx 
#     PRIVATE NORMALUNIX LINUX
# )
# target_link_libraries(doom 
#     PRIVATE m X11 Xext
# )