#ifndef CommandLineActions_H
#define CommandLineActions_H

class LoadingWindow;

/** @brief The collection of command line actions. */
namespace CommandLineActions {
/**
 * @brief Perform a utility function, then exit.
 * @param pLW the LoadingWindow that is presently not used? */
void
Handle(LoadingWindow* pLW);

/** @brief The housing for the command line arguments. */
class CommandLineArgs
{
  public:
	/** @brief the arguments in question. */
	vector<std::string> argv;
};
/**
 * @brief A list of command line arguemnts to process while the game is running.
 * These args could have come from this process or passed to this process
 * from another process. */
extern vector<CommandLineArgs> ToProcess;
}

#endif
