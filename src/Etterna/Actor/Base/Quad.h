#ifndef QUAD_H
#define QUAD_H

#include "Sprite.h"

/** @brief A rectangular shaped Actor with color. */
class Quad : public Sprite
{
  public:
	/** @brief Initialize the quad. */
	Quad();
	/**
	 * @brief Load the quad from the specified node.
	 * @param pNode the node to load the quad from.
	 */
	void LoadFromNode(const XNode* pNode) override;
	/** @brief Copy the quad. */
	[[nodiscard]] auto Copy() const -> Quad* override;
};

#endif
