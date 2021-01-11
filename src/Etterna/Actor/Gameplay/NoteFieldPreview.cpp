#include "Etterna/Globals/global.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "NoteFieldPreview.h"

REGISTER_ACTOR_CLASS(NoteFieldPreview);

void
NoteFieldPreview::LoadFromNode(const XNode* pNode)
{
	Actor::LoadFromNode(pNode);
}
