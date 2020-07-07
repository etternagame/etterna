#include "CodeSet.h"
#include "InputEventPlus.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/ThemeManager.h"

#define CODE_NAMES THEME->GetMetric(sType, "CodeNames")
#define CODE(s) THEME->GetMetric(sType, ssprintf("Code%s", (s).c_str()))
void
InputQueueCodeSet::Load(const std::string& sType)
{
	//
	// Load codes
	//
	split(CODE_NAMES, ",", m_asCodeNames, true);

	for (auto& m_asCodeName : m_asCodeNames) {
		std::vector<std::string> asBits;
		split(m_asCodeName, "=", asBits, true);
		auto sCodeName = asBits[0];
		if (asBits.size() > 1)
			m_asCodeName = asBits[1];

		InputQueueCode code;
		if (!code.Load(CODE(sCodeName)))
			continue;

		m_aCodes.push_back(code);
	}
}

std::string
InputQueueCodeSet::Input(const InputEventPlus& input) const
{
	for (unsigned i = 0; i < m_aCodes.size(); ++i) {
		if (!m_aCodes[i].EnteredCode(input.GameI.controller))
			continue;

		return m_asCodeNames[i];
	}
	return "";
}

bool
InputQueueCodeSet::InputMessage(const InputEventPlus& input, Message& msg) const
{
	auto sCodeName = Input(input);
	if (sCodeName.empty())
		return false;

	msg.SetName("Code");
	msg.SetParam("PlayerNumber", input.pn);
	msg.SetParam("Name", sCodeName);
	return true;
}
