#ifndef DISPLAY_COMMAND_BATCHER_H
#define DISPLAY_COMMAND_BATCHER_H

#include <queue>
#include <string>

// TODO: put commands for an actor into a blob and then make blob queue!
namespace Display {

class CommandBatcher {
 public:
	 // for later - push bytes, not raw locations?
  void InsertCommand(const std::string& cmdString);
  void CleanCommands();

  std::vector<std::string> m_CommandBuffer;
};

}  // namespace Display

#endif
