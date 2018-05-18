project(plymouthkcm)

cmake_minimum_required(VERSION 2.8.12)
set(KF5_MIN_VERSION "5.29.0")
set(QT_MIN_VERSION "5.5.0")

################# Disallow in-source build #################

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "plymouthkcm requires an out of source build. Please create a separate build directory and run 'cmake path_to_kirigami [options]' there.")
endif()



# Make CPack available to easy generate binary packages
include(CPack)
include(FeatureSummary)

################# set KDE specific information #################

set(PROJECT_VERSION "5.13.80")
set(PROJECT_VERSION_MAJOR 5)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui )

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
                    Archive NewStuff NewStuffCore KIO Declarative I18n Config ConfigWidgets)

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
################# Enable C++11 features for clang and gcc #################

if(UNIX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x")
endif()

################# now find all used packages #################

set (QT_MIN_VERSION "5.5.0")

find_package(Plymouth REQUIRED)
#########################################################################

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

