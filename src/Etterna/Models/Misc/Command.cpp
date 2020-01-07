#include "Etterna/Globals/global.h"
#include "Command.h"
#include "Foreach.h"
#include "RageUtil/Utils/RageUtil.h"

RString
Command::GetName() const
{
	if (m_vsArgs.empty())
		return RString();
	RString s = m_vsArgs[0];
	Trim(s);
	return s;
}

Command::Arg
Command::GetArg(unsigned index) const
{
	Arg a;
	if (index < m_vsArgs.size())
		a.s = m_vsArgs[index];
	return a;
}

void
Command::Load(const RString& sCommand)
{
	m_vsArgs.clear();
	split(sCommand, ",", m_vsArgs, false); // don't ignore empty
}

RString
Command::GetOriginalCommandString() const
{
	return join(",", m_vsArgs);
}

static void
SplitWithQuotes(const RString sSource,
				const char Delimitor,
				std::vector<RString>& asOut,
				const bool bIgnoreEmpty)
{
	/* Short-circuit if the source is empty; we want to return an empty vector
	 * if the string is empty, even if bIgnoreEmpty is true. */
	if (sSource.empty())
		return;

	size_t startpos = 0;
	do {
		size_t pos = startpos;
		while (pos < sSource.size()) {
			if (sSource[pos] == Delimitor)
				break;

			if (sSource[pos] == '"' || sSource[pos] == '\'') {
				/* We've found a quote.  Search for the close. */
				pos = sSource.find(sSource[pos], pos + 1);
				if (pos == std::string::npos)
					pos = sSource.size();
				else
					++pos;
			} else
				++pos;
		}

		if (pos - startpos > 0 || !bIgnoreEmpty) {
			/* Optimization: if we're copying the whole string, avoid substr;
			 * this allows this copy to be refcounted, which is much faster. */
			if (startpos == 0 && pos - startpos == sSource.size())
				asOut.push_back(sSource);
			else {
				const RString AddCString =
				  sSource.substr(startpos, pos - startpos);
				asOut.push_back(AddCString);
			}
		}

		startpos = pos + 1;
	} while (startpos <= sSource.size());
}

RString
Commands::GetOriginalCommandString() const
{
	RString s;
	FOREACH_CONST(Command, v, c)
	{
		if (s != "") {
			s += ";";
		}
		s += c->GetOriginalCommandString();
	}
	return s;
}

void
ParseCommands(const RString& sCommands, Commands& vCommandsOut, bool bLegacy)
{
	std::vector<RString> vsCommands;
	if (bLegacy)
		split(sCommands, ";", vsCommands, true);
	else
		SplitWithQuotes(sCommands, ';', vsCommands, true); // do ignore empty
	vCommandsOut.v.resize(vsCommands.size());

	for (unsigned i = 0; i < vsCommands.size(); i++) {
		Command& cmd = vCommandsOut.v[i];
		cmd.Load(vsCommands[i]);
	}
}

Commands
ParseCommands(const RString& sCommands)
{
	Commands vCommands;
	ParseCommands(sCommands, vCommands, false);
	return vCommands;
}
