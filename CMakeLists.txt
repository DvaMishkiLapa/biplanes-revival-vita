cmake_minimum_required(VERSION 3.15)

# PS Vita specific configuration
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

# Vita specific settings
set(TARGET BiplanesRevival)
set(${TARGET}_VERSION 1.2.1)
project(${TARGET} VERSION ${${TARGET}_VERSION} LANGUAGES C CXX)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "${TARGET}")
set(VITA_TITLEID  "BPREVIVAL")  # Valid Vita title ID format
set(VITA_VERSION  "01.21")

if(DEFINED ENV{VITA_IP})
  set(VITA_IP $ENV{VITA_IP})
endif()

# Compiler flags for Vita
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2 -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -ffast-math -Wno-psabi")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")

# Create executable
add_executable(${TARGET})

# Set C++ standard
set_target_properties(${TARGET} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
)

# Add Vita SDK modules path
list(APPEND CMAKE_MODULE_PATH "${VITASDK}/share/cmake/Modules")

# Find SDL2 libraries for Vita (simplified)
find_package(SDL2 2.0.10 REQUIRED)

# Add all source files
target_sources(${TARGET} PRIVATE
  src/biplanes.cpp

  include/fwd.hpp
  include/enums.hpp
  include/constants.hpp

  include/game_state.hpp
  include/network_data.hpp
  include/network_state.hpp
  include/canvas.hpp
  include/color.hpp
  include/stats.hpp
  include/sounds.hpp
  include/textures.hpp
  include/variables.hpp

  src/bullet.cpp
  include/bullet.hpp

  src/cloud.cpp
  include/cloud.hpp

  src/controls.cpp
  include/controls.hpp

  src/effects.cpp
  include/effects.hpp

  src/math.cpp
  include/math.hpp

  src/menu.cpp
  src/menu_input.cpp
  src/menu_navigation.cpp
  src/menu_sp.cpp
  src/menu_mp.cpp
  src/menu_help.cpp
  src/menu_stats.cpp
  include/menu.hpp

  src/plane.cpp
  src/plane_input.cpp
  src/plane_pilot.cpp
  include/plane.hpp

  src/render.cpp
  include/render.hpp

  src/resources.cpp
  include/resources.hpp

  src/sdl.cpp
  include/sdl.hpp

  src/time.cpp
  include/time.hpp

  src/timer.cpp
  include/timer.hpp

  src/utility.cpp
  include/utility.hpp

  src/zeppelin.cpp
  include/zeppelin.hpp

  src/ai_stuff.cpp
  include/ai_stuff.hpp

  # Network files (Vita-specific implementation)
  lib/Net-vita.h
  src/matchmake.cpp
  include/matchmake.hpp
  src/network.cpp
  include/network.hpp
)

# Add TimeUtils dependency
add_subdirectory(deps/TimeUtils)

# Compile definitions
target_compile_definitions(${TARGET} PRIVATE
  BIPLANES_EXE_NAME="${TARGET}"
  BIPLANES_VERSION="${${TARGET}_VERSION}"
  _USE_MATH_DEFINES
  VITA_PLATFORM
)

# Include directories
target_include_directories(${TARGET} PRIVATE
  ${SDL2_INCLUDE_DIR}
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

# Compiler options
target_compile_options(${TARGET} PUBLIC -Wno-narrowing)

# Link libraries for Vita (simplified)
target_link_libraries(${TARGET} PUBLIC
  TimeUtils::TimeUtils
  SDL2_mixer
  SDL2_image
  SDL2
  modplug
  vorbisfile
  opusfile
  xmp
  webpdecoder
  pthread
  webpdemux
  vorbis
  ogg
  opus
  vorbisidec

  SceDisplay_stub
  SceCtrl_stub
  SceTouch_stub
  SceMotion_stub
  SceCommonDialog_stub
  SceGxm_stub
  SceAudioIn_stub
  SceAudio_stub
  SceHid_stub
  SceNet_stub
  SceNetCtl_stub
  SceLibDbg_stub
)

# Copy assets to build directory
file(
  COPY assets
  DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
)

# Create Vita package
vita_create_self(${VITA_APP_NAME}.self ${TARGET})
vita_create_vpk(${VITA_APP_NAME}.vpk ${VITA_TITLEID} ${VITA_APP_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  ./assets assets
) 

# Optional: Send the VPK directly to the PS Vita
if (VITA_IP)
  add_custom_target(send
    COMMAND "echo" destroy | nc ${VITA_IP} 1338
    COMMAND "sleep" 1s
    COMMAND unzip -o ${VITA_APP_NAME}.vpk eboot.bin -d ${CMAKE_BINARY_DIR}
    COMMAND "curl" --ftp-method nocwd -T ${CMAKE_BINARY_DIR}/eboot.bin "ftp://${VITA_IP}:1337/ux0:/app/${VITA_TITLEID}/"
    COMMAND "echo" launch ${VITA_TITLEID} | nc ${VITA_IP} 1338
    DEPENDS ${VITA_APP_NAME}.vpk
  )
endif()

if(DEFINED ENV{VITA_IP})
  set(PARSECOREPATH $ENV{PARSECOREPATH})
endif()

# Optional: Dump debugging information
add_custom_target(dump
  COMMAND ${CMAKE_SOURCE_DIR}/get_dump.sh ${VITA_IP}
  COMMAND python3 ${PARSECOREPATH} coredump ${CMAKE_PROJECT_NAME}
)
