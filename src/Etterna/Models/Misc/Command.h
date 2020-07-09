/* Commands - Actor command parsing and reading helpers. */

#ifndef Commands_H
#define Commands_H

#include <string>
#include <vector>

class Command
{
  public:
	void Load(const std::string& sCommand);

	[[nodiscard]] auto GetOriginalCommandString() const
	  -> std::string; // used when reporting an error in number of args
	[[nodiscard]] auto GetName() const
	  -> std::string; // the command name is the first argument in all-lowercase

	void Clear() { m_vsArgs.clear(); }

	struct Arg
	{
		std::string s;
		Arg()
		  : s("")
		{
		}
	};

	[[nodiscard]] auto GetArg(unsigned index) const -> Arg;

	std::vector<std::string> m_vsArgs;

	Command() = default;
};

class Commands
{
  public:
	std::vector<Command> v;

	[[nodiscard]] auto GetOriginalCommandString() const
	  -> std::string; // used when reporting an error in number of args
};

// Take a command list string and return pointers to each of the tokens in the
// string. sCommand list is a list of commands separated by ';'.
// TODO(Sam): This is expensive to do during the game.  Eventually,  move all
// calls to ParseCommands to happen during load, then execute from the parsed
// Command structures.
void
ParseCommands(const std::string& sCmds, Commands& vCmdsOut, bool bLegacy);
auto
ParseCommands(const std::string& sCmds) -> Commands;

#endif
