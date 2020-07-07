#include "Etterna/Globals/global.h"
#include "ActorScroller.h"
#include "ActorUtil.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <algorithm>

/* Tricky: We need ActorFrames created in Lua to auto delete their children.
 * We don't want classes that derive from ActorFrame to auto delete their
 * children. The name "ActorFrame" is widely used in Lua, so we'll have
 * that string instead create an ActorFrameAutoDeleteChildren object. */
// REGISTER_ACTOR_CLASS( ActorScroller );
REGISTER_ACTOR_CLASS_WITH_NAME(ActorScrollerAutoDeleteChildren, ActorScroller);
ActorScroller*
ActorScroller::Copy() const
{
	return new ActorScroller(*this);
}

ActorScroller::ActorScroller()
{
	m_iNumItems = 0;
	m_fCurrentItem = 0;
	m_fDestinationItem = 0;
	m_fSecondsPerItem = 1;
	m_fSecondsPauseBetweenItems = 0;
	m_fNumItemsToDraw = 7;
	m_iFirstSubActorIndex = 0;
	m_bLoop = false;
	m_bWrap = false;
	m_bFastCatchup = false;
	m_bFunctionDependsOnPositionOffset = true;
	m_bFunctionDependsOnItemIndex = true;
	m_fPauseCountdownSeconds = 0;
	m_fQuantizePixels = 0;

	m_quadMask.SetBlendMode(BLEND_NO_EFFECT); // don't change color values
	m_quadMask.SetUseZBuffer(true);			  // we want to write to the Zbuffer
	m_fMaskWidth = 0;
	m_fMaskHeight = 0;
	DisableMask();
}

void
ActorScroller::Load2()
{
	m_iNumItems = m_SubActors.size();

	auto* L = LUA->Get();
	for (unsigned i = 0; i < m_SubActors.size(); ++i) {
		lua_pushnumber(L, i);
		this->m_SubActors[i]->m_pLuaInstance->Set(L, "ItemIndex");
	}
	LUA->Release(L);
}

void
ActorScroller::SetTransformFromReference(const LuaReference& ref)
{
	m_exprTransformFunction.SetFromReference(ref);

	// Probe to find which of the parameters are used.
#define GP(a, b) m_exprTransformFunction.GetTransformCached(a, b, 2)
	m_bFunctionDependsOnPositionOffset =
	  (GP(0, 0) != GP(1, 0)) && (GP(0, 1) != GP(1, 1));
	m_bFunctionDependsOnItemIndex =
	  (GP(0, 0) != GP(0, 1)) && (GP(1, 0) != GP(1, 1));
	m_exprTransformFunction.ClearCache();
}

void
ActorScroller::SetTransformFromExpression(const std::string& sTransformFunction)
{
	LuaReference ref;
	ref.SetFromExpression(sTransformFunction);
	SetTransformFromReference(ref);
}

void
ActorScroller::SetTransformFromWidth(float fItemWidth)
{
	SetTransformFromExpression(
	  ssprintf("function(self,offset,itemIndex,numItems) self:x(%f*offset) end",
			   fItemWidth));
}

void
ActorScroller::SetTransformFromHeight(float fItemHeight)
{
	SetTransformFromExpression(
	  ssprintf("function(self,offset,itemIndex,numItems) self:y(%f*offset) end",
			   fItemHeight));
}

void
ActorScroller::EnableMask(float fWidth, float fHeight)
{
	m_quadMask.SetVisible(fWidth != 0 && fHeight != 0);
	m_quadMask.SetWidth(fWidth);
	m_fMaskWidth = fWidth;
	m_quadMask.SetHeight(fHeight);
	m_fMaskHeight = fHeight;
}

void
ActorScroller::DisableMask()
{
	m_quadMask.SetVisible(false);
}

void
ActorScroller::ScrollThroughAllItems()
{
	m_fCurrentItem = (m_bLoop || m_bWrap) ? +m_fNumItemsToDraw / 2.0f
										  : -(m_fNumItemsToDraw / 2.0f) - 1;
	m_fDestinationItem =
	  static_cast<float>(m_iNumItems + m_fNumItemsToDraw / 2.0f + 1);
}

