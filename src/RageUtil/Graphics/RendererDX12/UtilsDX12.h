#ifndef UTILS_DX12_H
#define UTILS_DX12_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <source_location>
#include <string>

std::string HrToString(HRESULT hr);
void ThrowIfFailed(HRESULT hr, const std::source_location location = std::source_location::current());

#endif