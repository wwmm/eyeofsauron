configure_file(config.h.in config.h)

add_subdirectory(contents)

add_executable(eyeofsauron)

kde_target_enable_exceptions(eyeofsauron PRIVATE)

target_sources(eyeofsauron PRIVATE
    frame_source.cpp
    main.cpp
    tracker.cpp
    util.cpp
    resources.qrc
)

target_include_directories(eyeofsauron PRIVATE ${OpenCV_INCLUDE_DIRS} ${LIBV4L2_INCLUDE_DIRS} ${LIBMEDIAINFO_INCLUDE_DIRS})

target_link_libraries(eyeofsauron PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Quick
    Qt${QT_MAJOR_VERSION}::Qml
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::QuickControls2
    Qt${QT_MAJOR_VERSION}::Widgets
    Qt${QT_MAJOR_VERSION}::Charts
    Qt${QT_MAJOR_VERSION}::Multimedia
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigCore
    KF${QT_MAJOR_VERSION}::ConfigGui
    ${OpenCV_LIBS}
    ${LIBV4L2_LIBRARIES}
    ${LIBMEDIAINFO_LIBRARIES}
)

kconfig_add_kcfg_files(eyeofsauron GENERATE_MOC ${KCFGC_FILES})

# install(FILES ${KCFG_FILES} DESTINATION ${KDE_INSTALL_KCFGDIR})

install(TARGETS eyeofsauron ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})