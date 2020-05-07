list(APPEND CORE_HPP Services/Locator.hpp Services/IFileManager.hpp)
list(APPEND CORE_CPP Services/Locator.cpp)

target_sources(Etterna PRIVATE ${CORE_HPP} ${CORE_CPP})