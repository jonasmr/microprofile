cmake_minimum_required(VERSION 3.8)
project(microprofile)

option(MICROPROFILE_USE_CONFIG_FILE "Use user provided configuration in microprofile.config.h file." OFF)

set(MICROPROFILE_EXPORT_FILENAME microprofile.export.h)
set(MICROPROFILE_CONFIG_HEADER ${PROJECT_SOURCE_DIR}/microprofile.config.h)
set(MICROPROFILE_PUBLIC_HEADERS
	${PROJECT_SOURCE_DIR}/microprofile.h
	${CMAKE_CURRENT_BINARY_DIR}/${MICROPROFILE_EXPORT_FILENAME}
)

set(THREADS_PREFER_PTHREAD_FLAG ON)

add_library(${PROJECT_NAME} microprofile.cpp)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
	EXPORT_MACRO_NAME MICROPROFILE_API
	EXPORT_FILE_NAME ${MICROPROFILE_EXPORT_FILENAME}
)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(${PROJECT_NAME} PUBLIC MICROPROFILE_EXPORT)

if (MICROPROFILE_USE_CONFIG_FILE)
	target_compile_definitions(${PROJECT_NAME} PUBLIC MICROPROFILE_USE_CONFIG)
endif()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${MICROPROFILE_PUBLIC_HEADERS}")

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(stb QUIET IMPORTED_TARGET stb)
endif()
if (stb_FOUND)
	target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::stb)
	target_compile_definitions(${PROJECT_NAME} PRIVATE MICROPROFILE_SYSTEM_STB)
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME})
if (MICROPROFILE_USE_CONFIG_FILE)
	install(FILES ${MICROPROFILE_CONFIG_HEADER} TYPE INCLUDE)
endif()
