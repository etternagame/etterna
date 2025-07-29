#include "CommandBatcher.h"

void
Display::CommandBatcher::InsertCommand(const std::string& cmdString)
{
	m_CommandBuffer.push_back(cmdString);
}

void Display::CommandBatcher::CleanCommands()
{
	m_CommandBuffer.clear();
}
