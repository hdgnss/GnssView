cmake_minimum_required(VERSION 3.24)

if(CMAKE_HOST_WIN32)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

project(GnssView VERSION 0.1.0 LANGUAGES CXX)

set(HDGNSS_GITHUB_OWNER "Hdgnss" CACHE STRING
    "GitHub repository owner used by the in-app update checker")
set(HDGNSS_GITHUB_REPO "GnssView" CACHE STRING
    "GitHub repository name used by the in-app update checker")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/hdgnss_config.h"
    @ONLY
)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

# On macOS, respect the caller's requested architecture. The default preset
# builds natively on Apple Silicon with arm64.

option(HDGNSS_ENABLE_FTDI "Enable FTDI D2XX/libMPSSE SPI integration" ON)
set(HDGNSS_BUNDLE_IDENTIFIER "net.hdgnss.GnssView" CACHE STRING
    "macOS bundle identifier for GnssView")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    SerialPort
    Widgets
)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

# Recent macOS SDKs no longer provide AGL. Some Qt/OpenGL wrapper setups still
# propagate it through WrapOpenGL, so strip it to keep Qt Quick linking working.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(WRAPOPENGL_LIBS WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(WRAPOPENGL_LIBS)
        list(REMOVE_ITEM WRAPOPENGL_LIBS "/System/Library/Frameworks/AGL.framework")
        list(REMOVE_ITEM WRAPOPENGL_LIBS "AGL")
        set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
            INTERFACE_LINK_LIBRARIES "${WRAPOPENGL_LIBS}"
        )
    endif()
endif()

set(HDGNSS_SOURCES
    app/main.cpp
    src/core/AppController.cpp
    src/core/AppSettings.cpp
    src/core/AutomationPluginLoader.cpp
    src/core/BinaryProtocolRouter.cpp
    src/core/BuiltinProtocolRegistry.cpp
    src/core/FilePacketizer.cpp
    src/core/ProtocolDispatcher.cpp
    src/core/ProtocolPluginLoader.cpp
    src/core/StreamChunker.cpp
    src/core/TecPluginLoader.cpp
    src/core/TransportPluginLoader.cpp
    src/core/TransportViewModel.cpp
    src/core/UpdateChecker.cpp
    src/models/RawLogModel.cpp
    src/models/SatelliteModel.cpp
    src/models/SignalModel.cpp
    src/models/CommandButtonModel.cpp
    src/models/DeviationMapModel.cpp
    src/protocols/NmeaProtocolPlugin.cpp
    src/storage/RawRecorder.cpp
    src/tec/TecMapOverlayModel.cpp
    src/tec/TecMapRenderer.cpp
    src/transports/ITransport.cpp
    src/transports/SerialTransport.cpp
    src/transports/TcpClientTransport.cpp
    src/transports/UdpServerTransport.cpp
    src/utils/ByteUtils.cpp
)

if(APPLE)
    list(APPEND HDGNSS_SOURCES src/platform/MacWindowStyler_mac.mm)
else()
    list(APPEND HDGNSS_SOURCES src/platform/MacWindowStyler.cpp)
endif()

if(WIN32)
    list(APPEND HDGNSS_SOURCES
        assets/GnssView.rc
    )
endif()

set(HDGNSS_HEADERS
    src/platform/MacWindowStyler.h
    src/core/AppController.h
    src/core/AppSettings.h
    src/core/BinaryProtocolRouter.h
    src/core/BuiltinProtocolRegistry.h
    src/core/FilePacketizer.h
    src/core/ProtocolDispatcher.h
    src/core/ProtocolPluginLoader.h
    src/core/PluginMetadata.h
    src/core/StreamChunker.h
    src/core/TecPluginLoader.h
    src/core/TransportPluginLoader.h
    src/core/TransportViewModel.h
    src/core/UpdateChecker.h
    include/hdgnss/IConfigurablePlugin.h
    include/hdgnss/GnssTypes.h
    include/hdgnss/IPluginMetadata.h
    include/hdgnss/ITecDataPlugin.h
    include/hdgnss/IProtocolPlugin.h
    include/hdgnss/IPluginSettingsUi.h
    include/hdgnss/ITransport.h
    include/hdgnss/ITransportPlugin.h
    include/hdgnss/TecTypes.h
    src/models/RawLogModel.h
    src/models/SatelliteModel.h
    src/models/SignalModel.h
    src/models/CommandButtonModel.h
    src/models/DeviationMapModel.h
    src/protocols/GnssTypes.h
    src/protocols/IProtocolPlugin.h
    src/protocols/NmeaProtocolPlugin.h
    src/storage/RawRecorder.h
    src/tec/TecMapOverlayModel.h
    src/tec/TecMapRenderer.h
    src/tec/TecTypes.h
    src/transports/ITransport.h
    src/transports/SerialTransport.h
    src/transports/TcpClientTransport.h
    src/transports/UdpServerTransport.h
    src/utils/ByteUtils.h
)

qt_add_executable(GnssView
    ${HDGNSS_SOURCES}
    ${HDGNSS_HEADERS}
)

