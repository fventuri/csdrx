cmake_minimum_required (VERSION 3.3)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project (csdrx VERSION 0.1.0)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)

find_package(Csdr 0.18 REQUIRED)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O3")
set(CMAKE_C_FLAGS_DEBUG "-g -O3")

add_library(csdrx SHARED)
set_target_properties(csdrx PROPERTIES VERSION ${PROJECT_VERSION})
file(GLOB LIBCSDRX_HEADERS "${PROJECT_SOURCE_DIR}/*/*.hpp")
set_target_properties(csdrx PROPERTIES PUBLIC_HEADER "${LIBCSDRX_HEADERS}")

install(TARGETS csdrx
    EXPORT CsdrxTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/csdrx"
)

configure_file(csdrx.pc.in csdrx.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/csdrx.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(EXPORT CsdrxTargets
    FILE CsdrxTargets.cmake
    NAMESPACE Csdrx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Csdrx
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/CsdrxConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Csdrx
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CsdrxConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CsdrxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CsdrxConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Csdrx
)

include(FindPkgConfig)

if(NOT DEFINED COMPONENTS OR "filesource" IN_LIST COMPONENTS)
    add_subdirectory(filesource)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:filesource>)
endif()

if(NOT DEFINED COMPONENTS OR "filesourcemeasuredelay" IN_LIST COMPONENTS)
    add_subdirectory(filesourcemeasuredelay)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:filesourcemeasuredelay>)
endif()

if(NOT DEFINED COMPONENTS OR "pulseaudiowriter" IN_LIST COMPONENTS)
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)
    add_subdirectory(pulseaudiowriter)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:pulseaudiowriter>)
    target_link_libraries(csdrx ${PULSEAUDIO_LIBRARIES})
endif()

if(NOT DEFINED COMPONENTS OR "sdrplaysource" IN_LIST COMPONENTS)
    find_library(SDRPLAY_API_LIBRARIES libsdrplay_api.so.3)
    if(NOT SDRPLAY_API_LIBRARIES)
        message(FATAL_ERROR "libsdrplay_api v3 not found.")
    endif()
    get_filename_component(SDRPLAY_API_LIBRARY_DIRS "${SDRPLAY_API_LIBRARIES}" DIRECTORY)
    add_subdirectory(sdrplaysource)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:sdrplaysource>)
    target_link_libraries(csdrx ${SDRPLAY_API_LIBRARIES})
endif()

if(NOT DEFINED COMPONENTS OR "soapysource" IN_LIST COMPONENTS)
    pkg_check_modules(SOAPY_SDR REQUIRED SoapySDR)
    add_subdirectory(soapysource)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:soapysource>)
    target_link_libraries(csdrx ${SOAPY_SDR_LIBRARIES})
endif()

if(NOT DEFINED COMPONENTS OR "tcpsourcemeasuredelay" IN_LIST COMPONENTS)
    add_subdirectory(tcpsourcemeasuredelay)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:tcpsourcemeasuredelay>)
endif()

if(NOT DEFINED COMPONENTS OR "pulseaudiowritermeasuredelay" IN_LIST COMPONENTS)
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)
    add_subdirectory(pulseaudiowritermeasuredelay)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:pulseaudiowritermeasuredelay>)
    target_link_libraries(csdrx ${PULSEAUDIO_LIBRARIES})
endif()

if(NOT DEFINED COMPONENTS OR "sdrplaysourcemeasuredelay" IN_LIST COMPONENTS)
    find_library(SDRPLAY_API_LIBRARIES libsdrplay_api.so.3)
    if(NOT SDRPLAY_API_LIBRARIES)
        message(FATAL_ERROR "libsdrplay_api v3 not found.")
    endif()
    get_filename_component(SDRPLAY_API_LIBRARY_DIRS "${SDRPLAY_API_LIBRARIES}" DIRECTORY)
    add_subdirectory(sdrplaysourcemeasuredelay)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:sdrplaysourcemeasuredelay>)
    target_link_libraries(csdrx ${SDRPLAY_API_LIBRARIES})
endif()

if(NOT DEFINED COMPONENTS OR "soapysourcemeasuredelay" IN_LIST COMPONENTS)
    pkg_check_modules(SOAPY_SDR REQUIRED SoapySDR)
    add_subdirectory(soapysourcemeasuredelay)
    target_sources(csdrx PRIVATE $<TARGET_OBJECTS:soapysourcemeasuredelay>)
    target_link_libraries(csdrx ${SOAPY_SDR_LIBRARIES})
endif()
