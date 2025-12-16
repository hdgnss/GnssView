cmake_minimum_required(VERSION 3.16)

project(GnssView VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Network PrintSupport Charts)

# Enable automoc, autouic, and autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/ui/settingsdialog.cpp
    src/ui/cmdbuttondialog.cpp
    src/core/communicationmanager.cpp
    src/core/nmeaparser.cpp
    src/core/datalogger.cpp
    src/core/updatechecker.cpp
    src/core/ntripclient.cpp
    resources/resources.qrc
)

qt_standard_project_setup()

# Platform specific settings
if (WIN32)
    set(APP_ICONS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.rc")
    # Set WIN32 flag for Windows builds to create a GUI application without a console window
    add_executable(GnssView WIN32 ${PROJECT_SOURCES} ${APP_ICONS})
elseif (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE app.icns)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND PROJECT_SOURCES ${APP_ICON_MACOS} Info.plist)
    add_executable(GnssView MACOSX_BUNDLE ${PROJECT_SOURCES})
    set_target_properties(GnssView PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
else()
    # Linux/other platform build
    add_executable(GnssView ${PROJECT_SOURCES})
endif()

target_include_directories(GnssView PRIVATE
    ${PROJECT_BINARY_DIR}
    src
    src/core
    src/ui
)

target_link_libraries(GnssView PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Network
    Qt6::PrintSupport
    Qt6::Charts
)

