list(APPEND FILE_TYPES_SRC
	"CsvFile.cpp"
	"IniFile.cpp"
	"MsdFile.cpp"
	"XmlFile.cpp"
	"XmlToLua.cpp"
	"XmlFileUtil.cpp")

list(APPEND FILE_TYPES_HPP
	"CsvFile.h"
	"IniFile.h"
	"MsdFile.h"
	"XmlFile.h"
	"XmlToLua.h"
	"XmlFileUtil.h")

target_sources(Etterna PUBLIC ${FILE_TYPES_SRC} ${FILE_TYPES_HPP})
