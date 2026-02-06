cmake_minimum_required(VERSION 3.20)

project(CommandShift LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/Info.plist")

set(COMMANDSHIFT_DEVELOPMENT_TEAM "" CACHE STRING
    "Apple Developer Team ID used by the generated Xcode project")

set(APP_SOURCES
    src/main.mm
    src/keypresscatcher.mm
)

set(APP_HEADERS
    src/constants.h
    src/keypresscatcher.h
)

set(APP_RESOURCES
    src/icons/icon_white.icns
    src/icons/icon_20.png
    src/icons/icon_40.png
    src/icons/icon_60.png
)

add_executable(CommandShift MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${APP_RESOURCES}
)

target_compile_options(CommandShift PRIVATE
    $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
)

set_target_properties(CommandShift PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/src/Info.plist"
)

set_target_properties(CommandShift PROPERTIES
    XCODE_GENERATE_SCHEME YES
    XCODE_SCHEME_BUILD_CONFIGURATION "Release"
    XCODE_SCHEME_RUN_CONFIGURATION "Release"
    XCODE_SCHEME_PROFILE_CONFIGURATION "Release"
    XCODE_SCHEME_ARCHIVE_CONFIGURATION "Release"
    XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
    XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/src/CommandShift.entitlements"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[config=Release] "dwarf-with-dsym"
    XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[config=Release] "YES"
    XCODE_ATTRIBUTE_COPY_PHASE_STRIP[config=Release] "NO"
)

if(COMMANDSHIFT_DEVELOPMENT_TEAM)
    set_target_properties(CommandShift PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${COMMANDSHIFT_DEVELOPMENT_TEAM}"
    )
endif()

target_link_libraries(CommandShift
    "-framework Cocoa"
    "-framework ApplicationServices"
    "-framework UserNotifications"
    "-framework Carbon"
)

set_target_properties(CommandShift PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "vasybaran.loveFromUkraine.CommandShift"
)

set_source_files_properties(${APP_RESOURCES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)
