using(FFMPEG optional)
using(XERCESC)
covise_find_package(WMFSDK)

if (NOT WMFSDK_FOUND AND NOT FFMPEG_FOUND)
    return()
endif()

SET(HEADERS Video.h)
SET(SOURCES Video.cpp)

IF(WMFSDK_FOUND AND NOT FFMPEG_FOUND)
  ADD_DEFINITIONS(-DHAVE_WMFSDK)

  SET(HEADERS ${HEADERS} WINAVIVideo.h rgb2rgb.h swscale.h swscale_internal.h)
  SET(SOURCES ${SOURCES} WINWMVVideo.cpp WINAVIVideo.cpp rgb2rgb.c swscale.c yuv2rgb.c)

  INCLUDE_DIRECTORIES(SYSTEM ${WMFSDK_INCLUDE_DIRS})
  SET(EXTRA_LIBS ${EXTRA_LIBS} ${WMFSDK_LIBRARIES})
endif()
  
if (FFMPEG_FOUND)
  ADD_DEFINITIONS("-DHAVE_CODEC_CTX_PTR")
  
  SET(HEADERS ${HEADERS} FFMPEGVideo.h)
  SET(SOURCES ${SOURCES} FFMPEGVideo.cpp)
ENDIF(FFMPEG_FOUND)

cover_add_plugin(Video)

IF(MSVC AND NOT MSVC10)
# can't have this anymore, if you still nedd it add it for your compiler only or use a new ffmpeg if that helps ADD_COVISE_COMPILE_FLAGS(Video "/Zc:wchar_t-")
ENDIF()

