#ifndef CODE_SET_H
#define CODE_SET_H

#include "Etterna/Singletons/InputQueue.h"

struct Message;
class InputQueueCodeSet
{
  public:
	void Load(const RString& sType);
	RString Input(const InputEventPlus& input) const;
	bool InputMessage(const InputEventPlus& input, Message& msg) const;

  private:
	vector<InputQueueCode> m_aCodes;
	vector<RString> m_asCodeNames;
};

#endif
