/* LuaExpressionTransform -  */

#ifndef LuaExpressionTransform_H
#define LuaExpressionTransform_H

#include "Etterna/Actor/Base/Actor.h"
#include "LuaReference.h"
#include <map>

/**
 * @brief Handle transforming a list of items
 *
 * Cache item transforms based on fPositionOffsetFromCenter and iItemIndex for
 * speed. */
class LuaExpressionTransform
{
  public:
	LuaExpressionTransform();
	~LuaExpressionTransform();

	void SetFromReference(const LuaReference& ref);
	void SetNumSubdivisions(int iNumSubdivisions)
	{
		ASSERT(iNumSubdivisions > 0);
		m_iNumSubdivisions = iNumSubdivisions;
	}

	void TransformItemCached(Actor& a,
							 float fPositionOffsetFromCenter,
							 int iItemIndex,
							 int iNumItems);
	void TransformItemDirect(Actor& a,
							 float fPositionOffsetFromCenter,
							 int iItemIndex,
							 int iNumItems) const;
	const Actor::TweenState& GetTransformCached(float fPositionOffsetFromCenter,
												int iItemIndex,
												int iNumItems) const;
	void ClearCache() { m_mapPositionToTweenStateCache.clear(); }

  protected:
	LuaReference
	  m_exprTransformFunction; // params: self,offset,itemIndex,numItems
	int m_iNumSubdivisions;	// 1 == one evaluation per position
	struct PositionOffsetAndItemIndex
	{
		float fPositionOffsetFromCenter;
		int iItemIndex;

		bool operator<(const PositionOffsetAndItemIndex& other) const
		{
			if (fPositionOffsetFromCenter != other.fPositionOffsetFromCenter)
				return fPositionOffsetFromCenter <
					   other.fPositionOffsetFromCenter;
			return iItemIndex < other.iItemIndex;
		}
	};
	mutable std::map<PositionOffsetAndItemIndex, Actor::TweenState>
	  m_mapPositionToTweenStateCache;
};

#endif
