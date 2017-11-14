#ifndef ACTORFRAME_H
#define ACTORFRAME_H

#include "Actor.h"

/** @brief A container for other Actors. */
class ActorFrame : public Actor
{
public:
	ActorFrame();
	ActorFrame( const ActorFrame &cpy );
	~ActorFrame() override;

	/** @brief Set up the initial state. */
	void InitState() override;
	void LoadFromNode( const XNode* pNode ) override;
	ActorFrame *Copy() const override;

	/**
	 * @brief Add a new child to the ActorFrame.
	 * @param pActor the new Actor to add. */
	virtual void AddChild( Actor *pActor );
	/**
	 * @brief Remove the specified child from the ActorFrame.
	 * @param pActor the Actor to remove. */
	virtual void RemoveChild( Actor *pActor );
	void TransferChildren( ActorFrame *pTo );
	Actor* GetChild( const RString &sName );
	vector<Actor*> GetChildren() { return m_SubActors; }
	int GetNumChildren() const { return m_SubActors.size(); }

	/** @brief Remove all of the children from the frame. */
	void RemoveAllChildren();
	/** 
	 * @brief Move a particular actor to the tail.
	 * @param pActor the actor to go to the tail.
	 */
	void MoveToTail( Actor* pActor );
	/**
	 * @brief Move a particular actor to the head.
	 * @param pActor the actor to go to the head.
	 */
	void MoveToHead( Actor* pActor );
	void SortByDrawOrder();
	void SetDrawByZPosition( bool b );

	void SetDrawFunction( const LuaReference &DrawFunction ) { m_DrawFunction = DrawFunction; }
	void SetUpdateFunction( const LuaReference &UpdateFunction ) { m_UpdateFunction = UpdateFunction; }

	LuaReference GetDrawFunction() const { return m_DrawFunction; }
	virtual bool AutoLoadChildren() const { return false; } // derived classes override to automatically LoadChildrenFromNode
	void DeleteChildrenWhenDone( bool bDelete=true ) { m_bDeleteChildren = bDelete; }
	void DeleteAllChildren();

	// Commands
	void PushSelf( lua_State *L ) override;
	void PushChildrenTable( lua_State *L );
	void PushChildTable( lua_State *L, const RString &sName );
	void PlayCommandOnChildren( const RString &sCommandName, const LuaReference *pParamTable = nullptr );
	void PlayCommandOnLeaves( const RString &sCommandName, const LuaReference *pParamTable = nullptr );

	void RunCommandsRecursively( const LuaReference& cmds, const LuaReference *pParamTable = nullptr ) override;
	virtual void RunCommandsOnChildren( const LuaReference& cmds, const LuaReference *pParamTable = nullptr ); /* but not on self */
	void RunCommandsOnChildren( const apActorCommands& cmds, const LuaReference *pParamTable = nullptr ) { this->RunCommandsOnChildren( *cmds, pParamTable ); }	// convenience
	void RunCommandsOnLeaves( const LuaReference& cmds, const LuaReference *pParamTable = nullptr ) override; /* but not on self */

	void UpdateInternal( float fDeltaTime ) override;
	void BeginDraw() override;
	void DrawPrimitives() override;
	void EndDraw() override;

	// propagated commands
	void SetZTestMode( ZTestMode mode ) override;
	void SetZWrite( bool b ) override;
	void FinishTweening() override;
	void HurryTweening( float factor ) override;

	void SetUpdateRate(float rate) override { if(rate > 0.0f) { m_fUpdateRate = rate; }}
	float GetUpdateRate() override { return m_fUpdateRate; }
	void SetFOV( float fFOV ) { m_fFOV = fFOV; }
	void SetVanishPoint( float fX, float fY) { m_fVanishX = fX; m_fVanishY = fY; }

	void SetCustomLighting( bool bCustomLighting ) { m_bOverrideLighting = bCustomLighting; }
	void SetAmbientLightColor( const RageColor &c ) { m_ambientColor = c; }
	void SetDiffuseLightColor( const RageColor &c ) { m_diffuseColor = c; }
	void SetSpecularLightColor( const RageColor &c ) { m_specularColor = c; }
	void SetLightDirection( const RageVector3 &vec ) { m_lightDirection = vec; }

	virtual void SetPropagateCommands( bool b );

	/** @brief Amount of time until all tweens (and all children's tweens) have stopped: */
	float GetTweenTimeLeft() const override;

	void HandleMessage( const Message &msg ) override;
	void RunCommands( const LuaReference& cmds, const LuaReference *pParamTable = nullptr ) override;
	void RunCommands( const apActorCommands& cmds, const LuaReference *pParamTable = nullptr ) { this->RunCommands( *cmds, pParamTable ); }	// convenience

protected:
	void LoadChildrenFromNode( const XNode* pNode );

	/** @brief The children Actors used by the ActorFrame. */
	vector<Actor*>	m_SubActors;
	bool m_bPropagateCommands;
	bool m_bDeleteChildren;
	bool m_bDrawByZPosition;
	LuaReference m_UpdateFunction;
	LuaReference m_DrawFunction;

	// state effects
	float m_fUpdateRate;
	float m_fFOV;	// -1 = no change
	float m_fVanishX;
	float m_fVanishY;
	/**
	 * @brief A flad to see if an override for the lighting is needed.
	 *
	 * If true, set lightning to m_bLightning. */
	bool m_bOverrideLighting;
	bool m_bLighting;

	// lighting variables
	RageColor m_ambientColor;
	RageColor m_diffuseColor;
	RageColor m_specularColor;
	RageVector3 m_lightDirection;
};
/** @brief an ActorFrame that handles deleting children Actors automatically. */
class ActorFrameAutoDeleteChildren : public ActorFrame
{
public:
	ActorFrameAutoDeleteChildren() { DeleteChildrenWhenDone(true); }
	bool AutoLoadChildren() const override { return true; }
	ActorFrameAutoDeleteChildren *Copy() const override;
};

#endif

/**
 * @file
 * @author Chris Danford (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
