list(APPEND GLOBAL_FILES_SRC
	"MinaCalc.cpp"
	"GameLoop.cpp"
	"global.cpp"
	"SpecialFiles.cpp"
	"StepMania.cpp") # TODO: Refactor into separate main project.

list(APPEND GLOBAL_FILES_HPP
	"MinaCalc.h"
	"GameLoop.h"
	"global.h"
	"OptionsBinding.h"
	"ProductInfo.h" # TODO: Have this be auto-generated.
	"SpecialFiles.h"
	"StdString.h" # TODO: Remove the need for this file, transition to std::string.
	"StepMania.h" # TODO: Refactor into separate main project.
	"picosha2.h")

if(NOT APPLE)
	list(APPEND GLOBAL_FILES_SRC "Main.cpp")
endif()

target_sources(Etterna PUBLIC ${GLOBAL_FILES_SRC} ${GLOBAL_FILES_HPP})
