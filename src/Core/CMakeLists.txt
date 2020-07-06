list(APPEND CORE_HPP
        Misc/PlogLogger.hpp
        Services/Locator.hpp)
list(APPEND CORE_CPP
        Misc/PlogLogger.cpp
        Services/Locator.cpp)

target_sources(Etterna PRIVATE ${CORE_HPP} ${CORE_CPP})