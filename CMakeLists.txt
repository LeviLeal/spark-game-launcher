cmake_minimum_required(VERSION 3.16)
project(Spark VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(spark
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/models/game.h
    src/utils/runnerdetector.h
    src/utils/runnerdetector.cpp
    src/utils/iconhelper.h
    src/utils/iconhelper.cpp
    src/utils/librarymanager.h
    src/utils/librarymanager.cpp
    src/utils/processmanager.h
    src/utils/processmanager.cpp
    src/dialogs/gameconfigdialog.h
    src/dialogs/gameconfigdialog.cpp
    src/dialogs/logviewerdialog.h
    src/dialogs/logviewerdialog.cpp
    src/dialogs/settingsdialog.h
    src/dialogs/settingsdialog.cpp
    src/dialogs/generalsettingswidget.h
    src/dialogs/generalsettingswidget.cpp
    src/dialogs/protondownloadwidget.h
    src/dialogs/protondownloadwidget.cpp
    resources.qrc
)

target_link_libraries(spark PRIVATE Qt6::Widgets Qt6::Network)

add_custom_target(tarball
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-tarball.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Gerando pacote portatil .tar.xz do Spark"
    DEPENDS spark
)
