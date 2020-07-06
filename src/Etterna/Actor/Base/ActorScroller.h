#ifndef ActorScroller_H
#define ActorScroller_H

#include "ActorFrame.h"
#include "Quad.h"
class XNode;
#include "Etterna/Models/Lua/LuaExpressionTransform.h"
/** @brief ActorFrame that moves its children. */
class ActorScroller : public ActorFrame
{
  public:
	ActorScroller();

	void SetTransformFromReference(const LuaReference& ref);
	void SetTransformFromExpression(const std::string& sTransformFunction);
	void SetTransformFromWidth(float fItemWidth);
	void SetTransformFromHeight(float fItemHeight);

	void Load2();

	void EnableMask(float fWidth, float fHeight);
	void DisableMask();

	void UpdateInternal(float fDelta) override;
	void DrawPrimitives()
	  override; // handles drawing and doesn't call ActorFrame::DrawPrimitives

	void PositionItems();

	void LoadFromNode(const XNode* pNode) override;
	ActorScroller* Copy() const override;

	void SetLoop(bool bLoop) { m_bLoop = bLoop; }
	void SetWrap(bool bWrap) { m_bWrap = bWrap; }
	void SetNumItemsToDraw(float fNumItemsToDraw)
	{
		m_fNumItemsToDraw = fNumItemsToDraw;
	}
	void SetDestinationItem(float fItemIndex)
	{
		m_fDestinationItem = fItemIndex;
	}
	void SetCurrentAndDestinationItem(float fItemIndex)
	{
		m_fCurrentItem = m_fDestinationItem = fItemIndex;
	}
	float GetCurrentItem() const { return m_fCurrentItem; }
	float GetDestinationItem() const { return m_fDestinationItem; }
	void ScrollThroughAllItems();
	void ScrollWithPadding(float fItemPaddingStart, float fItemPaddingEnd);
	void SetPauseCountdownSeconds(float fSecs)
	{
		m_fPauseCountdownSeconds = fSecs;
	}
	void SetFastCatchup(bool bOn) { m_bFastCatchup = bOn; }
	void SetSecondsPerItem(float fSeconds) { m_fSecondsPerItem = fSeconds; }
	void SetSecondsPauseBetweenItems(float fSeconds)
	{
		m_fSecondsPauseBetweenItems = fSeconds;
	}
	float GetSecondsPauseBetweenItems() const
	{
		return m_fSecondsPauseBetweenItems;
	}
	void SetNumSubdivisions(int iNumSubdivisions)
	{
		m_exprTransformFunction.SetNumSubdivisions(iNumSubdivisions);
	}
	float GetSecondsForCompleteScrollThrough() const;
	float GetSecondsToDestination() const;
	int GetNumItems() const { return m_iNumItems; }

	// Commands
	void PushSelf(lua_State* L) override;

  protected:
	void PositionItemsAndDrawPrimitives(bool bDrawPrimitives);
	virtual void ShiftSubActors(int iDist);

	int m_iNumItems;
	/**
	 * @brief the current item we are focused on.
	 *
	 * An item at the center of the list, usually between 0 and
	 * m_SubActors.size(), will approach its destination.
	 *
	 * The above comment was paraphrased from what was here previously. It could
	 * use some clearing up. -Wolfman2000 */
	float m_fCurrentItem;
	float m_fDestinationItem;
	/**
	 * @brief How many seconds are there per item?
	 *
	 * If this is less than zero, then we are not scrolling. */
	float m_fSecondsPerItem;
	float m_fSecondsPauseBetweenItems;
	float m_fNumItemsToDraw;
	int m_iFirstSubActorIndex;
	bool m_bLoop;
	bool m_bWrap;
	bool m_bFastCatchup;
	bool m_bFunctionDependsOnPositionOffset;
	bool m_bFunctionDependsOnItemIndex;
	float m_fPauseCountdownSeconds;
	float m_fQuantizePixels;

	Quad m_quadMask;
	float m_fMaskWidth, m_fMaskHeight;

	LuaExpressionTransform
	  m_exprTransformFunction; // params: self,offset,itemIndex,numItems
};

class ActorScrollerAutoDeleteChildren : public ActorScroller
{
  public:
	ActorScrollerAutoDeleteChildren() { DeleteChildrenWhenDone(true); }
	bool AutoLoadChildren() const override { return true; }
	ActorScrollerAutoDeleteChildren* Copy() const override;
};

#endif