void
ActorScroller::ScrollWithPadding(float fItemPaddingStart, float fItemPaddingEnd)
{
	m_fCurrentItem = -fItemPaddingStart;
	m_fDestinationItem = m_iNumItems - 1 + fItemPaddingEnd;
}

float
ActorScroller::GetSecondsForCompleteScrollThrough() const
{
	const auto fTotalItems = m_fNumItemsToDraw + m_iNumItems;
	return fTotalItems * (m_fSecondsPerItem + m_fSecondsPauseBetweenItems);
}

float
ActorScroller::GetSecondsToDestination() const
{
	const auto fTotalItemsToMove = fabsf(m_fCurrentItem - m_fDestinationItem);
	return fTotalItemsToMove * m_fSecondsPerItem;
}

void
ActorScroller::LoadFromNode(const XNode* pNode)
{
	ActorFrame::LoadFromNode(pNode);

	Load2();

	float fNumItemsToDraw = 0;
	if (pNode->GetAttrValue("NumItemsToDraw", fNumItemsToDraw))
		SetNumItemsToDraw(fNumItemsToDraw);

	float fSecondsPerItem = 0;
	if (pNode->GetAttrValue("SecondsPerItem", fSecondsPerItem))
		ActorScroller::SetSecondsPerItem(fSecondsPerItem);

	auto* L = LUA->Get();
	pNode->PushAttrValue(L, "TransformFunction");
	{
		LuaReference ref;
		ref.SetFromStack(L);
		if (!ref.IsNil())
			SetTransformFromReference(ref);
	}
	LUA->Release(L);

	auto iSubdivisions = 1;
	if (pNode->GetAttrValue("Subdivisions", iSubdivisions))
		ActorScroller::SetNumSubdivisions(iSubdivisions);

	auto bUseMask = false;
	pNode->GetAttrValue("UseMask", bUseMask);

	if (bUseMask) {
		pNode->GetAttrValue("MaskWidth", m_fMaskWidth);
		pNode->GetAttrValue("MaskHeight", m_fMaskHeight);
		EnableMask(m_fMaskWidth, m_fMaskHeight);
	}

	pNode->GetAttrValue("QuantizePixels", m_fQuantizePixels);
	pNode->GetAttrValue("LoopScroller", m_bLoop);
	pNode->GetAttrValue("WrapScroller", m_bWrap);
}

void
ActorScroller::UpdateInternal(float fDeltaTime)
{
	ActorFrame::UpdateInternal(fDeltaTime);

	// If we have no children, the code below will busy loop.
	if (m_SubActors.empty())
		return;

	// handle pause
	if (fDeltaTime > m_fPauseCountdownSeconds) {
		fDeltaTime -= m_fPauseCountdownSeconds;
		m_fPauseCountdownSeconds = 0;
	} else {
		m_fPauseCountdownSeconds -= fDeltaTime;
		return;
	}

	if (m_fCurrentItem == m_fDestinationItem)
		return; // done scrolling

	const auto fOldItemAtTop = m_fCurrentItem;
	if (m_fSecondsPerItem > 0) {
		auto fApproachSpeed = fDeltaTime / m_fSecondsPerItem;
		if (m_bFastCatchup) {
			const auto fDistanceToMove =
			  fabsf(m_fCurrentItem - m_fDestinationItem);
			if (fDistanceToMove > 1)
				fApproachSpeed *= fDistanceToMove * fDistanceToMove;
		}

		fapproach(m_fCurrentItem, m_fDestinationItem, fApproachSpeed);
	}

	// if items changed, then pause
	if (static_cast<int>(fOldItemAtTop) != static_cast<int>(m_fCurrentItem))
		m_fPauseCountdownSeconds = m_fSecondsPauseBetweenItems;

	if (m_bWrap) {
		const auto Delta = m_fDestinationItem - m_fCurrentItem;
		m_fCurrentItem = fmodf(m_fCurrentItem, static_cast<float>(m_iNumItems));
		m_fDestinationItem = m_fCurrentItem + Delta;
	}

	if (m_bLoop) {
		m_fCurrentItem = fmodf(m_fCurrentItem, static_cast<float>(m_iNumItems));
	}
}

