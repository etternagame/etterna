#ifndef TIMING_SEGMENTS_H
#define TIMING_SEGMENTS_H

#include <utility>

#include "NoteTypes.h" // Converting rows to beats and vice~versa.

enum TimingSegmentType
{
	SEGMENT_BPM,
	SEGMENT_STOP,
	SEGMENT_DELAY,
	SEGMENT_TIME_SIG,
	SEGMENT_WARP,
	SEGMENT_LABEL,
	SEGMENT_TICKCOUNT,
	SEGMENT_COMBO,
	SEGMENT_SPEED,
	SEGMENT_SCROLL,
	SEGMENT_FAKE,
	NUM_TimingSegmentType,
	TimingSegmentType_Invalid,
};

// XXX: dumb names
enum SegmentEffectType
{
	SegmentEffectType_Row,		  // takes effect on a single row
	SegmentEffectType_Range,	  // takes effect for a definite amount of rows
	SegmentEffectType_Indefinite, // takes effect until the next segment of its
								  // type
	NUM_SegmentEffectType,
	SegmentEffectType_Invalid,
};

#define FOREACH_TimingSegmentType(tst) FOREACH_ENUM(TimingSegmentType, tst)

auto
TimingSegmentTypeToString(TimingSegmentType tst) -> const std::string&;

const int ROW_INVALID = -1;

#define COMPARE(x)                                                             \
	if (this->x != other.x)                                                    \
	return false
#define COMPARE_FLOAT(x)                                                       \
	if (fabsf(this->x - other.x) > EPSILON)                                    \
	return false

/**
 * @brief The base timing segment for make glorious benefit wolfman
 * XXX: this should be an abstract class.
 */
struct TimingSegment
{
	[[nodiscard]] virtual auto GetType() const -> TimingSegmentType
	{
		return TimingSegmentType_Invalid;
	}

	[[nodiscard]] virtual auto GetEffectType() const -> SegmentEffectType
	{
		return SegmentEffectType_Invalid;
	}

	[[nodiscard]] virtual auto Copy() const -> TimingSegment* = 0;

	[[nodiscard]] virtual auto IsNotable() const -> bool = 0;
	virtual void DebugPrint() const;

	// don't allow base TimingSegments to be instantiated directly
	TimingSegment(int iRow = ROW_INVALID)
	  : m_iStartRow(iRow)
	{
	}
	TimingSegment(float fBeat)
	  : m_iStartRow(ToNoteRow(fBeat))
	{
	}

	TimingSegment(const TimingSegment& other)
	  : m_iStartRow(other.GetRow())
	{
	}

	// for our purposes, two floats within this level of error are equal
	static const double EPSILON;

	virtual ~TimingSegment() = default;

	/**
	 * @brief Scales itself.
	 * @param start Starting row
	 * @param length Length in rows
	 * @param newLength The new length in rows
	 */
	virtual void Scale(int start, int length, int newLength);

	[[nodiscard]] auto GetRow() const -> int { return m_iStartRow; }
	void SetRow(int iRow) { m_iStartRow = iRow; }

	[[nodiscard]] auto GetBeat() const -> float
	{
		return NoteRowToBeat(m_iStartRow);
	}
	void SetBeat(float fBeat) { SetRow(BeatToNoteRow(fBeat)); }

	[[nodiscard]] virtual auto ToString(int /* dec */) const -> std::string
	{
		return FloatToString(GetBeat());
	}

	[[nodiscard]] virtual auto GetValues() const -> std::vector<float>
	{
		return std::vector<float>(0);
	}

	auto operator<(const TimingSegment& other) const -> bool
	{
		return GetRow() < other.GetRow();
	}

	// overloads should not call this base version; derived classes
	// should only compare contents, and this compares position.
	virtual auto operator==(const TimingSegment& other) const -> bool
	{
		return GetRow() == other.GetRow();
	}

	virtual auto operator!=(const TimingSegment& other) const -> bool
	{
		return !this->operator==(other);
	}

  private:
	/** @brief The row in which this segment activates. */
	int m_iStartRow;
};

/**
 * @brief Identifies when a whole region of arrows is to be ignored.
 *
 * FakeSegments are similar to the Fake Tap Notes in that the contents
 * inside are neither for nor against the player. They can be useful for
 * mission modes, in conjunction with WarpSegments, or perhaps other
 * uses not thought up at the time of this comment. Unlike the Warp
 * Segments, these are not magically jumped over: instead, these are
 * drawn normally.
 *
 * These were inspired by the Pump It Up series. */
