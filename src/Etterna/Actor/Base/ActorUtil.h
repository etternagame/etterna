#ifndef ActorUtil_H
#define ActorUtil_H

#include "Actor.h"

class XNode;

using CreateActorFn = Actor* (*)();

template<typename T>
Actor*
CreateActor()
{
	return new T;
}

/**
 * @brief Register the requested Actor with the specified external name.
 *
 * This should be called manually only if needed. */
#define REGISTER_ACTOR_CLASS_WITH_NAME(className, externalClassName)           \
	struct Register##className                                                 \
	{                                                                          \
		Register##className()                                                  \
		{                                                                      \
			ActorUtil::Register(#externalClassName, CreateActor<className>);   \
		}                                                                      \
	};                                                                         \
	className* className::Copy() const { return new className(*this); }        \
	static Register##className register##className

/**
 * @brief Register the requested Actor so that it's more accessible.
 *
 * Each Actor class should have a REGISTER_ACTOR_CLASS in its CPP file. */
#define REGISTER_ACTOR_CLASS(className)                                        \
	REGISTER_ACTOR_CLASS_WITH_NAME(className, className)

enum FileType
{
	FT_Bitmap,
	FT_Sprite,
	FT_Sound,
	FT_Movie,
	FT_Directory,
	FT_Xml,
	FT_Model,
	FT_Lua,
	FT_Ini,
	NUM_FileType,
	FileType_Invalid
};
const std::string&
FileTypeToString(FileType ft);

/** @brief Utility functions for creating and manipulating Actors. */
namespace ActorUtil {
void
InitFileTypeLists();
std::vector<std::string> const&
GetTypeExtensionList(FileType ft);
void
AddTypeExtensionsToList(FileType ft, std::vector<std::string>& add_to);

// Every screen should register its class at program initialization.
void
Register(const std::string& sClassName, CreateActorFn pfn);

apActorCommands
ParseActorCommands(const std::string& sCommands, const std::string& sName = "");
void
SetXY(Actor& actor, const std::string& sMetricsGroup);
inline void
PlayCommand(Actor& actor, const std::string& sCommandName)
{
	actor.PlayCommand(sCommandName);
}
inline void
OnCommand(Actor& actor)
{
	ASSERT_M(
	  actor.HasCommand("On"),
	  ssprintf("%s is missing an OnCommand.", actor.GetLineage().c_str()));
	actor.PlayCommand("On");
}
inline void
OffCommand(Actor& actor)
{
	// HACK:  It's very often that we command things to TweenOffScreen
	// that we aren't drawing.  We know that an Actor is not being
	// used if its name is blank.  So, do nothing on Actors with a blank name.
	// (Do "playcommand" anyway; BGAs often have no name.)
	if (actor.GetName().empty())
		return;
	ASSERT_M(actor.HasCommand("Off"),
			 ssprintf("Actor %s is missing an OffCommand.",
					  actor.GetLineage().c_str()));
	actor.PlayCommand("Off");
}

void
LoadCommand(Actor& actor,
			const std::string& sMetricsGroup,
			const std::string& sCommandName);
void
LoadCommandFromName(Actor& actor,
					const std::string& sMetricsGroup,
					const std::string& sCommandName,
					const std::string& sName);
void
LoadAllCommands(Actor& actor, const std::string& sMetricsGroup);
void
LoadAllCommandsFromName(Actor& actor,
						const std::string& sMetricsGroup,
						const std::string& sName);

inline void
LoadAllCommandsAndSetXY(Actor& actor, const std::string& sMetricsGroup)
{
	LoadAllCommands(actor, sMetricsGroup);
	SetXY(actor, sMetricsGroup);
}
inline void
LoadAllCommandsAndOnCommand(Actor& actor, const std::string& sMetricsGroup)
{
	LoadAllCommands(actor, sMetricsGroup);
	OnCommand(actor);
}
inline void
SetXYAndOnCommand(Actor& actor, const std::string& sMetricsGroup)
{
	SetXY(actor, sMetricsGroup);
	OnCommand(actor);
}
inline void
LoadAllCommandsAndSetXYAndOnCommand(Actor& actor,
									const std::string& sMetricsGroup)
{
	LoadAllCommands(actor, sMetricsGroup);
	SetXY(actor, sMetricsGroup);
	OnCommand(actor);
}

/* convenience */
inline void
SetXY(Actor* pActor, const std::string& sMetricsGroup)
{
	SetXY(*pActor, sMetricsGroup);
}
inline void
PlayCommand(Actor* pActor, const std::string& sCommandName)
{
	if (pActor)
		pActor->PlayCommand(sCommandName);
}
inline void
OnCommand(Actor* pActor)
{
	if (pActor)
		ActorUtil::OnCommand(*pActor);
}
inline void
OffCommand(Actor* pActor)
{
	if (pActor)
		ActorUtil::OffCommand(*pActor);
}

inline void
LoadAllCommands(Actor* pActor, const std::string& sMetricsGroup)
{
	if (pActor)
		LoadAllCommands(*pActor, sMetricsGroup);
}

inline void
LoadAllCommandsAndSetXY(Actor* pActor, const std::string& sMetricsGroup)
{
	if (pActor)
		LoadAllCommandsAndSetXY(*pActor, sMetricsGroup);
}
inline void
LoadAllCommandsAndOnCommand(Actor* pActor, const std::string& sMetricsGroup)
{
	if (pActor)
		LoadAllCommandsAndOnCommand(*pActor, sMetricsGroup);
}
inline void
SetXYAndOnCommand(Actor* pActor, const std::string& sMetricsGroup)
{
	if (pActor)
		SetXYAndOnCommand(*pActor, sMetricsGroup);
}
inline void
LoadAllCommandsAndSetXYAndOnCommand(Actor* pActor,
									const std::string& sMetricsGroup)
{
	if (pActor)
		LoadAllCommandsAndSetXYAndOnCommand(*pActor, sMetricsGroup);
}

// Return a Sprite, BitmapText, or Model depending on the file type
Actor*
LoadFromNode(const XNode* pNode, Actor* pParentActor = nullptr);
Actor*
MakeActor(const std::string& sPath, Actor* pParentActor = nullptr);
std::string
GetSourcePath(const XNode* pNode);
std::string
GetWhere(const XNode* pNode);
bool
GetAttrPath(const XNode* pNode,
			const std::string& sName,
			std::string& sOut,
			bool optional = false);
bool
LoadTableFromStackShowErrors(Lua* L);

bool
ResolvePath(std::string& sPath,
			const std::string& sName,
			bool optional = false);

void
SortByZPosition(std::vector<Actor*>& vActors);

FileType
GetFileType(const std::string& sPath);
};

#define SET_XY(actor) ActorUtil::SetXY(actor, m_sName)
#define ON_COMMAND(actor) ActorUtil::OnCommand(actor)
#define OFF_COMMAND(actor) ActorUtil::OffCommand(actor)
#define SET_XY_AND_ON_COMMAND(actor)                                           \
	ActorUtil::SetXYAndOnCommand(actor, m_sName)
#define COMMAND(actor, command_name) ActorUtil::PlayCommand(actor, command_name)
#define LOAD_ALL_COMMANDS(actor) ActorUtil::LoadAllCommands(actor, m_sName)
#define LOAD_ALL_COMMANDS_AND_SET_XY(actor)                                    \
	ActorUtil::LoadAllCommandsAndSetXY(actor, m_sName)
#define LOAD_ALL_COMMANDS_AND_ON_COMMAND(actor)                                \
	ActorUtil::LoadAllCommandsAndOnCommand(actor, m_sName)
#define LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(actor)                     \
	ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(actor, m_sName)

#endif
