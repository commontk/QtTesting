set(UI_FILES TestingDemo.ui)

set(SOURCE_FILES TestingDemo.cxx TestingDemo.h)

set(MOC_FILES TestingDemo.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(QtTesting_QT_VERSION VERSION_GREATER "5")
    qt6_wrap_ui(EXAMPLE_UI_BUILT_SOURCES ${UI_FILES})
    qt6_wrap_cpp(EXAMPLE_MOC_BUILT_SOURCES ${MOC_FILES})
else()
    qt5_wrap_ui(EXAMPLE_UI_BUILT_SOURCES ${UI_FILES})
    qt5_wrap_cpp(EXAMPLE_MOC_BUILT_SOURCES ${MOC_FILES})
endif()

add_executable(
    TestingDemo
    ${SOURCE_FILES}
    ${EXAMPLE_UI_BUILT_SOURCES}
    ${EXAMPLE_MOC_BUILT_SOURCES}
)

target_link_libraries(TestingDemo qttesting ${qt_imported_targets})
set_target_properties(
    TestingDemo
    PROPERTIES COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
)
