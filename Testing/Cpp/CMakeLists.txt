include(../CMake/qtTestingMacroGenerateMocs.cmake)

if(QtTesting_QT_VERSION VERSION_GREATER "5")
    find_package(Qt6 REQUIRED QUIET COMPONENTS Core Test Widgets Gui)
    set(TEST_LIBRARIES Qt6::Test Qt6::Core Qt6::Widgets Qt6::Gui)
else()
    find_package(Qt5 REQUIRED QUIET COMPONENTS Core Test Widgets Gui)
    set(TEST_LIBRARIES Qt5::Test Qt5::Core Qt5::Widgets Qt5::Gui)
endif()

set(KIT ${PROJECT_NAME})

set(TEST_SOURCES
    pqAbstractButtonEventTranslatorTest.cpp
    pqEventPlayerTest.cpp
    pqEventRecorderTest.cpp
    pqEventTranslatorTest.cpp
    pqDoubleSpinBoxEventPlayerTest.cpp
    pqDoubleSpinBoxEventTranslatorTest.cpp
    pqSpinBoxEventPlayerTest.cpp
    pqSpinBoxEventTranslatorTest.cpp
    pqTestUtilityTest.cpp
)

set(TEST_MOC_HEADERS pqTest.h)

create_test_sourcelist(Tests ${KIT}CppTests.cxx ${TEST_SOURCES})

set(TestsToRun ${Tests})
remove(TestsToRun ${KIT}CppTests.cxx)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(QtTesting_QT_VERSION VERSION_GREATER "5")
    qt6_generate_mocs(${TEST_SOURCES})
    qt6_wrap_cpp( TEST_MOC_SRCS ${TEST_MOC_HEADERS} )
else()
    qt5_generate_mocs(${TEST_SOURCES})
    qt5_wrap_cpp( TEST_MOC_SRCS ${TEST_MOC_HEADERS} )
endif()

add_executable(${KIT}CppTests ${Tests} ${TEST_MOC_SRCS})
target_link_libraries(
    ${KIT}CppTests
    ${PROJECT_NAME}
    ${TEST_LIBRARIES}
    qttesting
)
set_target_properties(
    ${KIT}CppTests
    PROPERTIES COMPILE_FLAGS "${Qt5Test_EXECUTABLE_COMPILE_FLAGS}"
)

macro(SIMPLE_TEST testname)
    add_test(
        NAME ${testname}
        COMMAND $<TARGET_FILE:${KIT}CppTests> ${testname} ${ARGN}
    )
endmacro()

#
# Add Tests
#
simple_test( pqAbstractButtonEventTranslatorTest )
simple_test( pqEventPlayerTest )
simple_test( pqEventRecorderTest )
simple_test( pqDoubleSpinBoxEventPlayerTest )
simple_test( pqDoubleSpinBoxEventTranslatorTest )
simple_test( pqSpinBoxEventPlayerTest )
simple_test( pqSpinBoxEventTranslatorTest )
simple_test( pqEventTranslatorTest )
simple_test( pqTestUtilityTest )
