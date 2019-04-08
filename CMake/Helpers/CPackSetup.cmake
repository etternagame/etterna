## CPack Setup
set(CPACK_PACKAGE_VENDOR "Etterna Team")
set(CMAKE_PACKAGE_DESCRIPTION "Advanced cross-platform rhythm game focused on keyboard play")
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/CMake/CPack/license_install.txt) # Only used on windows for NSIS
set(CPACK_COMPONENT_ETTERNA_REQUIRED TRUE)  # Require Etterna component to be installed


# Univeral CMake install lines
install(DIRECTORY Announcers            COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY Assets                COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY BackgroundEffects     COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY BackgroundTransitions COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY BGAnimations          COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY Characters            COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY Data                  COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY NoteSkins             COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY Scripts               COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY Songs                 COMPONENT Etterna DESTINATION Etterna)
install(DIRECTORY Themes                COMPONENT Etterna DESTINATION Etterna)
install(FILES portable.ini              COMPONENT Etterna DESTINATION Etterna)

############################################################################################################
############## WINDOWS ONLY CMAKE ##########################################################################
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    SET(CPACK_NSIS_INSTALL_ROOT "C:\\\\Games") # Default install directory
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "Etterna\\\\Program")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "Etterna.exe")
    set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/Installer/Install.ico)
    set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${PROJECT_SOURCE_DIR}/Installer/welcome-ett.bmp)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

    ## Switch the strings below to use backslashes. NSIS requires it for those variables in particular. Copied from original script.
    string(REGEX REPLACE "/" "\\\\\\\\" CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP}")

    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "!define MUI_HEADERIMAGE_BITMAP ${MUI_HEADERIMAGE_BITMAP}")

    install(TARGETS Etterna     COMPONENT Etterna DESTINATION "Etterna\\\\Program")
    install(DIRECTORY Program   COMPONENT Etterna DESTINATION Etterna)
    install(FILES CMake/CPack/license_install.txt COMPONENT Etterna DESTINATION Etterna/Docs)

############################################################################################################
############## UNIX ONLY CMAKE #############################################################################
elseif(APPLE)
    # CPack Packaging
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_DMG_VOLUME_NAME Etterna)    

    install(TARGETS Etterna COMPONENT Etterna DESTINATION Etterna)
endif()


