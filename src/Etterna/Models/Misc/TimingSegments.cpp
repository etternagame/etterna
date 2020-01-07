#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "TimingSegments.h"

const double TimingSegment::EPSILON = 1e-6;

static const char* TimingSegmentTypeNames[] = { "BPM",		 "Stop",  "Delay",
												"Time Sig",  "Warp",  "Label",
												"Tickcount", "Combo", "Speed",
												"Scroll",	"Fake" };
XToString(TimingSegmentType);

#define LTCOMPARE(x)                                                           \
	if (this->x < other.x)                                                     \
		return true;                                                           \
	if (this->x > other.x)                                                     \
		return false;

void
TimingSegment::Scale(int start, int length, int newLength)
{
	SetRow(ScalePosition(start, length, newLength, this->GetRow()));
}

void
TimingSegment::DebugPrint() const
{
	LOG->Trace("\tTimingSegment(%d [%f])", GetRow(), GetBeat());
}

void
BPMSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %f)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetBPM());
}

void
StopSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %f)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetPause());
}

void
DelaySegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %f)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetPause());
}

void
TimeSignatureSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %d/%d)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetNum(),
			   GetDen());
}

void
WarpSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %d [%f])",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetLengthRows(),
			   GetLengthBeats());
}

void
LabelSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %s)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetLabel().c_str());
}

void
TickcountSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %d)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetTicks());
}

void
ComboSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %d, %d)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetCombo(),
			   GetMissCombo());
}

void
SpeedSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %f, %f, %d)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetRatio(),
			   GetDelay(),
			   GetUnit());
}

void
ScrollSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %f)",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetRatio());
}

void
FakeSegment::DebugPrint() const
{
	LOG->Trace("\t%s(%d [%f], %d [%f])",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetLengthRows(),
			   GetLengthBeats());
}

RString
FakeSegment::ToString(int dec) const
{
	RString str = "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetLength());
}

void
FakeSegment::Scale(int start, int length, int newLength)
{
	float startBeat = GetBeat();
	float endBeat = startBeat + GetLength();
	float newStartBeat = ScalePosition(NoteRowToBeat(start),
									   NoteRowToBeat(length),
									   NoteRowToBeat(newLength),
									   startBeat);
	float newEndBeat = ScalePosition(NoteRowToBeat(start),
									 NoteRowToBeat(length),
									 NoteRowToBeat(newLength),
									 endBeat);
	SetLength(newEndBeat - newStartBeat);
	TimingSegment::Scale(start, length, newLength);
}

RString
WarpSegment::ToString(int dec) const
{
	RString str = "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetLength());
}

void
WarpSegment::Scale(int start, int length, int newLength)
{
	// XXX: this function is duplicated, there should be a better way
	float startBeat = GetBeat();
	float endBeat = startBeat + GetLength();
	float newStartBeat = ScalePosition(NoteRowToBeat(start),
									   NoteRowToBeat(length),
									   NoteRowToBeat(newLength),
									   startBeat);
	float newEndBeat = ScalePosition(NoteRowToBeat(start),
									 NoteRowToBeat(length),
									 NoteRowToBeat(newLength),
									 endBeat);
	SetLength(newEndBeat - newStartBeat);
	TimingSegment::Scale(start, length, newLength);
}

RString
TickcountSegment::ToString(int dec) const
{
	const RString str = "%.0" + IntToString(dec) + "f=%i";
	return ssprintf(str.c_str(), GetBeat(), GetTicks());
}
RString
ComboSegment::ToString(int dec) const
{
	RString str = "%.0" + IntToString(dec) + "f=%i";
	if (GetCombo() == GetMissCombo()) {
		return ssprintf(str.c_str(), GetBeat(), GetCombo());
	}
	str += "=%i";
	return ssprintf(str.c_str(), GetBeat(), GetCombo(), GetMissCombo());
}

std::vector<float>
ComboSegment::GetValues() const
{
	std::vector<float> ret;
	ret.push_back(static_cast<float>(GetCombo()));
	ret.push_back(static_cast<float>(GetMissCombo()));
	return ret;
}

RString
LabelSegment::ToString(int dec) const
{
	const RString str = "%.0" + IntToString(dec) + "f=%s";
	return ssprintf(str.c_str(), GetBeat(), GetLabel().c_str());
}

RString
BPMSegment::ToString(int dec) const
{
	const RString str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetBPM());
}

RString
TimeSignatureSegment::ToString(int dec) const
{
	const RString str = "%.0" + IntToString(dec) + "f=%i=%i";
	return ssprintf(str.c_str(), GetBeat(), GetNum(), GetDen());
}

std::vector<float>
TimeSignatureSegment::GetValues() const
{
	std::vector<float> ret;
	ret.push_back(static_cast<float>(GetNum()));
	ret.push_back(static_cast<float>(GetDen()));
	return ret;
}

RString
SpeedSegment::ToString(int dec) const
{
	const RString str = "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) +
						"f=%.0" + IntToString(dec) + "f=%u";
	return ssprintf(str.c_str(),
					GetBeat(),
					GetRatio(),
					GetDelay(),
					static_cast<unsigned int>(GetUnit()));
}

std::vector<float>
SpeedSegment::GetValues() const
{
	std::vector<float> ret;
	ret.push_back(GetRatio());
	ret.push_back(GetDelay());
	ret.push_back(static_cast<float>(GetUnit()));
	return ret;
}

void
SpeedSegment::Scale(int start, int oldLength, int newLength)
{
	if (GetUnit() == 0) {
		// XXX: this function is duplicated, there should be a better way
		float startBeat = GetBeat();
		float endBeat = startBeat + GetDelay();
		float newStartBeat = ScalePosition(NoteRowToBeat(start),
										   NoteRowToBeat(oldLength),
										   NoteRowToBeat(newLength),
										   startBeat);
		float newEndBeat = ScalePosition(NoteRowToBeat(start),
										 NoteRowToBeat(oldLength),
										 NoteRowToBeat(newLength),
										 endBeat);
		SetDelay(newEndBeat - newStartBeat);
	}
	TimingSegment::Scale(start, oldLength, newLength);
}

RString
ScrollSegment::ToString(int dec) const
{
	const RString str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetRatio());
}

RString
StopSegment::ToString(int dec) const
{
	const RString str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetPause());
}

RString
DelaySegment::ToString(int dec) const
{
	const RString str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetPause());
}
