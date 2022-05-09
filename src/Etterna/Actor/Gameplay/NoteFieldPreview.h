#ifndef NOTE_FIELDPREVIEW_H
#define NOTE_FIELDPREVIEW_H

#include "NoteField.h"

class Steps;

class NoteFieldPreview : public NoteField
{
  public:
	void LoadFromNode(const XNode* pNode) override;
	void Update(float fDeltaTime) override;
	void DrawPrimitives() override;
	// THIS DOESNT TRANSFORM NOTEDATA!!! DO IT YOURSELF!!!!	
	void LoadNoteData(NoteData* pNoteData);
	// This transforms NoteData to style and will follow PlayerOptions if wanted
	void LoadNoteData(Steps* pSteps, bool bTransform = false);
	void LoadDummyNoteData();
	void UpdateDrawDistance(int aftertargetspixels, int beforetargetspixels);
	void UpdateYReversePixels(float ReverseOffsetPixels);
	void ensure_note_displays_have_skin() override;
	NoteFieldPreview();
	~NoteFieldPreview();
	void SetPoseNoteField(bool b) { poseNoteField = b; }
	void SetConstantMini(float f)
	{
		constantMini = f;
		usingConstantMini = true;
	}
	void ResetConstantMini()
	{
		constantMini = 0.F;
		usingConstantMini = false;
	}
	
	[[nodiscard]] auto Copy() const -> NoteFieldPreview* override;
	void PushSelf(lua_State* L) override;

  private:
	NoteData* p_dummyNoteData;
	NoteData* p_NoteDataFromSteps;
	bool loadedNoteDataAtLeastOnce = false;
	bool poseNoteField = false;
	bool usingConstantMini = false;
	float constantMini = 0.F;
	float ReceptorArrowsYReverse = 0.F;
	float ReceptorArrowsYStandard = 0.F;
};

#endif
