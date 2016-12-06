# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_plymouth\")

configure_file (config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )


set(kcm_plymouth_SRCS
  kcm.cpp
)

add_library(kcm_plymouth MODULE ${kcm_plymouth_SRCS})

target_link_libraries(kcm_plymouth
  KF5::CoreAddons
  KF5::KCMUtils
  KF5::I18n
  KF5::Declarative
  KF5::QuickAddons
  KF5::NewStuff

  Qt5::Widgets
  ${X11_LIBRARIES}
)

kcoreaddons_desktop_to_json(kcm_plymouth "kcm_plymouth.desktop" SERVICE_TYPES kcmodule.desktop)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES kcm_plymouth.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS kcm_plymouth DESTINATION ${PLUGIN_INSTALL_DIR}/kcms)
install(FILES plymouth.knsrc DESTINATION ${KDE_INSTALL_CONFDIR})

kpackage_install_package(package kcm_plymouth kcms)
