project(plymouthkcm)

cmake_minimum_required(VERSION 2.8.12)
set(KF5_MIN_VERSION "5.74")
set(QT_MIN_VERSION "5.15.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "plymouthkcm requires an out of source build. Please create a separate build directory and run 'cmake path_to_plymouth_kcm [options]' there.")
endif()

set(PROJECT_VERSION "5.20.90")
set(PROJECT_VERSION_MAJOR 5)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
if(ECM_VERSION VERSION_GREATER_EQUAL 5.79)
    include(KDEGitCommitHooks)
endif()

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
                    Archive NewStuff NewStuffCore KIO Declarative I18n Config)

find_package(Plymouth REQUIRED)

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT -DQT_NO_URL_CAST_FROM_STRING)

add_subdirectory(src)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
if(ECM_VERSION VERSION_GREATER_EQUAL 5.79)
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
