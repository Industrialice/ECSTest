# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

if(NOT DEFINED ${ENABLE_DEBUG})
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(ENABLE_DEBUG "TRUE")
    else()
        set(ENABLE_DEBUG "FALSE")
    endif()
endif()

if(${ENABLE_DEBUG} MATCHES "TRUE")
    add_compile_options(-Og -g -ftrapv)
    add_definitions(-DDEBUG -D_DEBUG)
else()
    add_compile_options(-Ofast -fno-rtti -fno-stack-protector -D_FORTIFY_SOURCE=0)
    add_definitions(-DNDEBUG -D_NDEBUG)
endif()

#add_compile_options(-fsanitize=address -fno-omit-frame-pointer)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++2a> $<$<COMPILE_LANGUAGE:C>:-std=c11>)
add_compile_options(-fsized-deallocation)
add_compile_options(-fno-exceptions)
add_compile_options( -fvisibility=hidden -fvisibility-inlines-hidden)
add_compile_options(-fstrict-aliasing)
#add_compile_options(-fno-strict-aliasing)

if(ANDROID_ABI STREQUAL "armeabi-v7a")
    add_compile_options(-march=armv7-a)
    add_compile_options(-mfloat-abi=softfp)
    add_compile_options(-mfpu=neon)
elseif(ANDROID_ABI STREQUAL "arm64-v8a")
elseif(ANDROID_ABI STREQUAL "x86")
elseif(ANDROID_ABI STREQUAL "x86-64")
endif()

#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
#set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
#set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")

add_definitions(-DPLATFORM_ANDROID)

file(GLOB CORE_FILES "../../../../StdLib2018/Core/*.cpp")
file(GLOB PLATFORM_FILES "../../../../StdLib2018/Platform/*.cpp")
file(GLOB MISC_FILES "../../../../StdLib2018/Misc/*.cpp")
file(GLOB ECS_FILES "../../../ECS/*.cpp")
file(GLOB UNITTEST_FILES "../../*.cpp")

foreach(item ${PLATFORM_FILES})
  if(${item} MATCHES "Win_.*cpp")
    list(REMOVE_ITEM PLATFORM_FILES ${item})
  endif()
endforeach()

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library(
    # Sets the name of the library.
    native-lib

    # Sets the library as a shared library.
    SHARED

    # Provides a relative path to your source file(s).
    src/main/cpp/native-lib.cpp
    ${CORE_FILES}
    ${PLATFORM_FILES}
    ${MISC_FILES}
    ${ECS_FILES}
    ${UNITTEST_FILES}
)

include_directories(
    ../../../../StdLib2018/Core/
    ../../../../StdLib2018/Platform/
    ../../../../StdLib2018/Misc/
    ../../../ECS/
    ../../
)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )