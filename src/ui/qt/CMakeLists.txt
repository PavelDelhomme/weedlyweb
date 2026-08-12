# Backend UI Qt6 + Qt WebEngine
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets)

add_executable(WeedlyWebQt
    src/qt/main.cpp
    src/qt/BrowserWindow.cpp
    src/qt/WebView.cpp
    src/qt/TabBar.cpp
    src/qt/FavoritesBar.cpp
    src/qt/NavigationBar.cpp
    src/qt/CommandPalette.cpp
    include/qt/BrowserWindow.h
    include/qt/WebView.h
    include/qt/TabBar.h
    include/qt/FavoritesBar.h
    include/qt/NavigationBar.h
    include/qt/CommandPalette.h
)

target_include_directories(WeedlyWebQt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

target_link_libraries(WeedlyWebQt PRIVATE
    WeedlyWebCore
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
)

add_custom_command(TARGET WeedlyWebQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:WeedlyWebQt>/assets
)

install(TARGETS WeedlyWebQt RUNTIME DESTINATION bin)