struct FakeSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_FAKE;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Range;
	}

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new FakeSegment(*this);
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return m_iLengthRows > 0;
	}
	void DebugPrint() const override;

	FakeSegment(int iStartRow, int iLengthRows)
	  : TimingSegment(iStartRow)
	  , m_iLengthRows(iLengthRows)
	{
	}

	FakeSegment(int iStartRow, float fBeats)
	  : TimingSegment(iStartRow)
	  , m_iLengthRows(ToNoteRow(fBeats))
	{
	}

	FakeSegment(const FakeSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_iLengthRows(other.GetLengthRows())
	{
	}

	FakeSegment() = default;

	[[nodiscard]] auto GetLengthRows() const -> int { return m_iLengthRows; }
	[[nodiscard]] auto GetLengthBeats() const -> float
	{
		return ToBeat(m_iLengthRows);
	}
	[[nodiscard]] auto GetLength() const -> float
	{
		return GetLengthBeats();
	} // compatibility

	void SetLength(int iRows) { m_iLengthRows = ToNoteRow(iRows); }
	void SetLength(float fBeats) { m_iLengthRows = ToNoteRow(fBeats); }

	void Scale(int start, int length, int newLength) override;

	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetLength());
	}

	auto operator==(const FakeSegment& other) const -> bool
	{
		return m_iLengthRows == other.m_iLengthRows;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const FakeSegment&>(other));
	}

  private:
	/** @brief The number of rows the FakeSegment is alive for. */
	int m_iLengthRows{ -1 };
};

/**
 * @brief Identifies when a song needs to warp to a new beat.
 *
 * A warp segment is used to replicate the effects of Negative BPMs without
 * abusing negative BPMs. Negative BPMs should be converted to warp segments.
 * WarpAt=WarpToRelative is the format, where both are in beats.
 * (Technically they're both rows though.) */
struct WarpSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_WARP;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Range;
	}

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new WarpSegment(*this);
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return m_iLengthRows > 0;
	}
	void DebugPrint() const override;

	WarpSegment(const WarpSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_iLengthRows(other.GetLengthRows())
	{
	}

	WarpSegment(int iStartRow, int iLengthRows)
	  : TimingSegment(iStartRow)
	  , m_iLengthRows(iLengthRows)
	{
	}

	WarpSegment(int iStartRow, float fBeats)
	  : TimingSegment(iStartRow)
	  , m_iLengthRows(ToNoteRow(fBeats))
	{
	}

	WarpSegment() = default;

	[[nodiscard]] auto GetLengthRows() const -> int { return m_iLengthRows; }
	[[nodiscard]] auto GetLengthBeats() const -> float
	{
		return ToBeat(m_iLengthRows);
	}
	[[nodiscard]] auto GetLength() const -> float
	{
		return GetLengthBeats();
	} // compatibility

	void SetLength(int iRows) { m_iLengthRows = ToNoteRow(iRows); }
	void SetLength(float fBeats) { m_iLengthRows = ToNoteRow(fBeats); }

	void Scale(int start, int length, int newLength) override;
	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetLength());
	}

	auto operator==(const WarpSegment& other) const -> bool
	{
		return m_iLengthRows == other.m_iLengthRows;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const WarpSegment&>(other));
	}

  private:
	/** @brief The number of rows the WarpSegment will warp past. */
	int m_iLengthRows{ 0 };
};

/**
 * @brief Identifies when a chart is to have a different tickcount value
 * for hold notes.
 *
 * A tickcount segment is used to better replicate the checkpoint hold
 * system used by various based video games. The number is used to
 * represent how many ticks can be counted in one beat.
 */

struct TickcountSegment : public TimingSegment
{
	/** @brief The default amount of ticks per beat. */
	static const unsigned DEFAULT_TICK_COUNT = 4;

	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_TICKCOUNT;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new TickcountSegment(*this);
	}

	TickcountSegment(int iStartRow = ROW_INVALID,
					 int iTicks = DEFAULT_TICK_COUNT)
	  : TimingSegment(iStartRow)
	  , m_iTicksPerBeat(iTicks)
	{
	}

	TickcountSegment(const TickcountSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_iTicksPerBeat(other.GetTicks())
	{
	}

	[[nodiscard]] auto GetTicks() const -> int { return m_iTicksPerBeat; }
	void SetTicks(int iTicks) { m_iTicksPerBeat = iTicks; }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetTicks() * 1.F);
	}

	auto operator==(const TickcountSegment& other) const -> bool
	{
		return m_iTicksPerBeat == other.m_iTicksPerBeat;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const TickcountSegment&>(other));
	}

  private:
	/** @brief The amount of hold checkpoints counted per beat */
	int m_iTicksPerBeat;
};

