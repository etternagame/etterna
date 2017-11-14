#ifndef GRAPH_DISPLAY_H
#define GRAPH_DISPLAY_H

#include "ActorFrame.h"
#include "AutoActor.h"

class StageStats;
class PlayerStageStats;
class GraphLine;
class GraphBody;
/** @brief A graph of the player's life over the course of Gameplay, used on Evaluation. */
class GraphDisplay: public ActorFrame
{
public:
	GraphDisplay();
	~GraphDisplay() override;
	GraphDisplay *Copy() const override;

	void Load( const RString &sMetricsGroup );
	void Set( const StageStats &ss, const PlayerStageStats &s );

	// Lua
	void PushSelf( lua_State *L ) override;

private:
	void UpdateVerts();

	vector<float> m_Values;

	RectF m_quadVertices;

	vector<Actor*> m_vpSongBoundaries;
	AutoActor m_sprBarely;
	AutoActor m_sprBacking;
	AutoActor m_sprSongBoundary;

	GraphLine *m_pGraphLine;
	GraphBody *m_pGraphBody;
};

#endif

/*
 * (c) 2003 Glenn Maynard
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
