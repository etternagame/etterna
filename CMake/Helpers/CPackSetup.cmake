## CPack Setup
set(CPACK_PACKAGE_VENDOR "Etterna Team")
set(CMAKE_PACKAGE_DESCRIPTION "Advanced cross-platform rhythm game focused on keyboard play")
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/CMake/CPack/license_install.txt)
set(CPACK_COMPONENT_ETTERNA_REQUIRED TRUE)  # Require Etterna component to be installed

# Custom Variables
set(INSTALL_DIR "Etterna")

# Windows Specific CPack
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_INSTALL_ROOT "C:\\\\Games") # Default install directory
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY Program)
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN Etterna.exe)
    set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/CMake/CPack/Windows/Install.ico)
    set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/CMake/CPack/Windows/Install.ico)
    set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${PROJECT_SOURCE_DIR}/CMake/CPack/Windows/welcome-ett.bmp)
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	set(CPACK_NSIS_MODIFY_PATH ON)
	set(CPACK_CREATE_DESKTOP_LINKS Etterna.exe)
    set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}\\\\CMake\\\\CPack\\\\Windows\\\\header-ett.bmp)

    ## Switch the strings below to use backslashes. NSIS requires it for those variables in particular. Copied from original script.
    string(REGEX REPLACE "/" "\\\\\\\\" CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP}")

    set(INSTALL_DIR ".")
    install(TARGETS Etterna     COMPONENT Etterna DESTINATION Program)
    install(DIRECTORY Program   COMPONENT Etterna DESTINATION .)
    install(FILES CMake/CPack/license_install.txt COMPONENT Etterna DESTINATION Docs)

# macOS Specific CPack
elseif(APPLE)
    # CPack Packaging
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_DMG_VOLUME_NAME Etterna)

    install(TARGETS Etterna COMPONENT Etterna DESTINATION Etterna)
endif()

# Universal Install Directories
install(DIRECTORY Announcers            COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY Assets                COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY BackgroundEffects     COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY BackgroundTransitions COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY BGAnimations          COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY Characters            COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY Data                  COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY NoteSkins             COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY Scripts               COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY Songs                 COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(DIRECTORY Themes                COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
install(FILES portable.ini              COMPONENT Etterna DESTINATION "${INSTALL_DIR}")