/**
 * @brief Identifies when a chart is to have a different combo multiplier value.
 *
 * Admitedly, this would primarily be used for mission mode style charts.
 * However, it can have its place during normal gameplay.
 */
struct ComboSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_COMBO;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new ComboSegment(*this);
	}

	ComboSegment(int iStartRow = ROW_INVALID,
				 int iCombo = 1,
				 int iMissCombo = 1)
	  : TimingSegment(iStartRow)
	  , m_iCombo(iCombo)
	  , m_iMissCombo(iMissCombo)
	{
	}

	ComboSegment(const ComboSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_iCombo(other.GetCombo())
	  , m_iMissCombo(other.GetMissCombo())
	{
	}

	[[nodiscard]] auto GetCombo() const -> int { return m_iCombo; }
	[[nodiscard]] auto GetMissCombo() const -> int { return m_iMissCombo; }

	void SetCombo(int iCombo) { m_iCombo = iCombo; }
	void SetMissCombo(int iCombo) { m_iMissCombo = iCombo; }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;
	[[nodiscard]] auto GetValues() const -> std::vector<float> override;

	auto operator==(const ComboSegment& other) const -> bool
	{
		COMPARE(m_iCombo);
		COMPARE(m_iMissCombo);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const ComboSegment&>(other));
	}

  private:
	/** @brief The amount the combo increases at this point. */
	int m_iCombo;

	/** @brief The amount of miss combos given at this point. */
	int m_iMissCombo;
};

/**
 * @brief Identifies when a chart is entering a different section.
 *
 * This is meant for helping to identify different sections of a chart
 * versus relying on measures and beats alone.
 */
struct LabelSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_LABEL;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new LabelSegment(*this);
	}

	LabelSegment(int iStartRow = ROW_INVALID,
				 std::string sLabel = std::string())
	  : TimingSegment(iStartRow)
	  , m_sLabel(std::move(sLabel))
	{
	}

	LabelSegment(const LabelSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_sLabel(other.GetLabel())
	{
	}

	[[nodiscard]] auto GetLabel() const -> const std::string&
	{
		return m_sLabel;
	}
	void SetLabel(const std::string& sLabel) { m_sLabel.assign(sLabel); }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;
	// Use the default definition for GetValues because the value for a
	// LabelSegment is not a float or set of floats. TimingSegmentSetToLuaTable
	// in TimingData.cpp has a special case for labels to handle this.

	auto operator==(const LabelSegment& other) const -> bool
	{
		return m_sLabel == other.m_sLabel;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const LabelSegment&>(other));
	}

  private:
	/** @brief The label/section name for this point. */
	std::string m_sLabel;
};

/**
 * @brief Identifies when a song changes its BPM.
 */
struct BPMSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_BPM;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new BPMSegment(*this);
	}

	// note that this takes a BPM, not a BPS (compatibility)
	BPMSegment(int iStartRow = ROW_INVALID, float fBPM = 0.0F)
	  : TimingSegment(iStartRow)
	{
		SetBPM(fBPM);
	}

	BPMSegment(const BPMSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_fBPS(other.GetBPS())
	{
	}

	[[nodiscard]] auto GetBPS() const -> float { return m_fBPS; }
	[[nodiscard]] auto GetBPM() const -> float { return m_fBPS * 60.0F; }

	void SetBPS(float fBPS) { m_fBPS = fBPS; }
	void SetBPM(float fBPM) { m_fBPS = fBPM / 60.0F; }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetBPM());
	}

	auto operator==(const BPMSegment& other) const -> bool
	{
		COMPARE_FLOAT(m_fBPS);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const BPMSegment&>(other));
	}

  private:
	/** @brief The number of beats per second within this BPMSegment. */
	float m_fBPS{};
};

/**
 * @brief Identifies when a song changes its time signature.
 *
 * This only supports simple time signatures. The upper number
 * (called the numerator here, though this isn't properly a
 * fraction) is the number of beats per measure. The lower number
 * (denominator here) is the note value representing one beat. */
