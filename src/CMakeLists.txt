# --- Application executable -------------------------------------------------

add_executable(nimbus-gdrive
    main.cpp

    # core (rclone engine layer)
    core/RcloneLocator.cpp
    core/RcloneLocator.h
    core/RcloneDaemon.cpp
    core/RcloneDaemon.h
    core/RcloneApiClient.cpp
    core/RcloneApiClient.h
    core/MountManager.cpp
    core/MountManager.h

    # models
    models/RemotesModel.cpp
    models/RemotesModel.h
    models/MountsModel.cpp
    models/MountsModel.h

    # sync
    sync/BisyncManager.cpp
    sync/BisyncManager.h
    sync/LocalWatcher.cpp
    sync/LocalWatcher.h

    # config
    config/Settings.cpp
    config/Settings.h

    # ui (C++ controllers behind QML)
    ui/TrayIcon.cpp
    ui/TrayIcon.h
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/OAuthWizard.cpp
    ui/OAuthWizard.h
)

# QML/Qt resources (icons + .qml files)
qt_add_resources(nimbus-gdrive "kdegdrive"
    PREFIX "/"
    FILES
        qml/MainWindow.qml
        qml/OAuthWizard.qml
        qml/SettingsPage.qml
        qml/RemoteDetailPage.qml
        qml/AboutPage.qml
        logo.png
)

target_include_directories(nimbus-gdrive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(nimbus-gdrive PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets

    KF6::CoreAddons
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::DBusAddons
    KF6::StatusNotifierItem
    # Kirigami is used from QML (imported at runtime), no C++ link needed
    # for the current MVP.
)

# Make QML sources visible to the QML linter / IDE.
set_target_properties(nimbus-gdrive PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    AUTOMOC ON
)

install(TARGETS nimbus-gdrive ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
