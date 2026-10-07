cmake_minimum_required(VERSION 3.16)
project(Qt_chat VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

set(SOURCES_SERVER
    Network_Node.h
    Chat_Server.h
    chat_server.cpp
)

set(SOURCES_CLIENT
    Network_Node.h
    Chat_Client.h
    ui/app_client_login.ui
    ui/app_client_chat.ui
    app_client.h
    app_client.cpp
)

add_executable(${PROJECT_NAME}_server WIN32 ${SOURCES_SERVER})
add_executable(${PROJECT_NAME}_client WIN32 ${SOURCES_CLIENT})

target_link_libraries(${PROJECT_NAME}_server PRIVATE
    Qt6::Core
    Qt6::Network
)

target_link_libraries(${PROJECT_NAME}_client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