struct TimeSignatureSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_TIME_SIG;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new TimeSignatureSegment(*this);
	}

	TimeSignatureSegment(int iStartRow = ROW_INVALID,
						 int iNum = 4,
						 int iDenom = 4)
	  : TimingSegment(iStartRow)
	  , m_iNumerator(iNum)
	  , m_iDenominator(iDenom)
	{
	}

	TimeSignatureSegment(const TimeSignatureSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_iNumerator(other.GetNum())
	  , m_iDenominator(other.GetDen())
	{
	}

	[[nodiscard]] auto GetNum() const -> int { return m_iNumerator; }
	void SetNum(int num) { m_iNumerator = num; }

	[[nodiscard]] auto GetDen() const -> int { return m_iDenominator; }
	void SetDen(int den) { m_iDenominator = den; }

	void Set(int num, int den)
	{
		m_iNumerator = num;
		m_iDenominator = den;
	}

	[[nodiscard]] auto ToString(int dec) const -> std::string override;
	[[nodiscard]] auto GetValues() const -> std::vector<float> override;

	/**
	 * @brief Retrieve the number of note rows per measure within the
	 * TimeSignatureSegment.
	 *
	 * With BeatToNoteRow(1) rows per beat, then we should have
	 * BeatToNoteRow(1)*m_iNumerator beats per measure. But if we assume that
	 * every BeatToNoteRow(1) rows is a quarter note, and we want the beats to
	 * be 1/m_iDenominator notes, then we should have BeatToNoteRow(1)*4 is rows
	 * per whole note and thus BeatToNoteRow(1)*4/m_iDenominator is rows per
	 * beat. Multiplying by m_iNumerator gives rows per measure.
	 * @returns the number of note rows per measure.
	 */
	[[nodiscard]] auto GetNoteRowsPerMeasure() const -> int
	{
		return BeatToNoteRow(1) * 4 * m_iNumerator / m_iDenominator;
	}

	auto operator==(const TimeSignatureSegment& other) const -> bool
	{
		COMPARE(m_iNumerator);
		COMPARE(m_iDenominator);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const TimeSignatureSegment&>(other));
	}

  private:
	int m_iNumerator, m_iDenominator;
};

/**
 * @brief Identifies when the arrow scroll changes.
 *
 * SpeedSegments take a Player's scrolling BPM (Step's BPM * speed mod),
 * and then multiplies it with the percentage value. No matter the player's
 * speed mod, the ratio will be the same. Unlike forced attacks, these
 * cannot be turned off at a set time: reset it by setting the percentage
 * back to 1.
 *
 * These were inspired by the Pump It Up series. */
struct SpeedSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_SPEED;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new SpeedSegment(*this);
	}

	/** @brief The type of unit used for segment scaling. */
	enum BaseUnit
	{
		UNIT_BEATS,
		UNIT_SECONDS
	};

	SpeedSegment(int iStartRow = ROW_INVALID,
				 float fRatio = 1.0F,
				 float fDelay = 0.0F,
				 BaseUnit unit = UNIT_BEATS)
	  : TimingSegment(iStartRow)
	  , m_fRatio(fRatio)
	  , m_fDelay(fDelay)
	  , m_Unit(unit)
	{
	}

	SpeedSegment(const SpeedSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_fRatio(other.GetRatio())
	  , m_fDelay(other.GetDelay())
	  , m_Unit(other.GetUnit())
	{
	}

	[[nodiscard]] auto GetRatio() const -> float { return m_fRatio; }
	void SetRatio(float fRatio) { m_fRatio = fRatio; }

	[[nodiscard]] auto GetDelay() const -> float { return m_fDelay; }
	void SetDelay(float fDelay) { m_fDelay = fDelay; }

	[[nodiscard]] auto GetUnit() const -> BaseUnit { return m_Unit; }
	void SetUnit(BaseUnit unit) { m_Unit = unit; }

	void Scale(int start, int length, int newLength) override;

	[[nodiscard]] auto ToString(int dec) const -> std::string override;
	[[nodiscard]] auto GetValues() const -> std::vector<float> override;

	auto operator==(const SpeedSegment& other) const -> bool
	{
		COMPARE_FLOAT(m_fRatio);
		COMPARE_FLOAT(m_fDelay);
		COMPARE(m_Unit);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const SpeedSegment&>(other));
	}

  private:
	/** @brief The percentage by which the Player's BPM is multiplied. */
	float m_fRatio;

	/**
	 * @brief The number of beats or seconds to wait before applying.
	 * A value of 0 means this is immediate. */
	float m_fDelay;

	/** @brief The mode that this segment uses for the math.  */
	BaseUnit m_Unit;
};

