#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "SongPosition.h"

static Preference<float> g_fVisualDelaySeconds("VisualDelaySeconds", 0.0f);

void
SongPosition::UpdateSongPosition(float fPositionSeconds,
								 const TimingData& timing,
								 const RageTimer& timestamp)
{

	if (!timestamp.IsZero())
		m_LastBeatUpdate = timestamp;
	else
		m_LastBeatUpdate.Touch();

	TimingData::GetBeatArgs beat_info;
	beat_info.elapsed_time = fPositionSeconds;
	timing.GetBeatAndBPSFromElapsedTime(beat_info);
	m_fSongBeat = beat_info.beat;
	m_fCurBPS = beat_info.bps_out;
	m_bFreeze = beat_info.freeze_out;
	m_bDelay = beat_info.delay_out;
	m_iWarpBeginRow = beat_info.warp_begin_out;
	m_fWarpDestination = beat_info.warp_dest_out;

	// "Crash reason : -243478.890625 -48695.773438"
	// The question is why is -2000 used as the limit? -aj
	ASSERT_M(m_fSongBeat > ARBITRARY_MIN_GAMEPLAY_NUMBER,
			 ssprintf("Song beat %f at %f seconds is less than -200000!",
					  m_fSongBeat,
					  fPositionSeconds));

	m_fMusicSeconds = fPositionSeconds;
	m_fSongBeatNoOffset =
	  timing.GetBeatFromElapsedTimeNoOffset(fPositionSeconds);

	m_fMusicSecondsVisible =
	  fPositionSeconds - (g_fVisualDelaySeconds.Get() *
						  GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate);
	beat_info.elapsed_time = m_fMusicSecondsVisible;
	timing.GetBeatAndBPSFromElapsedTime(beat_info);
	m_fSongBeatVisible = beat_info.beat;
}

void
SongPosition::Reset()
{

	m_fMusicSecondsVisible = 0;
	m_fSongBeatVisible = 0;

	m_fMusicSeconds = 0; // MUSIC_SECONDS_INVALID;
	m_fSongBeat = 0;
	m_fSongBeatNoOffset = 0;
	m_fCurBPS = 10;
	// m_bStop = false;
	m_bFreeze = false;
	m_bDelay = false;
	m_iWarpBeginRow =
	  -1; // Set to -1 because some song may want to warp to row 0. -aj
	m_fWarpDestination =
	  -1; // Set when a warp is encountered. also see above. -aj
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
class LunaSongPosition : public Luna<SongPosition>
{
  public:
	DEFINE_METHOD(GetMusicSecondsVisible, m_fMusicSecondsVisible);
	DEFINE_METHOD(GetSongBeatVisible, m_fSongBeatVisible);
	DEFINE_METHOD(GetMusicSeconds, m_fMusicSeconds);
	DEFINE_METHOD(GetSongBeat, m_fSongBeat);
	DEFINE_METHOD(GetSongBeatNoOffset, m_fSongBeatNoOffset);
	DEFINE_METHOD(GetCurBPS, m_fCurBPS);
	DEFINE_METHOD(GetFreeze, m_bFreeze);
	DEFINE_METHOD(GetDelay, m_bDelay);
	DEFINE_METHOD(GetWarpBeginRow, m_iWarpBeginRow);
	DEFINE_METHOD(GetWarpDestination, m_fWarpDestination);

	LunaSongPosition()
	{
		ADD_METHOD(GetMusicSecondsVisible);
		ADD_METHOD(GetSongBeatVisible);
		ADD_METHOD(GetMusicSeconds);
		ADD_METHOD(GetSongBeat);
		ADD_METHOD(GetSongBeatNoOffset);
		ADD_METHOD(GetCurBPS);
		ADD_METHOD(GetFreeze);
		ADD_METHOD(GetDelay);
		ADD_METHOD(GetWarpBeginRow);
		ADD_METHOD(GetWarpDestination);
	}
};

LUA_REGISTER_CLASS(SongPosition);
