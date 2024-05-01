install(FILES 
    icons/eyeofsauron.svg 
    DESTINATION ${KDE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps
)

install(PROGRAMS 
    com.github.wwmm.eyeofsauron.desktop 
    DESTINATION ${KDE_INSTALL_APPDIR}
)