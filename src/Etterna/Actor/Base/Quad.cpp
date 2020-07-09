#include "Etterna/Globals/global.h"
#include "ActorUtil.h"
#include "Quad.h"
#include "RageUtil/Graphics/RageTextureManager.h"

REGISTER_ACTOR_CLASS(Quad);

Quad::Quad()
{
	Load(TEXTUREMAN->GetDefaultTextureID());
}

void
Quad::LoadFromNode(const XNode* pNode)
{
	// HACK: Bypass Sprite's texture loading.  Sprite should really derive from
	// Quad.
	Actor::LoadFromNode(pNode);
}
