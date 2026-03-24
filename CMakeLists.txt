cmake_minimum_required(VERSION 3.16)
project(RailMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql)

qt6_add_executable(RailMonitor
    main.cpp
    database/DatabaseManager.cpp database/DatabaseManager.h
    network/NetworkManager.cpp network/NetworkManager.h
    ui/MainWindow.cpp ui/MainWindow.h
    ui/NetworkConfigPage.cpp ui/NetworkConfigPage.h
    ui/RailMapViewerWidget.cpp ui/RailMapViewerWidget.h
    ui/ShoeCabinetPage.cpp ui/ShoeCabinetPage.h

    ui/TieShoePage.cpp ui/TieShoePage.h
    protocol/ProtocolPacket.cpp
    protocol/TimeSyncRequest.h protocol/TimeSyncRequest.cpp
    protocol/ProtocolPacket.h
    protocol/TimeSyncResponse.h protocol/TimeSyncResponse.cpp
    protocol/WhitelistSync.h protocol/WhitelistSync.cpp
    protocol/DeviceParser.h protocol/DeviceParser.cpp
    DeviceManager.h DeviceManager.cpp

    resources.qrc
)

target_include_directories(RailMonitor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RailMonitor PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
