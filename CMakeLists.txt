cmake_minimum_required(VERSION 3.21)
project(SNESTilesRipper VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# All binaries land in <repo>/bin (compression plugins in bin/plugins)
set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
file(MAKE_DIRECTORY ${BIN_DIR}/plugins)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

set(LOW ${CMAKE_SOURCE_DIR}/src/lowlevelstuff/src)

add_compile_options(-Wall)

# ------------------------------------------------------------------ data

set(LOWLEVEL_SOURCES
	${LOW}/tile.c
	${LOW}/mapping_hirom.c
	${LOW}/mapping_lorom.c
	${LOW}/rommapping.c
	${LOW}/palette.c
	${LOW}/rominfo.c
	${LOW}/savestateloader.c
	${LOW}/savestate/savestate.c
	${LOW}/puff/puff.c
	${LOW}/savestate/snes9x_152.c)

# ------------------------------------------------------------------ GUI

file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM APP_SOURCES ${CMAKE_SOURCE_DIR}/src/tinymain.cpp)
file(GLOB APP_UIS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.ui)

add_executable(snestilesripper
	${APP_SOURCES}
	${LOWLEVEL_SOURCES}
	${APP_UIS})
if(WIN32)
	target_sources(snestilesripper PRIVATE ${CMAKE_SOURCE_DIR}/src/snestilesripper.rc)
endif()
target_include_directories(snestilesripper PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${LOW}
	${LOW}/savestate
	${LOW}/puff
	${LOW}/compressions)
target_link_libraries(snestilesripper PRIVATE
	Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
set_target_properties(snestilesripper PROPERTIES OUTPUT_NAME "SNESTilesRipper")

# ------------------------------------------------------------------ CLI

set(CLI_SOURCES
	${CMAKE_SOURCE_DIR}/src/tinymain.cpp
	${CMAKE_SOURCE_DIR}/src/romdataengine.cpp
	${CMAKE_SOURCE_DIR}/src/tilepreset.cpp
	${CMAKE_SOURCE_DIR}/src/myrominfo.cpp
	${CMAKE_SOURCE_DIR}/src/pngexportimport.cpp
	${CMAKE_SOURCE_DIR}/src/tilespattern.cpp
	${CMAKE_SOURCE_DIR}/src/snespalette.cpp
	${LOW}/tile.c
	${LOW}/mapping_hirom.c
	${LOW}/mapping_lorom.c
	${LOW}/rommapping.c
	${LOW}/rominfo.c
	${LOW}/palette.c)

add_executable(snestilesripper-cli ${CLI_SOURCES})
target_include_directories(snestilesripper-cli PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(snestilesripper-cli PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(snestilesripper-cli PROPERTIES OUTPUT_NAME "SNESTilesRipperCLI")

# ------------------------------------------------- compression plugins

add_library(nintendocompression MODULE
	${CMAKE_SOURCE_DIR}/src/compressions/Nintendo/nintendo.cpp
	${LOW}/compressions/stdnintendo.c)
target_include_directories(nintendocompression PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/compressions/Nintendo
	${LOW}/compressions)
target_link_libraries(nintendocompression PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(nintendocompression PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR}/plugins
	OUTPUT_NAME "nintendocompression")

add_library(konamicompression MODULE
	${CMAKE_SOURCE_DIR}/src/compressions/Konami/konamicompression.cpp
	${CMAKE_SOURCE_DIR}/src/compressions/Konami/Comp.cpp
	${CMAKE_SOURCE_DIR}/src/compressions/Konami/Global.cpp)
target_include_directories(konamicompression PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/compressions/Konami)
target_link_libraries(konamicompression PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(konamicompression PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR}/plugins
	OUTPUT_NAME "konamicompression")

add_library(quintetcompression MODULE
	${CMAKE_SOURCE_DIR}/src/compressions/Quintet/quintetcompression.cpp
	${CMAKE_SOURCE_DIR}/src/compressions/Quintet/Comp.cpp
	${CMAKE_SOURCE_DIR}/src/compressions/Quintet/Global.cpp
	${LOW}/compressions/quintet1/quintet1_compression.c
	${LOW}/compressions/quintet1/bitstream/src/bitstream.c)
target_include_directories(quintetcompression PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/compressions/Quintet
	${LOW}/compressions/quintet1
	${LOW}/compressions/quintet1/bitstream/include)
target_link_libraries(quintetcompression PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(quintetcompression PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR}/plugins
	OUTPUT_NAME "quintetcompression")

add_library(compression MODULE
	${CMAKE_SOURCE_DIR}/src/compressions/BandaiNamco/compression.cpp
	${CMAKE_SOURCE_DIR}/src/compressions/BandaiNamco/Comp.cpp
	${CMAKE_SOURCE_DIR}/src/compressions/BandaiNamco/Global.cpp)
target_include_directories(compression PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/compressions/BandaiNamco)
target_link_libraries(compression PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(compression PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR}/plugins
	OUTPUT_NAME "compression")