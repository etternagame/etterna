; Included by the NSIS installer script
; Don't forget to also change ProductInfo.h!

; a friendly string to refer to the product
!define PRODUCT_FAMILY "Etterna"

!define PRODUCT_ID "Etterna"
; TODO: This needs to be updated with the git rev hash
!define PRODUCT_VER "v0.69.1"
!define PRODUCT_DISPLAY "${PRODUCT_FAMILY} ${PRODUCT_VER}"
!define PRODUCT_BITMAP "ett"

!define PRODUCT_URL "https://github.com/etternagame/etterna"
!define UPDATES_URL "https://github.com/etternagame/etterna"

;!define INSTALL_EXTERNAL_PCKS
;!define INSTALL_INTERNAL_PCKS
!define INSTALL_PROGRAM_LIBRARIES
!define INSTALL_EXECUTABLES
!define INSTALL_NON_PCK_FILES
!define ASSOCIATE_SMZIP
!define ASSOCIATE_SMURL
;!define SHOW_AUTORUN
!define MAKE_OPEN_PROGRAM_FOLDER_SHORTCUT
!define MAKE_OPEN_SETTINGS_FOLDER_SHORTCUT
;!define DIRECTX_81_REDIST_PRESENT
;!define MAKE_UPDATES_SHORTCUT
!define CRC_CHECK
!define COMPRESS
