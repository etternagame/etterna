list(APPEND SMDATA_GLOBAL_FILES_SRC
  "Etterna/Globals/GameLoop.cpp"
  "Etterna/Globals/global.cpp"
  "Etterna/Globals/SpecialFiles.cpp"
  "Etterna/Globals/StepMania.cpp" # TODO: Refactor into separate main project.
  "${SM_SRC_DIR}/generated/verstub.cpp"
)

list(APPEND SMDATA_GLOBAL_FILES_HPP
  "generated/config.hpp"
  "Etterna/Globals/GameLoop.h"
  "Etterna/Globals/global.h"
  "Etterna/Globals/OptionsBinding.h"
  "Etterna/Globals/ProductInfo.h" # TODO: Have this be auto-generated.
  "Etterna/Globals/SpecialFiles.h"
  "Etterna/Globals/StdString.h" # TODO: Remove the need for this file, transition to std::string.
  "Etterna/Globals/StepMania.h" # TODO: Refactor into separate main project.
  "Etterna/Globals/picosha2.h"
)

source_group("Global Files" FILES ${SMDATA_GLOBAL_FILES_SRC} ${SMDATA_GLOBAL_FILES_HPP})
