SET( EXTENSION_NAME "Extensions_SDLFont")

ADD_LIBRARY( ${EXTENSION_NAME}
  Resources/SDLFont.h
  Resources/SDLFont.cpp
)

TARGET_LINK_LIBRARIES( ${EXTENSION_NAME}
  OpenEngine_Core
  ${SDLTTF_LIBRARY}
)
