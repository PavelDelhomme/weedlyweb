cmake_minimum_required(VERSION 3.16)
project(WeedlyWeb VERSION 1.0)

# Activer le C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Option pour choisir l'interface (Qt ou GTK)
# Par défaut, Qt est prioritaire si disponible, sinon GTK
option(USE_QT "Utiliser Qt6 au lieu de GTK" ON)

# Si USE_QT est ON, vérifier que Qt est disponible
if(USE_QT)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(QT6_TEST QUIET Qt6Core Qt6Gui Qt6Widgets Qt6WebEngineWidgets)
        if(NOT QT6_TEST_FOUND)
            message(WARNING "⚠️  Qt6 demandé mais non détecté, basculement vers GTK")
            message(WARNING "💡 Pour installer Qt6 : sudo pacman -S qt6-base qt6-webengine")
            set(USE_QT OFF CACHE BOOL "Utiliser Qt6" FORCE)
        else()
            message(STATUS "✅ Qt6 détecté et activé par défaut")
        endif()
    else()
        message(WARNING "⚠️  pkg-config non trouvé, basculement vers GTK")
        set(USE_QT OFF CACHE BOOL "Utiliser Qt6" FORCE)
    endif()
endif()

# Si Qt est demandé, utiliser ui-qt
if(USE_QT)
    message(STATUS "🔧 Configuration pour Qt6")
    message(STATUS "📁 Build directory: ${CMAKE_BINARY_DIR}/ui-qt/build")
    add_subdirectory(ui-qt)
    set(EXECUTABLE_NAME WeedlyWebQt)
    set(EXECUTABLE_PATH ${CMAKE_BINARY_DIR}/ui-qt/build/WeedlyWebQt)
else()
    message(STATUS "🔧 Configuration pour GTK")
    message(STATUS "📁 Build directory: ${CMAKE_BINARY_DIR}")
    
    # Configuration GTK
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
    
    # Rechercher nlohmann/json
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
        PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
        ${CMAKE_SOURCE_DIR}/third_party/nlohmann
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
        message(STATUS "nlohmann/json téléchargé dans: ${CMAKE_BINARY_DIR}/nlohmann/json.hpp")
    endif()
    
    if(NLOHMANN_JSON_INCLUDE_DIR)
        include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
    endif()
    
    # Sources GTK
    set(SOURCES
        src/main.cpp
        src/browser/Browser.cpp
        src/rendering/RenderingEngine.cpp
        src/managers/FavoritesManager.cpp
        src/managers/TabsManager.cpp
        src/managers/HTTPManager.cpp
        src/managers/MemoryManager.cpp
        src/managers/FileManager.cpp
        src/engine/ScriptEngine.cpp
        src/utils/Utils.cpp
        src/utils/CVEAnalyzer.cpp
        src/utils/CommandPalette.cpp
        src/utils/RequestInterceptor.cpp
        src/database/Database.cpp
    )
    
    set(HEADERS
        include/browser/Browser.h
        include/rendering/RenderingEngine.h
        include/managers/FavoritesManager.h
        include/managers/TabsManager.h
        include/managers/HTTPManager.h
        include/managers/MemoryManager.h
        include/managers/FileManager.h
        include/engine/ScriptEngine.h
        include/utils/Utils.h
        include/utils/CVEAnalyzer.h
        include/utils/CommandPalette.h
        include/utils/RequestInterceptor.h
        include/database/Database.h
    )
    
    # Créer l'exécutable GTK
    add_executable(WeedlyWeb ${SOURCES} ${HEADERS})
    
    # Flags de debug
    target_compile_options(WeedlyWeb PRIVATE -g)
    
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
    
    # Copier les assets
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
    
    # Installation
    set(INSTALL_DIR /usr/local)
    install(TARGETS WeedlyWeb RUNTIME DESTINATION ${INSTALL_DIR}/bin)
    install(FILES
        ${CMAKE_SOURCE_DIR}/assets/settings/config.json
        ${CMAKE_SOURCE_DIR}/assets/datas/favorites.json
        ${CMAKE_SOURCE_DIR}/assets/styles/style.css
        ${CMAKE_SOURCE_DIR}/assets/help/help.html
        DESTINATION ${INSTALL_DIR}/share/WeedlyWeb
    )
    
    set(EXECUTABLE_NAME WeedlyWeb)
    set(EXECUTABLE_PATH ${CMAKE_BINARY_DIR}/WeedlyWeb)
endif()

# Cible help personnalisée pour afficher les instructions de démarrage
# Note: "help" est réservé par CMake, on utilise "project-help" qui sera accessible via "make help" grâce à un alias
add_custom_target(project-help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "🌐 WeedlyWeb - Guide de démarrage"
    COMMAND ${CMAKE_COMMAND} -E echo "=================================="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "📋 Commandes principales :"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  make              Compile le projet"
    COMMAND ${CMAKE_COMMAND} -E echo "  make clean        Nettoie le répertoire de build"
    COMMAND ${CMAKE_COMMAND} -E echo "  make install      Installe l'application"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "🚀 Pour démarrer rapidement :"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  1. Installer les dépendances :"
    COMMAND ${CMAKE_COMMAND} -E echo "     ./scripts/install-deps.sh"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  2. Configurer CMake (si pas déjà fait) :"
    COMMAND ${CMAKE_COMMAND} -E echo "     cmake -B build -S ."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  3. Compiler :"
    COMMAND ${CMAKE_COMMAND} -E echo "     make"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  4. Lancer l'application :"
    COMMAND ${CMAKE_COMMAND} -E echo "     ./${EXECUTABLE_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "📚 Documentation :"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  - README.md                    Documentation principale"
    COMMAND ${CMAKE_COMMAND} -E echo "  - docs/QUICKSTART.md           Guide de démarrage rapide"
    COMMAND ${CMAKE_COMMAND} -E echo "  - docs/INSTALL_DEPENDENCIES.md Guide d'installation"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "💡 Astuce : Consultez README.md pour plus d'informations !"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=================================="
    COMMAND ${CMAKE_COMMAND} -E echo ""
)

# Message final
message(STATUS "")
message(STATUS "✅ Configuration terminée !")
message(STATUS "📦 Exécutable: ${EXECUTABLE_NAME}")
message(STATUS "🚀 Pour compiler: make")
message(STATUS "▶️  Pour lancer: ./${EXECUTABLE_PATH}")
message(STATUS "💡 Pour l'aide: make project-help")
message(STATUS "")
