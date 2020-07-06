/* Commands - Actor command parsing and reading helpers. */

#ifndef Commands_H
#define Commands_H

class Command
{
  public:
	void Load(const std::string& sCommand);

	[[nodiscard]] std::string GetOriginalCommandString()
	  const; // used when reporting an error in number of args
	[[nodiscard]] std::string GetName()
	  const; // the command name is the first argument in all-lowercase

	void Clear() { m_vsArgs.clear(); }

	struct Arg
	{
		std::string s;
		Arg()
		  : s("")
		{
		}
	};

	[[nodiscard]] Arg GetArg(unsigned index) const;

	vector<std::string> m_vsArgs;

	Command()
	  : m_vsArgs()
	{
	}
};

class Commands
{
  public:
	vector<Command> v;

	[[nodiscard]] std::string GetOriginalCommandString()
	  const; // used when reporting an error in number of args
};

// Take a command list string and return pointers to each of the tokens in the
// string. sCommand list is a list of commands separated by ';'.
// TODO: This is expensive to do during the game.  Eventually,  move all calls
// to ParseCommands to happen during load, then execute from the parsed Command
// structures.
void
ParseCommands(const std::string& sCmds, Commands& vCmdsOut, bool bLegacy);
Commands
ParseCommands(const std::string& sCmds);

#endif
