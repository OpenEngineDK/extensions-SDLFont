#SET(CMAKE_INCLUDE_PATH ${OE_LIB_DIR}/sdlimage/include/SDL)
#SET(CMAKE_LIBRARY_PATH ${OE_LIB_DIR}/sdlimage/lib/static)
INCLUDE(${CMAKE_ROOT}/Modules/FindSDL_ttf.cmake)
INCLUDE(${OE_CURRENT_EXTENSION_DIR}/SetupSDL_ttf.cmake)
