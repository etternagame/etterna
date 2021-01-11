#ifndef NOTE_FIELDPREVIEW_H
#define NOTE_FIELDPREVIEW_H

#include "NoteField.h"

class NoteFieldPreview : public NoteField
{
	void LoadFromNode(const XNode* pNode) override;
	[[nodiscard]] auto Copy() const -> NoteFieldPreview* override;
};

#endif
