# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_plymouth\")

SET(PLYMOUTH_THEMES_DIR "/usr/share/plymouth/themes/" CACHE STRING "Where Plymouth themes are installed")
SET(PLYMOUTH_CONFIG_PATH "/etc/plymouth/plymouthd.conf" CACHE STRING "where the main plymouth config file is")
configure_file (config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )

set(kcm_plymouth_SRCS
  kcm.cpp kcm.h
)

add_library(kcm_plymouth MODULE ${kcm_plymouth_SRCS})

target_link_libraries(kcm_plymouth
  KF5::CoreAddons
  KF5::AuthCore
  KF5::I18n
  KF5::Declarative
  KF5::QuickAddons
  KF5::NewStuff
  KF5::NewStuffCore
  KF5::ConfigCore
  KF5::KIOCore
)

kcoreaddons_desktop_to_json(kcm_plymouth "kcm_plymouth.desktop" SERVICE_TYPES kcmodule.desktop)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES kcm_plymouth.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(TARGETS kcm_plymouth DESTINATION ${KDE_INSTALL_PLUGINDIR}/kcms)
install(FILES plymouth.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})

kpackage_install_package(package kcm_plymouth kcms)

#installer tool for knewstuff
add_executable(kplymouththemeinstaller
    kplymouththemeinstaller.cpp
)

target_link_libraries(kplymouththemeinstaller
    KF5::I18n
    KF5::AuthCore
    KF5::CoreAddons
    KF5::Archive
    KF5::ConfigCore)
install(TARGETS kplymouththemeinstaller ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

#polkit stuff
add_executable(plymouthhelper helper.cpp helper.h ${helper_mocs})
target_link_libraries(plymouthhelper
    KF5::Archive
    KF5::AuthCore
    KF5::ConfigCore
    KF5::I18n)
install(TARGETS plymouthhelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(plymouthhelper org.kde.kcontrol.kcmplymouth root)
kauth_install_actions(org.kde.kcontrol.kcmplymouth kcmplymouth_actions.actions)