/**
 * @brief Identifies when the chart scroll changes.
 *
 * ScrollSegments adjusts the scrolling speed of the note field.
 * Unlike forced attacks, these cannot be turned off at a set time:
 * reset it by setting the precentage back to 1.
 *
 * These were inspired by the Pump It Up series. */
struct ScrollSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_SCROLL;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Indefinite;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return true;
	} // indefinite segments are always true
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new ScrollSegment(*this);
	}

	ScrollSegment(int iStartRow = ROW_INVALID, float fRatio = 1.0F)
	  : TimingSegment(iStartRow)
	  , m_fRatio(fRatio)
	{
	}

	ScrollSegment(const ScrollSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_fRatio(other.GetRatio())
	{
	}

	[[nodiscard]] auto GetRatio() const -> float { return m_fRatio; }
	void SetRatio(float fRatio) { m_fRatio = fRatio; }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetRatio());
	}

	auto operator==(const ScrollSegment& other) const -> bool
	{
		COMPARE_FLOAT(m_fRatio);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const ScrollSegment&>(other));
	}

  private:
	/** @brief The percentage by which the chart's scroll rate is multiplied. */
	float m_fRatio;
};

/**
 * @brief Identifies when a song has a stop, DDR/ITG style.
 */
struct StopSegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_STOP;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Row;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return m_fSeconds > 0;
	}
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new StopSegment(*this);
	}

	StopSegment(int iStartRow = ROW_INVALID, float fSeconds = 0.0F)
	  : TimingSegment(iStartRow)
	  , m_fSeconds(fSeconds)
	{
	}

	StopSegment(const StopSegment& other)
	  : TimingSegment(other.GetRow())
	  , m_fSeconds(other.GetPause())
	{
	}

	[[nodiscard]] auto GetPause() const -> float { return m_fSeconds; }
	void SetPause(float fSeconds) { m_fSeconds = fSeconds; }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetPause());
	}

	auto operator==(const StopSegment& other) const -> bool
	{
		COMPARE_FLOAT(m_fSeconds);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const StopSegment&>(other));
	}

  private:
	/** @brief The number of seconds to pause at the segment's row. */
	float m_fSeconds;
};

/**
 * @brief Identifies when a song has a delay, or pump style stop.
 */
struct DelaySegment : public TimingSegment
{
	[[nodiscard]] auto GetType() const -> TimingSegmentType override
	{
		return SEGMENT_DELAY;
	}

	[[nodiscard]] auto GetEffectType() const -> SegmentEffectType override
	{
		return SegmentEffectType_Row;
	}

	[[nodiscard]] auto IsNotable() const -> bool override
	{
		return m_fSeconds > 0;
	}
	void DebugPrint() const override;

	[[nodiscard]] auto Copy() const -> TimingSegment* override
	{
		return new DelaySegment(*this);
	}

	DelaySegment(int iStartRow = ROW_INVALID, float fSeconds = 0)
	  : TimingSegment(iStartRow)
	  , m_fSeconds(fSeconds)
	{
	}

	DelaySegment(const DelaySegment& other)
	  : TimingSegment(other.GetRow())
	  , m_fSeconds(other.GetPause())
	{
	}

	[[nodiscard]] auto GetPause() const -> float { return m_fSeconds; }
	void SetPause(float fSeconds) { m_fSeconds = fSeconds; }

	[[nodiscard]] auto ToString(int dec) const -> std::string override;

	[[nodiscard]] auto GetValues() const -> std::vector<float> override
	{
		return std::vector<float>(1, GetPause());
	}

	auto operator==(const DelaySegment& other) const -> bool
	{
		COMPARE_FLOAT(m_fSeconds);
		return true;
	}

	auto operator==(const TimingSegment& other) const -> bool override
	{
		if (GetType() != other.GetType()) {
			return false;
		}

		return operator==(static_cast<const DelaySegment&>(other));
	}

  private:
	/** @brief The number of seconds to pause at the segment's row. */
	float m_fSeconds;
};

#undef COMPARE
#undef COMPARE_FLOAT

#endif
