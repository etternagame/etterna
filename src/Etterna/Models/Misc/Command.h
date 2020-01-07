/* Commands - Actor command parsing and reading helpers. */

#ifndef Commands_H
#define Commands_H

#include <vector>

class Command
{
  public:
	void Load(const RString& sCommand);

	RString GetOriginalCommandString()
	  const; // used when reporting an error in number of args
	RString GetName()
	  const; // the command name is the first argument in all-lowercase

	void Clear() { m_vsArgs.clear(); }

	struct Arg
	{
		RString s;
		Arg()
		  : s("")
		{
		}
	};
	Arg GetArg(unsigned index) const;

	std::vector<RString> m_vsArgs;

	Command()
	  : m_vsArgs()
	{
	}
};

class Commands
{
  public:
	std::vector<Command> v;

	RString GetOriginalCommandString()
	  const; // used when reporting an error in number of args
};

// Take a command list string and return pointers to each of the tokens in the
// string. sCommand list is a list of commands separated by ';'.
// TODO: This is expensive to do during the game.  Eventually,  move all calls
// to ParseCommands to happen during load, then execute from the parsed Command
// structures.
void
ParseCommands(const RString& sCmds, Commands& vCmdsOut, bool bLegacy);
Commands
ParseCommands(const RString& sCmds);

#endif
