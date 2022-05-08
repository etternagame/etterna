#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "TimingSegments.h"
#include "Core/Services/Locator.hpp"

const double TimingSegment::EPSILON = 1e-6;

static const char* TimingSegmentTypeNames[] = { "BPM",		 "Stop",  "Delay",
												"Time Sig",	 "Warp",  "Label",
												"Tickcount", "Combo", "Speed",
												"Scroll",	 "Fake" };
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
	Locator::getLogger()->trace("\tTimingSegment({} [{}])", GetRow(), GetBeat());
}

void
BPMSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetBPM());
}

void
StopSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetPause());
}

void
DelaySegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetPause());
}

void
TimeSignatureSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {}/{})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetNum(),
			   GetDen());
}

void
WarpSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {} [{}])",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetLengthRows(),
			   GetLengthBeats());
}

void
LabelSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetLabel().c_str());
}

void
TickcountSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetTicks());
}

void
ComboSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {}, {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetCombo(),
			   GetMissCombo());
}

void
SpeedSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {}, {}, {})",
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
	Locator::getLogger()->trace("\t{}({} [{}], {})",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetRatio());
}

void
FakeSegment::DebugPrint() const
{
	Locator::getLogger()->trace("\t{}({} [{}], {} [{}])",
			   TimingSegmentTypeToString(GetType()).c_str(),
			   GetRow(),
			   GetBeat(),
			   GetLengthRows(),
			   GetLengthBeats());
}

std::string
FakeSegment::ToString(int dec) const
{
	const auto str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetLength());
}

void
FakeSegment::Scale(int start, int length, int newLength)
{
	const auto startBeat = GetBeat();
	const auto endBeat = startBeat + GetLength();
	const auto newStartBeat = ScalePosition(NoteRowToBeat(start),
											NoteRowToBeat(length),
											NoteRowToBeat(newLength),
											startBeat);
	const auto newEndBeat = ScalePosition(NoteRowToBeat(start),
										  NoteRowToBeat(length),
										  NoteRowToBeat(newLength),
										  endBeat);
	SetLength(newEndBeat - newStartBeat);
	TimingSegment::Scale(start, length, newLength);
}

std::string
WarpSegment::ToString(int dec) const
{
	const auto str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetLength());
}

void
WarpSegment::Scale(int start, int length, int newLength)
{
	// XXX: this function is duplicated, there should be a better way
	const auto startBeat = GetBeat();
	const auto endBeat = startBeat + GetLength();
	const auto newStartBeat = ScalePosition(NoteRowToBeat(start),
											NoteRowToBeat(length),
											NoteRowToBeat(newLength),
											startBeat);
	const auto newEndBeat = ScalePosition(NoteRowToBeat(start),
										  NoteRowToBeat(length),
										  NoteRowToBeat(newLength),
										  endBeat);
	SetLength(newEndBeat - newStartBeat);
	TimingSegment::Scale(start, length, newLength);
}

std::string
TickcountSegment::ToString(int dec) const
{
	const auto str = "%.0" + IntToString(dec) + "f=%i";
	return ssprintf(str.c_str(), GetBeat(), GetTicks());
}
std::string
ComboSegment::ToString(int dec) const
{
	auto str = "%.0" + IntToString(dec) + "f=%i";
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

std::string
LabelSegment::ToString(int dec) const
{
	const auto str = "%.0" + IntToString(dec) + "f=%s";
	return ssprintf(str.c_str(), GetBeat(), GetLabel().c_str());
}

std::string
BPMSegment::ToString(int dec) const
{
	const auto str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetBPM());
}

std::string
TimeSignatureSegment::ToString(int dec) const
{
	const auto str = "%.0" + IntToString(dec) + "f=%i=%i";
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

std::string
SpeedSegment::ToString(int dec) const
{
	const auto str = "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) +
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
		const auto startBeat = GetBeat();
		const auto endBeat = startBeat + GetDelay();
		const auto newStartBeat = ScalePosition(NoteRowToBeat(start),
												NoteRowToBeat(oldLength),
												NoteRowToBeat(newLength),
												startBeat);
		const auto newEndBeat = ScalePosition(NoteRowToBeat(start),
											  NoteRowToBeat(oldLength),
											  NoteRowToBeat(newLength),
											  endBeat);
		SetDelay(newEndBeat - newStartBeat);
	}
	TimingSegment::Scale(start, oldLength, newLength);
}

std::string
ScrollSegment::ToString(int dec) const
{
	const auto str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetRatio());
}

std::string
StopSegment::ToString(int dec) const
{
	const auto str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetPause());
}

std::string
DelaySegment::ToString(int dec) const
{
	const auto str =
	  "%.0" + IntToString(dec) + "f=%.0" + IntToString(dec) + "f";
	return ssprintf(str.c_str(), GetBeat(), GetPause());
}