if(WIN32)
    set_target_properties(GnssView PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

add_executable(GnssViewStreamChunkerRegression
    tests/StreamChunkerRegression.cpp
    src/core/StreamChunker.cpp
    src/protocols/NmeaProtocolPlugin.cpp
)

add_executable(GnssViewRegression
    tests/GnssViewRegression.cpp
    include/hdgnss/ITransport.h
    src/core/AppController.cpp
    src/core/AppSettings.cpp
    src/core/AutomationPluginLoader.cpp
    src/core/BinaryProtocolRouter.cpp
    src/core/BuiltinProtocolRegistry.cpp
    src/core/FilePacketizer.cpp
    src/core/ProtocolDispatcher.cpp
    src/core/ProtocolPluginLoader.cpp
    src/core/StreamChunker.cpp
    src/core/TecPluginLoader.cpp
    src/core/TransportPluginLoader.cpp
    src/core/TransportViewModel.cpp
    src/core/UpdateChecker.cpp
    src/models/RawLogModel.cpp
    src/models/SatelliteModel.cpp
    src/models/SignalModel.cpp
    src/models/CommandButtonModel.cpp
    src/models/DeviationMapModel.cpp
    src/protocols/NmeaProtocolPlugin.cpp
    src/storage/RawRecorder.cpp
    src/tec/TecMapOverlayModel.cpp
    src/tec/TecMapRenderer.cpp
    src/transports/ITransport.cpp
    src/transports/SerialTransport.cpp
    src/transports/TcpClientTransport.cpp
    src/transports/UdpServerTransport.cpp
    src/utils/ByteUtils.cpp
)

target_include_directories(GnssViewStreamChunkerRegression PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GnssViewStreamChunkerRegression PRIVATE
    Qt6::Core
)

target_include_directories(GnssViewRegression PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GnssViewRegression PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::SerialPort
    Qt6::Widgets
)

target_compile_definitions(GnssViewRegression PRIVATE
    HDGNSS_REGRESSION_TESTS=1
)

set_target_properties(GnssView PROPERTIES
    QT_QML_MODULE_NO_IMPORT_SCAN TRUE
)

if(APPLE)
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/GnssView.icns"
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(GnssView PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/GnssView.icns"
    )
    set_target_properties(GnssView PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "GnssView"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${HDGNSS_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_ICON_FILE "GnssView.icns"
    )
endif()

target_compile_definitions(GnssView PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060500
    $<$<BOOL:${HDGNSS_ENABLE_FTDI}>:HDGNSS_ENABLE_FTDI=1>
)

target_include_directories(GnssView PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GnssView PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::SerialPort
    Qt6::Widgets
)

if(WIN32)
    target_link_libraries(GnssView PRIVATE dwmapi)
endif()

if(APPLE)
    # Skip the optional Mimer SQL driver during deployment because this local
    # Qt installation references a missing libmimerapi.dylib.
    qt_import_plugins(GnssView EXCLUDE Qt::QMimerSQLDriverPlugin)
endif()

file(GLOB_RECURSE HDGNSS_QML_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml/*"
)
file(GLOB_RECURSE HDGNSS_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*"
)

add_custom_target(GnssViewRuntimeAssets ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:GnssView>/qml"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GnssView>/qml"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml"
            "$<TARGET_FILE_DIR:GnssView>/qml"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:GnssView>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GnssView>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:GnssView>/assets"
    DEPENDS
        ${HDGNSS_QML_FILES}
        ${HDGNSS_ASSET_FILES}
    VERBATIM
)

add_dependencies(GnssView GnssViewRuntimeAssets)

if(WIN32)
    find_program(HDGNSS_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt.exe
        HINTS
            "${Qt6_DIR}/../../../bin"
            "${Qt6_DIR}/../../bin"
            "$ENV{QTDIR}/bin"
            "$ENV{Qt6_DIR}/../../../bin"
    )

    if(HDGNSS_WINDEPLOYQT_EXECUTABLE)
        add_custom_target(GnssViewDeployQt
            COMMAND "${HDGNSS_WINDEPLOYQT_EXECUTABLE}"
                --dir "$<TARGET_FILE_DIR:GnssView>"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml"
                --verbose 0
                "$<TARGET_FILE:GnssView>"
            DEPENDS
                GnssView
                GnssViewRuntimeAssets
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found. Build will succeed, but Qt runtime deployment is disabled.")
    endif()
endif()

if(APPLE)
    install(TARGETS GnssView
        BUNDLE DESTINATION .
    )

    qt_generate_deploy_app_script(
        TARGET GnssView
        OUTPUT_SCRIPT HDGNSS_DEPLOY_QML_APP_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_TOOL_OPTIONS
            -qmldir=${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml
    )
    install(SCRIPT "${HDGNSS_DEPLOY_QML_APP_SCRIPT}")

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_NAME "GnssView")
    set(CPACK_PACKAGE_VENDOR "HDGNSS")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME
        "GnssView-${PROJECT_VERSION}-macOS-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_DMG_VOLUME_NAME "GnssView")
    include(CPack)

    add_custom_target(GnssViewDmg
        COMMENT "Building GnssView DMG package"
    )
    add_dependencies(GnssViewDmg package)
endif()
