list(APPEND SMDATA_FILE_TYPES_SRC
  "Etterna/FileTypes/CsvFile.cpp"
  "Etterna/FileTypes/IniFile.cpp"
  "Etterna/FileTypes/MsdFile.cpp"
  "Etterna/FileTypes/XmlFile.cpp"
  "Etterna/FileTypes/XmlToLua.cpp"
  "Etterna/FileTypes/XmlFileUtil.cpp"
)
list(APPEND SMDATA_FILE_TYPES_HPP
  "Etterna/FileTypes/CsvFile.h"
  "Etterna/FileTypes/IniFile.h"
  "Etterna/FileTypes/MsdFile.h"
  "Etterna/FileTypes/XmlFile.h"
  "Etterna/FileTypes/XmlToLua.h"
  "Etterna/FileTypes/XmlFileUtil.h"
)

source_group("File Types" FILES ${SMDATA_FILE_TYPES_SRC} ${SMDATA_FILE_TYPES_HPP})
