list(APPEND CORE_HPP
        Misc/PlogLogger.hpp
        Services/Locator.hpp)
list(APPEND CORE_CPP
        Services/ILogger.cpp
        Services/Locator.cpp
        Misc/PlogLogger.cpp)

target_sources(Etterna PRIVATE ${CORE_HPP} ${CORE_CPP})