list(APPEND SMDATA_GLOBAL_FILES_SRC
     "Main.cpp"
     "GameLoop.cpp"
     "global.cpp"
     "SpecialFiles.cpp"
     "StepMania.cpp" # TODO: Refactor into separate main project.
     "${PROJECT_BINARY_DIR}/generated/verstub.cpp"  #TODO: This is bad form.
     )

list(APPEND SMDATA_GLOBAL_FILES_HPP
     "GameLoop.h"
     "global.h"
     "OptionsBinding.h"
     "ProductInfo.h" # TODO: Have this be auto-generated.
     "SpecialFiles.h"
     "StdString.h" # TODO: Remove the need for this file, transition to std::string.
     "StepMania.h" # TODO: Refactor into separate main project.
     "picosha2.h")
source_group("Global Files" FILES ${SMDATA_GLOBAL_FILES_SRC} ${SMDATA_GLOBAL_FILES_HPP})

# Add sources to Etterna executable
target_sources(Etterna PUBLIC ${SMDATA_GLOBAL_FILES_SRC})