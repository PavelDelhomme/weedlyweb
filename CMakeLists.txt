cmake_minimum_required(VERSION 3.10)
project(WeedlyWeb VERSION 1.0)

# Activer le C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

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

# Ajouter les fichiers sources
set(SOURCES
    src/main.cpp
    src/Navigateur.cpp
    src/MoteurRendu.cpp
    src/MoteurScript.cpp
    src/GestionnaireHTTP.cpp
    src/GestionnaireMemoire.cpp
    src/GestionnaireFichiers.cpp
    src/GestionnaireFavoris.cpp
)
set(HEADERS
    include/Navigateur.h
    include/MoteurRendu.h
    include/GestionnaireFavoris.h
    include/GestionnaireHTTP.h
    include/GestionnaireMemoire.h
    include/GestionnaireFichiers.h
    include/MoteurScript.h
)

# Créer l'exécutable
add_executable(WeedlyWeb ${SOURCES} ${HEADERS})

# Inclure les répertoires nécessaires
target_include_directories(WeedlyWeb PRIVATE ${WEBKIT2GTK_INCLUDE_DIRS})

# Lier les bibliothèques nécessaires
target_link_libraries(WeedlyWeb PRIVATE ${WEBKIT2GTK_LIBRARIES} gtk-3 ${CURL_LIBRARIES} nlohmann_json)

# Ajouter des commandes post-build
add_custom_command(TARGET WeedlyWeb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets/settings
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets/datas
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/settings/config.json ${CMAKE_BINARY_DIR}/assets/settings/config.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/datas/favoris.json ${CMAKE_BINARY_DIR}/assets/datas/favoris.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/styles/style.css ${CMAKE_BINARY_DIR}/assets/styles/style.css
)

# Définir le répertoire d'installation
set(INSTALL_DIR /usr/local)

# Ajouter une cible install pour l'exécutable
install(TARGETS WeedlyWeb RUNTIME DESTINATION ${INSTALL_DIR}/bin)

# Installer les fichiers de configuration et styles
install(FILES
    ${CMAKE_SOURCE_DIR}/assets/settings/config.json
    ${CMAKE_SOURCE_DIR}/assets/datas/favoris.json
    ${CMAKE_SOURCE_DIR}/assets/styles/style.css
    DESTINATION ${INSTALL_DIR}/share/WeedlyWeb
)


# Ajouter une cible de nettoyage sécurisée (renommée pour éviter les conflits avec `make clean`)
add_custom_target(custom_clean ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/WeedlyWeb
    COMMENT "Nettoyage sécurisé du répertoire de build sans supprimer les dépendances"
)