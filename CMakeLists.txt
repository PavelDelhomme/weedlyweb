cmake_minimum_required(VERSION 3.16)
project(WeedlyWeb VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Backends UI (plusieurs peuvent être activés en même temps)
option(BUILD_UI_GTK "Compiler le backend GTK/WebKit2GTK" ON)
option(BUILD_UI_QT "Compiler le backend Qt6/WebEngine" ON)
# Compatibilité ancienne option
option(USE_QT "DEPRECATED: utilise BUILD_UI_QT ; si ON force Qt seul" OFF)

if(USE_QT)
    message(WARNING "USE_QT est déprécié — utilisez -DBUILD_UI_QT=ON -DBUILD_UI_GTK=OFF")
    set(BUILD_UI_GTK OFF CACHE BOOL "" FORCE)
    set(BUILD_UI_QT ON CACHE BOOL "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
        ${CMAKE_SOURCE_DIR}/third_party/nlohmann
        ${CMAKE_BINARY_DIR}
)

if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(STATUS "nlohmann/json non trouvé, téléchargement...")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/nlohmann)
    file(DOWNLOAD
        https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        ${CMAKE_BINARY_DIR}/nlohmann/json.hpp
        TIMEOUT 30
        SHOW_PROGRESS
    )
    set(NLOHMANN_JSON_INCLUDE_DIR ${CMAKE_BINARY_DIR})
endif()

# Détection Qt (optionnelle)
set(QT6_AVAILABLE FALSE)
if(BUILD_UI_QT)
    find_package(Qt6 QUIET COMPONENTS Core Gui Widgets WebEngineWidgets)
    if(Qt6_FOUND)
        set(QT6_AVAILABLE TRUE)
        message(STATUS "✅ Qt6 WebEngine détecté — backend Qt activé")
    else()
        message(WARNING "⚠️  BUILD_UI_QT=ON mais Qt6 WebEngine introuvable — backend Qt désactivé")
        message(WARNING "💡 Arch: sudo pacman -S qt6-base qt6-webengine")
        set(BUILD_UI_QT OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Détection GTK
if(BUILD_UI_GTK)
    pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)
    message(STATUS "✅ WebKit2GTK détecté — backend GTK activé")
endif()

if(NOT BUILD_UI_GTK AND NOT BUILD_UI_QT)
    message(FATAL_ERROR "Aucun backend UI activé. Activez BUILD_UI_GTK et/ou BUILD_UI_QT.")
endif()

add_subdirectory(src/core)

if(BUILD_UI_GTK)
    add_subdirectory(src/ui/gtk)
endif()

if(BUILD_UI_QT)
    add_subdirectory(src/ui/qt)
endif()

# Installation commune
install(PROGRAMS
    ${CMAKE_SOURCE_DIR}/scripts/run-weedlyweb.sh
    DESTINATION lib/weedlyweb
)
install(PROGRAMS
    ${CMAKE_SOURCE_DIR}/scripts/weedlyweb
    DESTINATION bin
)
install(FILES
    ${CMAKE_SOURCE_DIR}/assets/desktop/weedlyweb.desktop
    DESTINATION share/applications
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
    DESTINATION share/WeedlyWeb
)

add_custom_target(project-help
    COMMAND ${CMAKE_COMMAND} -E echo "WeedlyWeb — backends: GTK=${BUILD_UI_GTK} Qt=${BUILD_UI_QT}"
    COMMAND ${CMAKE_COMMAND} -E echo "  make / cmake --build build"
    COMMAND ${CMAKE_COMMAND} -E echo "  ./scripts/run-weedlyweb.sh          # GTK (défaut)"
    COMMAND ${CMAKE_COMMAND} -E echo "  WEEDLYWEB_UI=qt ./scripts/run-weedlyweb.sh"
    COMMAND ${CMAKE_COMMAND} -E echo "  Structure: src/core + src/ui/{gtk,qt,android,cocoa,win32}"
)

message(STATUS "")
message(STATUS "✅ Configuration terminée")
message(STATUS "   Core     : WeedlyWebCore")
if(BUILD_UI_GTK)
    message(STATUS "   GTK      : WeedlyWeb  (src/ui/gtk)")
endif()
if(BUILD_UI_QT)
    message(STATUS "   Qt       : WeedlyWebQt (src/ui/qt)")
endif()
message(STATUS "   Stubs    : src/ui/{android,cocoa,win32}")
message(STATUS "")
