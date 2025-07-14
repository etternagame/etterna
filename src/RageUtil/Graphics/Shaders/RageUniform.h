#ifndef RAGE_UNIFORM_H
#define RAGE_UNIFORM_H

#include <cstdint>
#include <string>
#include <concepts>
#include <vector>

template<typename _T>
concept UniformType = requires(_T type) {
	requires std::is_same_v<_T, int> || std::is_same_v<_T, bool> ||
			   std::is_same_v<_T, float>;
};

// TODO: iirc this isn't really glued to any specific API,
// this might turn out to be just a std::vector with some metadata?
template<UniformType _UniformType>
class RageUniform
{
  public:
	RageUniform(std::string name,
					  uint32_t startRegister,
					  const std::vector<_UniformType>& data)
	  : m_Name(name)
	  , m_StartRegister(startRegister)
	  , m_Data(data)
	{
	}

	const std::string m_Name;
	const uint32_t m_StartRegister;
	const std::vector<_UniformType> m_Data;
};

#endif