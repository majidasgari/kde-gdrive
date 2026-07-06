cmake_minimum_required(VERSION 3.24)

project(nimbus-gdrive
    VERSION 0.1.0
    DESCRIPTION "A stable, fast rclone-based Google Drive client"
    LANGUAGES CXX
)

# --- KDE ECM (Extra CMake Modules) ------------------------------------------
# Must be found before KDE Frameworks packages, as it provides the macros.
find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# KDE tooling: C++ standard, transitive Qt/KF handling, install dirs, ...
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(FeatureSummary)

# --- Qt 6 -------------------------------------------------------------------
find_package(Qt6 6.6.0 REQUIRED COMPONENTS
    Core
    Gui
    Network
    Qml
    Quick
    QuickControls2
    Widgets
)

# --- KDE Frameworks 6 -------------------------------------------------------
find_package(KF6 6.0.0 REQUIRED COMPONENTS
    CoreAddons
    I18n
    Config
    DBusAddons
    StatusNotifierItem
    Kirigami
)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX NIMBUS_GDRIVE
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/nimbus-gdrive-version.h"
)

# --- App data dir (icons / desktop) -----------------------------------------
add_subdirectory(data)
add_subdirectory(src)

# License installed to share/doc/
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}
        RENAME LICENSE)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
