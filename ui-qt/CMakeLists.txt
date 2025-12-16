cmake_minimum_required(VERSION 3.16)
project(WeedlyWebQt VERSION 1.0)

# Activer le C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Activer AUTOMOC, AUTOUIC, AUTORCC pour Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Chercher PkgConfig (nécessaire pour pkg_check_modules)
find_package(PkgConfig REQUIRED)

# Chercher Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets)

# Vérifier que Qt6 est trouvé
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 n'a pas pu être trouvé. Installez Qt6 avec : sudo pacman -S qt6-base qt6-webengine")
endif()

# Rechercher cURL (pour HTTPManager)
# Utiliser pkg-config pour cURL (plus fiable que find_package)
pkg_check_modules(CURL REQUIRED libcurl)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "cURL n'a pas pu être trouvé. Installez-le avec : sudo pacman -S curl")
endif()

# Rechercher SQLite3 (pour Database)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
if(NOT SQLITE3_FOUND)
    message(FATAL_ERROR "SQLite3 n'a pas pu être trouvé. Installez-le avec : sudo pacman -S sqlite")
endif()

# Inclure les répertoires
# ui-qt/CMakeLists.txt est dans ui-qt/, donc le parent est la racine
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
include_directories(${PROJECT_ROOT}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Rechercher nlohmann/json
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    ${PROJECT_ROOT}/third_party/nlohmann
    ${CMAKE_BINARY_DIR}
    ${PROJECT_ROOT}/build/nlohmann
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

if(NLOHMANN_JSON_INCLUDE_DIR)
    include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# Sources Qt (chemins relatifs depuis ui-qt/)
set(QT_SOURCES
    src/qt/main.cpp
    src/qt/BrowserWindow.cpp
    src/qt/WebView.cpp
    src/qt/TabBar.cpp
    src/qt/FavoritesBar.cpp
    src/qt/NavigationBar.cpp
    src/qt/CommandPalette.cpp
)

# Headers Qt
set(QT_HEADERS
    include/qt/BrowserWindow.h
    include/qt/WebView.h
    include/qt/TabBar.h
    include/qt/FavoritesBar.h
    include/qt/NavigationBar.h
    include/qt/CommandPalette.h
)

# Sources partagées (managers, database, etc.) - chemins depuis la racine
# EXCLURE les managers qui dépendent de GTK/WebKit
set(SHARED_SOURCES
    ${PROJECT_ROOT}/src/managers/HTTPManager.cpp
    ${PROJECT_ROOT}/src/managers/FileManager.cpp
    ${PROJECT_ROOT}/src/database/Database.cpp
    ${PROJECT_ROOT}/src/utils/CVEAnalyzer.cpp
    # Exclure FavoritesManager, TabsManager, MemoryManager, Utils car ils dépendent de GTK
    # ${PROJECT_ROOT}/src/managers/FavoritesManager.cpp
    # ${PROJECT_ROOT}/src/managers/TabsManager.cpp
    # ${PROJECT_ROOT}/src/managers/MemoryManager.cpp
    # ${PROJECT_ROOT}/src/utils/Utils.cpp
    # ${PROJECT_ROOT}/src/engine/ScriptEngine.cpp
)

set(SHARED_HEADERS
    ${PROJECT_ROOT}/include/managers/HTTPManager.h
    ${PROJECT_ROOT}/include/managers/FileManager.h
    ${PROJECT_ROOT}/include/database/Database.h
    ${PROJECT_ROOT}/include/utils/CVEAnalyzer.h
    # Exclure les headers GTK-dépendants
    # ${PROJECT_ROOT}/include/managers/FavoritesManager.h
    # ${PROJECT_ROOT}/include/managers/TabsManager.h
    # ${PROJECT_ROOT}/include/managers/MemoryManager.h
    # ${PROJECT_ROOT}/include/utils/Utils.h
    # ${PROJECT_ROOT}/include/engine/ScriptEngine.h
)

# Créer l'exécutable Qt
add_executable(WeedlyWebQt ${QT_SOURCES} ${QT_HEADERS} ${SHARED_SOURCES} ${SHARED_HEADERS})

# Lier Qt6 et autres bibliothèques
target_link_libraries(WeedlyWebQt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
)

# Lier cURL
target_link_libraries(WeedlyWebQt PRIVATE ${CURL_LIBRARIES})

# Lier SQLite3
target_link_libraries(WeedlyWebQt PRIVATE ${SQLITE3_LIBRARIES})

# Inclure les répertoires
target_include_directories(WeedlyWebQt PRIVATE
    ${PROJECT_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NLOHMANN_JSON_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

# Copier les assets
add_custom_command(TARGET WeedlyWebQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_ROOT}/assets ${CMAKE_BINARY_DIR}/assets
)

