cmake_minimum_required(VERSION 3.19)
project(1OS_Calculator LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(1OS_Calculator
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    app.rc
    googleauthmanager.h
    googleauthmanager.cpp
        googledrivebackup.h
        googledrivebackup.cpp
        calculatorengine.h
        calculatorengine.cpp
        historymanager.h
        historymanager.cpp
        currencyservice.h
        currencyservice.cpp
        themeservice.h
        themeservice.cpp
    )

target_link_libraries(1OS_Calculator
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

# ── Bundle the OAuth client config next to the executable ──────────
# client_secret.json must ship alongside the binary so
# GoogleAuthManager::loadClientConfig() can find it at runtime.
set(OAUTH_CLIENT_JSON "${CMAKE_CURRENT_SOURCE_DIR}/client_secret.json")
if(EXISTS "${OAUTH_CLIENT_JSON}")
    add_custom_command(TARGET 1OS_Calculator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OAUTH_CLIENT_JSON}"
                "$<TARGET_FILE_DIR:1OS_Calculator>/client_secret.json"
        COMMENT "Copying Google OAuth client_secret.json next to executable"
    )
    install(FILES "${OAUTH_CLIENT_JSON}" DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    message(WARNING "client_secret.json not found at ${OAUTH_CLIENT_JSON} — "
                     "Google Sign-In will be disabled until you add it. "
                     "Rename your downloaded OAuth client file to exactly "
                     "'client_secret.json' and place it next to CMakeLists.txt.")
endif()

install(TARGETS 1OS_Calculator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET 1OS_Calculator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
