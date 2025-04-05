cmake_minimum_required(VERSION 3.16)
project(ExamBankSystem LANGUAGES CXX)

set(VBOX_SDK_DIR ${CMAKE_SOURCE_DIR}/sdk) 

include(qt.cmake)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://gitcode.com/gh_mirrors/cp/cpp-httplib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cpp-httplib)

if(WIN32)
    execute_process(COMMAND where VirtualBox
        OUTPUT_VARIABLE VBOX_EXECUTABLE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND which VirtualBox
        OUTPUT_VARIABLE VBOX_EXECUTABLE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
get_filename_component(VBOX_INSTALL_DIR ${VBOX_EXECUTABLE} DIRECTORY)

if(WIN32)
    set(VBOX_INCLUDES
        ${VBOX_SDK_DIR}/bindings/mscom/include
        ${VBOX_SDK_DIR}/bindings/mscom/lib
    )
else()
    set(VBOX_INCLUDES
        ${VBOX_SDK_DIR}/bindings/xpcom/include
        ${VBOX_SDK_DIR}/bindings/xpcom/lib
    )
    find_library(VBOX_XPCOM_LIB NAMES vboxxpcom)
    set(VBOX_LIBS
        ${VBOX_XPCOM_LIB}
        pthread
    )
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickWidgets
        QuickControls2
)
qt_standard_project_setup()

set(PROJECT_SOURCES
    src/main.cpp
    src/gui/main/ExamBankSystem.cpp
    src/gui/main/ExamBankSystem.h
    src/gui/main/ExamBankSystem.ui
    src/core/env/ExamEnvManager.cpp
    src/core/env/ExamEnvManager.h
    src/core/virtualbox/VirtualBoxController.cpp
    src/core/virtualbox/VirtualBoxController.h
    src/core/virtualbox/LoadingManager.cpp
    src/core/virtualbox/LoadingManager.h
    src/gui/exam/PageExam.cpp
    src/gui/exam/PageExam.h
    src/gui/exam/PageExam.ui
    src/gui/examEnv/PageExamEnv.cpp
    src/gui/examEnv/PageExamEnv.h
    src/gui/examEnv/PageExamEnv.ui
    src/gui/special/PageSpecial.cpp
    src/gui/special/PageSpecial.h
    src/gui/special/PageSpecial.ui
    sdk/bindings/mscom/lib/VirtualBox_i.c
)

set(RESOURCE_FILES
    ExamBankSystem.qrc
)

qt_add_resources(APP_RESOURCES ${RESOURCE_FILES})

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${APP_RESOURCES})

configure_file(config.ini ${CMAKE_CURRENT_BINARY_DIR}/config.ini COPYONLY)

qt_add_qml_module(${PROJECT_NAME}
    URI ExamBankSystem
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        qml/Loading.qml
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${VBOX_INCLUDES}
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Qml
        Qt::Quick
        Qt::QuickWidgets
        Qt::QuickControls2
        ${VBOX_LIBS} 
        ole32
        oleaut32
        rpcrt4 
        uuid
        httplib  # 添加cpp-httplib库链接
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_DCOM
        _ATL_STATIC_REGISTRY
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${VBOX_INSTALL_DIR}/VBoxC.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
else()
    find_program(VBOX_XPIDL_EXECUTABLE xpidl PATHS ${VBOX_INSTALL_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${VBOX_XPIDL_EXECUTABLE} -m typelib -w -v -I${VBOX_SDK_DIR}/bindings/xpcom/idl ${VBOX_SDK_DIR}/bindings/xpcom/idl/VirtualBox_XPCOM.idl
    )
endif()