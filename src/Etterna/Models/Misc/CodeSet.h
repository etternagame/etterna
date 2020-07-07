#ifndef CODE_SET_H
#define CODE_SET_H

#include "Etterna/Singletons/InputQueue.h"

struct Message;
class InputQueueCodeSet
{
  public:
	void Load(const std::string& sType);
	[[nodiscard]] std::string Input(const InputEventPlus& input) const;
	bool InputMessage(const InputEventPlus& input, Message& msg) const;

  private:
	std::vector<InputQueueCode> m_aCodes;
	std::vector<std::string> m_asCodeNames;
};

#endif
