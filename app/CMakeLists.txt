# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)

set(FFMPEG_INCLUDE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/include)
set(FFMPEG_LIB_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/lib/arm64-v8a)
set(FFMPEG_LIBS avformat avcodec avutil swscale swresample)

include_directories(${FFMPEG_INCLUDE_DIR})

foreach(sname ${FFMPEG_LIBS})
    add_library(${sname} SHARED IMPORTED)
    set_target_properties(${sname} PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB_DIR}/lib${sname}.so)
endforeach()

add_library(haoplayer SHARED
    src/main/cpp/player_jni.cpp
    src/main/cpp/player.cpp
    src/main/cpp/demuxer.cpp
    src/main/cpp/audio_decoder.cpp
    src/main/cpp/audio_render.cpp
    src/main/cpp/audio_device.cpp
    src/main/cpp/audio_track.cpp
    src/main/cpp/video_decoder.cpp
    src/main/cpp/video_render.cpp
    src/main/cpp/video_device.cpp
    src/main/cpp/ffwrapper.cpp)


# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.
find_library(LOG_LIB log)
find_library(ANDROID_LIB android)

target_link_libraries(haoplayer ${FFMPEG_LIBS} ${LOG_LIB} ${ANDROID_LIB})