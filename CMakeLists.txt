cmake_minimum_required(VERSION 3.16)
project(WeedlyWeb VERSION 1.0)

# Activer le C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Inclure le répertoire des en-têtes
include_directories(${PROJECT_SOURCE_DIR}/include)

# Chercher la bibliothèque WebKit2GTK
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)

# Vérification de la bibliothèque
if(NOT WEBKIT2GTK_FOUND)
    message(FATAL_ERROR "webKit2GTK 4.1 n'a pas pu être trouvé. Assurez-vous que webkit2gtk-4.1.pc est disponible.")
endif()

# Rechercher la bibliothèque cURL
find_package(CURL REQUIRED)

# Rechercher SQLite3
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Rechercher nlohmann/json (header-only library)
# Essayer de trouver dans les chemins standards
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    # Si pas trouvé, télécharger directement le header
    message(STATUS "nlohmann/json non trouvé, téléchargement...")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/nlohmann)
    file(DOWNLOAD 
        https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        ${CMAKE_BINARY_DIR}/nlohmann/json.hpp
        TIMEOUT 30
        SHOW_PROGRESS
    )
    set(NLOHMANN_JSON_INCLUDE_DIR ${CMAKE_BINARY_DIR})
    message(STATUS "nlohmann/json téléchargé dans: ${CMAKE_BINARY_DIR}/nlohmann/json.hpp")
    message(STATUS "Include directory: ${NLOHMANN_JSON_INCLUDE_DIR}")
endif()

# Ajouter le répertoire d'inclusion pour nlohmann/json
if(NLOHMANN_JSON_INCLUDE_DIR)
    include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
    message(STATUS "nlohmann/json ajouté aux include directories: ${NLOHMANN_JSON_INCLUDE_DIR}")
endif()

# Add source files (organized by category)
set(SOURCES
    src/main.cpp
    # Browser core
    src/browser/Browser.cpp
    # Rendering engine
    src/rendering/RenderingEngine.cpp
    # Managers
    src/managers/FavoritesManager.cpp
    src/managers/TabsManager.cpp
    src/managers/HTTPManager.cpp
    src/managers/MemoryManager.cpp
    src/managers/FileManager.cpp
    # Script engine
    src/engine/ScriptEngine.cpp
    # Utils
    src/utils/Utils.cpp
    src/utils/CVEAnalyzer.cpp
    src/utils/CommandPalette.cpp
    src/utils/RequestInterceptor.cpp
    # Database
    src/database/Database.cpp
)

set(HEADERS
    # Browser core
    include/browser/Browser.h
    # Rendering engine
    include/rendering/RenderingEngine.h
    # Managers
    include/managers/FavoritesManager.h
    include/managers/TabsManager.h
    include/managers/HTTPManager.h
    include/managers/MemoryManager.h
    include/managers/FileManager.h
    # Script engine
    include/engine/ScriptEngine.h
    # Utils
    include/utils/Utils.h
    include/utils/CVEAnalyzer.h
    include/utils/CommandPalette.h
    include/utils/RequestInterceptor.h
    # Database
    include/database/Database.h
)

# Créer l'exécutable
add_executable(WeedlyWeb ${SOURCES} ${HEADERS})

# Inclure les répertoires nécessaires
target_include_directories(WeedlyWeb PRIVATE 
    ${WEBKIT2GTK_INCLUDE_DIRS} 
    ${GDK_PIXBUF_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

# Lier les bibliothèques nécessaires
target_link_libraries(WeedlyWeb PRIVATE 
    ${WEBKIT2GTK_LIBRARIES} 
    gtk-3 
    ${GDK_PIXBUF_LIBRARIES}
    ${CURL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

# Ajouter des commandes post-build
add_custom_command(TARGET WeedlyWeb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets/settings
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets/datas
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets/icons
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets/help
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/settings/config.json ${CMAKE_BINARY_DIR}/assets/settings/config.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/datas/favorites.json ${CMAKE_BINARY_DIR}/assets/datas/favorites.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/styles/style.css ${CMAKE_BINARY_DIR}/assets/styles/style.css
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/help/help.html ${CMAKE_BINARY_DIR}/assets/help/help.html
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/icons/weedlyweb.svg ${CMAKE_BINARY_DIR}/assets/icons/weedlyweb.svg
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/icons/weedlyweb.png ${CMAKE_BINARY_DIR}/assets/icons/weedlyweb.png
)

# Définir le répertoire d'installation
set(INSTALL_DIR /usr/local)

# Ajouter une cible install pour l'exécutable
install(TARGETS WeedlyWeb RUNTIME DESTINATION ${INSTALL_DIR}/bin)

# Installer les fichiers de configuration et styles
install(FILES
    ${CMAKE_SOURCE_DIR}/assets/settings/config.json
    ${CMAKE_SOURCE_DIR}/assets/datas/favorites.json
    ${CMAKE_SOURCE_DIR}/assets/styles/style.css
    ${CMAKE_SOURCE_DIR}/assets/help/help.html
    DESTINATION ${INSTALL_DIR}/share/WeedlyWeb
)


# Ajouter une cible de nettoyage sécurisée (renommée pour éviter les conflits avec `make clean`)
add_custom_target(custom_clean ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/WeedlyWeb
    COMMENT "Nettoyage sécurisé du répertoire de build sans supprimer les dépendances"
)