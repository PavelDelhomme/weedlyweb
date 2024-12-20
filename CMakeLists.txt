cmake_minimum_required(VERSION 3.10)
project(WeedlyWeb VERSION 1.0)

# Activer le C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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
    src/GestionnaireHTTP.cpp
)

# Créer l'exécutable
add_executable(WeedlyWeb ${SOURCES})

# Lier les bibliothèques nécessaires
target_include_directories(WeedlyWeb PRIVATE ${WEBKIT2GTK_INCLUDE_DIRS})
target_link_libraries(WeedlyWeb ${WEBKIT2GTK_LIBRARIES} gtk-3 ${CURL_LIBRARIES})
