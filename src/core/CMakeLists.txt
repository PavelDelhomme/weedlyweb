# Core partagé WeedlyWeb (sans dépendance UI)
add_library(WeedlyWebCore STATIC
    database/Database.cpp
    managers/HTTPManager.cpp
    managers/FileManager.cpp
    managers/TabsManager.cpp
    utils/CVEAnalyzer.cpp
    utils/FavoritesJson.cpp
    utils/PageEnhancements.cpp
)

target_include_directories(WeedlyWebCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${NLOHMANN_JSON_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(WeedlyWebCore PUBLIC
    ${CURL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

target_compile_features(WeedlyWebCore PUBLIC cxx_std_17)
