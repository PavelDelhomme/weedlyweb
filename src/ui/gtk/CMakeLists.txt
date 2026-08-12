# Backend UI GTK + WebKit2GTK
add_executable(WeedlyWeb
    main.cpp
    browser/Browser.cpp
    rendering/RenderingEngine.cpp
    managers/FavoritesManager.cpp
    managers/MemoryManager.cpp
    engine/ScriptEngine.cpp
    utils/Utils.cpp
    utils/CommandPalette.cpp
    utils/RequestInterceptor.cpp
)

target_include_directories(WeedlyWeb PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${WEBKIT2GTK_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

target_compile_options(WeedlyWeb PRIVATE $<$<CONFIG:Debug>:-g>)

target_link_libraries(WeedlyWeb PRIVATE
    WeedlyWebCore
    ${WEBKIT2GTK_LIBRARIES}
    gtk-3
    ${CURL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

add_custom_command(TARGET WeedlyWeb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:WeedlyWeb>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/settings/config.json $<TARGET_FILE_DIR:WeedlyWeb>/assets/settings/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/datas/favorites.json $<TARGET_FILE_DIR:WeedlyWeb>/assets/datas/favorites.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/styles/style.css $<TARGET_FILE_DIR:WeedlyWeb>/assets/styles/style.css
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets/help/help.html $<TARGET_FILE_DIR:WeedlyWeb>/assets/help/help.html
)

install(TARGETS WeedlyWeb RUNTIME DESTINATION bin)
