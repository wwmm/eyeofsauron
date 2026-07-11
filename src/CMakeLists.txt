configure_file(config.h.in config.h @ONLY)

add_subdirectory(contents)

add_executable(eyeofsauron)

kde_target_enable_exceptions(eyeofsauron PRIVATE)

kconfig_add_kcfg_files(eyeofsauron GENERATE_MOC ${KCFGC_FILES})

qt_policy(SET QTP0004 NEW)

set_source_files_properties(
    contents/ui/Common.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(eyeofsauron
    URI eos.ui
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    SOURCES
        tracker.hpp
    QML_FILES
        contents/ui/ChartMouseArea.qml
        contents/ui/Common.qml
        contents/ui/EoSSpinBox.qml
        contents/ui/EoSSwitch.qml
        contents/ui/Main.qml
        contents/ui/PreferencesPage.qml
        contents/ui/SoundWave.qml
        contents/ui/SourceMenu.qml
        contents/ui/Tracker.qml
)

target_sources(eyeofsauron PRIVATE
    frame_source.cpp
    io_device.cpp
    main.cpp
    sound_wave.cpp
    tracker.cpp
    util.cpp
    resources.qrc
)

target_include_directories(eyeofsauron PRIVATE 
    ${OpenCV_INCLUDE_DIRS} 
    ${FFTW3_INCLUDE_DIRS} 
    ${LIBV4L2_INCLUDE_DIRS} 
    ${LIBMEDIAINFO_INCLUDE_DIRS}
)

target_link_libraries(eyeofsauron PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Charts
    Qt6::Multimedia
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::IconThemes
    ${OpenCV_LIBS}
    ${FFTW3_LIBRARIES}
    ${LIBV4L2_LIBRARIES}
    ${LIBMEDIAINFO_LIBRARIES}
)


# install(FILES ${KCFG_FILES} DESTINATION ${KDE_INSTALL_KCFGDIR})

install(TARGETS eyeofsauron ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})