void
ActorScroller::DrawPrimitives()
{
	PositionItemsAndDrawPrimitives(true);
}

void
ActorScroller::PositionItems()
{
	PositionItemsAndDrawPrimitives(false);
}

/* Shift m_SubActors forward by iDist. This will place item
 * m_iFirstSubActorIndex in m_SubActors[0]. */
void
ActorScroller::ShiftSubActors(int iDist)
{
	if (iDist != INT_MAX)
		CircularShift(m_SubActors, iDist);
}

void
ActorScroller::PositionItemsAndDrawPrimitives(bool bDrawPrimitives)
{
	if (m_SubActors.empty())
		return;

	auto fNumItemsToDraw = m_fNumItemsToDraw;
	if (m_quadMask.GetVisible()) {
		// write to z buffer so that top and bottom are clipped
		// Draw an extra item; this is the one that will be masked.
		fNumItemsToDraw++;
		const auto fPositionFullyOffScreenTop = -(fNumItemsToDraw) / 2.f;
		const auto fPositionFullyOffScreenBottom = (fNumItemsToDraw) / 2.f;

		m_exprTransformFunction.TransformItemCached(
		  m_quadMask, fPositionFullyOffScreenTop, -1, m_iNumItems);
		if (bDrawPrimitives)
			m_quadMask.Draw();

		m_exprTransformFunction.TransformItemCached(
		  m_quadMask, fPositionFullyOffScreenBottom, m_iNumItems, m_iNumItems);
		if (bDrawPrimitives)
			m_quadMask.Draw();
	}

	const auto fFirstItemToDraw = m_fCurrentItem - fNumItemsToDraw / 2.f;
	const auto fLastItemToDraw = m_fCurrentItem + fNumItemsToDraw / 2.f;
	auto iFirstItemToDraw = static_cast<int>(ceilf(fFirstItemToDraw));
	auto iLastItemToDraw = static_cast<int>(ceilf(fLastItemToDraw));
	if (!m_bLoop && !m_bWrap) {
		iFirstItemToDraw = std::clamp(iFirstItemToDraw, 0, m_iNumItems);
		iLastItemToDraw = std::clamp(iLastItemToDraw, 0, m_iNumItems);
	}

	vector<Actor*> subs;

	{
		// Shift m_SubActors so iFirstItemToDraw is at the beginning.
		const auto iNewFirstIndex = iFirstItemToDraw;
		const auto iDist = iNewFirstIndex - m_iFirstSubActorIndex;
		m_iFirstSubActorIndex = iNewFirstIndex;
		ShiftSubActors(iDist);
	}

	const auto iNumToDraw = iLastItemToDraw - iFirstItemToDraw;
	for (auto i = 0; i < iNumToDraw; ++i) {
		auto iItem = i + iFirstItemToDraw;
		auto fPosition = iItem - m_fCurrentItem;
		auto iIndex = i; // index into m_SubActors
		if (m_bLoop || m_bWrap)
			wrap(iIndex, m_SubActors.size());
		else if (iIndex < 0 || iIndex >= static_cast<int>(m_SubActors.size()))
			continue;

		// Optimization: Zero out unused parameters so that they don't create
		// new, unnecessary  entries in the position cache. On scrollers with
		// lots of items, especially with Subdivisions > 1,
		// m_exprTransformFunction uses too much memory.
		if (!m_bFunctionDependsOnPositionOffset)
			fPosition = 0;
		if (!m_bFunctionDependsOnItemIndex)
			iItem = 0;

		m_exprTransformFunction.TransformItemCached(
		  *m_SubActors[iIndex], fPosition, iItem, m_iNumItems);
		if (bDrawPrimitives) {
			if (m_bDrawByZPosition)
				subs.push_back(m_SubActors[iIndex]);
			else
				m_SubActors[iIndex]->Draw();
		}
	}

	if (m_bDrawByZPosition) {
		ActorUtil::SortByZPosition(subs);
		for (auto& a : subs)
			a->Draw();
	}
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ActorScroller. */
class LunaActorScroller : public Luna<ActorScroller>
{
  public:
	static int PositionItems(T* p, lua_State* L)
	{
		p->PositionItems();
		COMMON_RETURN_SELF;
	}
	static int SetTransformFromFunction(T* p, lua_State* L)
	{
		LuaReference ref;
		LuaHelpers::FromStack(L, ref, 1);
		p->SetTransformFromReference(ref);
		COMMON_RETURN_SELF;
	}
	static int SetTransformFromHeight(T* p, lua_State* L)
	{
		p->SetTransformFromHeight(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetTransformFromWidth(T* p, lua_State* L)
	{
		p->SetTransformFromWidth(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetCurrentAndDestinationItem(T* p, lua_State* L)
	{
		p->SetCurrentAndDestinationItem(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetDestinationItem(T* p, lua_State* L)
	{
		p->SetDestinationItem(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetSecondsToDestination(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSecondsToDestination());
		return 1;
	}
	static int SetSecondsPerItem(T* p, lua_State* L)
	{
		p->SetSecondsPerItem(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetSecondsPauseBetweenItems(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSecondsPauseBetweenItems());
		return 1;
	}
	static int SetSecondsPauseBetweenItems(T* p, lua_State* L)
	{
		p->SetSecondsPauseBetweenItems(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetPauseCountdownSeconds(T* p, lua_State* L)
	{
		p->SetPauseCountdownSeconds(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetNumSubdivisions(T* p, lua_State* L)
	{
		p->SetNumSubdivisions(IArg(1));
		COMMON_RETURN_SELF;
	}
	static int ScrollThroughAllItems(T* p, lua_State* L)
	{
		p->ScrollThroughAllItems();
		COMMON_RETURN_SELF;
	}
	static int ScrollWithPadding(T* p, lua_State* L)
	{
		p->ScrollWithPadding(FArg(1), FArg(2));
		COMMON_RETURN_SELF;
	}
	static int SetFastCatchup(T* p, lua_State* L)
	{
		p->SetFastCatchup(BArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetLoop(T* p, lua_State* L)
	{
		p->SetLoop(BArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetWrap(T* p, lua_State* L)
	{
		p->SetWrap(BArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetMask(T* p, lua_State* L)
	{
		p->EnableMask(FArg(1), FArg(2));
		COMMON_RETURN_SELF;
	}

	static int SetNumItemsToDraw(T* p, lua_State* L)
	{
		p->SetNumItemsToDraw(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetFullScrollLengthSeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSecondsForCompleteScrollThrough());
		return 1;
	}
	static int GetCurrentItem(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetCurrentItem());
		return 1;
	}
	static int GetDestinationItem(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetDestinationItem());
		return 1;
	}
	static int GetNumItems(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumItems());
		return 1;
	}

	LunaActorScroller()
	{
		ADD_METHOD(PositionItems);
		ADD_METHOD(SetTransformFromFunction);
		ADD_METHOD(SetTransformFromHeight);
		ADD_METHOD(SetTransformFromWidth);
		ADD_METHOD(SetCurrentAndDestinationItem);
		ADD_METHOD(SetDestinationItem);
		ADD_METHOD(GetSecondsToDestination);
		ADD_METHOD(SetSecondsPerItem);
		ADD_METHOD(SetSecondsPauseBetweenItems);
		ADD_METHOD(GetSecondsPauseBetweenItems);
		ADD_METHOD(SetPauseCountdownSeconds);
		ADD_METHOD(SetNumSubdivisions);
		ADD_METHOD(ScrollThroughAllItems);
		ADD_METHOD(ScrollWithPadding);
		ADD_METHOD(SetFastCatchup);
		ADD_METHOD(SetLoop);
		ADD_METHOD(SetWrap);
		ADD_METHOD(SetMask);
		ADD_METHOD(SetNumItemsToDraw);
		ADD_METHOD(GetFullScrollLengthSeconds);
		ADD_METHOD(GetCurrentItem);
		ADD_METHOD(GetDestinationItem);
		ADD_METHOD(GetNumItems);
	}
};

LUA_REGISTER_DERIVED_CLASS(ActorScroller, ActorFrame)
// lua end
