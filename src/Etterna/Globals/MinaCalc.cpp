#include "MinaCalc.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <xmmintrin.h>
#include <cstring>
#include <string>
#include <set>
#include <unordered_set>
#include <deque>
#include <utility>
#include <cassert>
#include "Etterna/Globals/global.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"

using std::max;
using std::min;
using std::pair;
using std::pow;
using std::vector;

enum tap_size
{
	single,
	jump,
	hand,
	quad,
	num_tap_size
};

enum hands
{
	lh,
	rh,
	num_hands,
};

// cross column behavior between 2 notes
enum cc_type
{
	cc_left_right,
	cc_right_left,
	cc_jump_single,
	cc_single_single,
	cc_single_jump,
	cc_jump_jump,
	num_cc_types,
	cc_init,
};

enum meta_type
{
	meta_oht,
	meta_ccacc,
	meta_acca,
	meta_ccsjjscc,
	meta_ccsjjscc_inverted,
	meta_enigma,
	meta_meta_enigma,
	num_meta_types,
	meta_init,
};

// hand specific meaning the left two or right two columns and only for 4k
enum col_type
{
	col_left,
	col_right,
	col_ohjump,
	num_col_types,
	col_empty,
	col_init
};

static const std::string calc_params_xml = "Save/calc params.xml";
// intervals are _half_ second, no point in wasting time or cpu cycles on 100
// nps joke files
static const int max_nps_for_single_interval = 50;
static const vector<float> dimples_the_all_zero_output{ 0.F, 0.F, 0.F, 0.F,
														0.F, 0.F, 0.F, 0.F };
static const vector<float> gertrude_the_all_max_output{ 100.F, 100.F, 100.F,
														100.F, 100.F, 100.F,
														100.F, 100.F };
static const int num_cols_per_hand = 2;
static const int num_chart_cols = 4;
static const vector<int> col_ids = { 1, 2, 4, 8 };
static const unsigned hand_col_ids[2] = { 3, 12 };
static const int zto3[4] = { 0, 1, 2, 3 };
static const char note_map[16][5]{ "----", "1---", "-1--", "11--",
								   "--1-", "1-1-", "-11-", "111-",
								   "---1", "1--1", "-1-1", "11-1",
								   "--11", "1-11", "-111", "1111" };

static const col_type ct_loop[3] = { col_left, col_right, col_ohjump };

static const float s_init = -5.F;
static const float ms_init = 5000.F;

// neutral pattern mod value.. as opposed to min
static const float neutral = 1.F;

// DON'T WANT TO RECOMPILE HALF THE GAME IF I EDIT THE HEADER FILE
// global multiplier to standardize baselines
static const float finalscaler = 3.632F;

// ***note*** if we want max control over stamina we need to have one model for
// affecting the other skillsets to a certain degree, enough to push up longer
// stream ratings into contention with shorter ones, and another for both a more
// granular and influential modifier to calculate the end stamina rating with
// so todo on that

// Stamina Model params
static const float stam_ceil = 1.075234F; // stamina multiplier max
static const float stam_mag = 243.F;	  // multiplier generation scaler
static const float stam_fscale = 500.F; // how fast the floor rises (it's lava)
static const float stam_prop =
  0.69424F; // proportion of player difficulty at which stamina tax begins

// since we are no longer using the normalizer system we need to lower
// the base difficulty for each skillset and then detect pattern types
// to push down OR up, rather than just down and normalizing to a differential
// since chorded patterns have lower enps than streams, streams default to 1
// and chordstreams start lower
// stam is a special case and may use normalizers again
static const float basescalers[NUM_Skillset] = { 0.F,   0.97F,  0.92F, 0.83F,
												 0.94F, 0.715F, 0.73F, 0.95F };
bool debug_lmao = false;

#pragma region stuffs
// Relies on endiannes (significantly inaccurate)
inline auto
fastpow(double a, double b) -> float
{
	int u[2];
	std::memcpy(&u, &a, sizeof a);
	u[1] = static_cast<int>(b * (u[1] - 1072632447) + 1072632447);
	u[0] = 0;
	std::memcpy(&a, &u, sizeof a);
	return static_cast<float>(a);
}

// not super accurate, good enough for our purposes
inline auto
fastsqrt(float _in) -> float
{
	if (_in == 0.F) {
		return 0.F;
	}
	__m128 in = _mm_load_ss(&_in);
	float out;
	_mm_store_ss(&out, _mm_mul_ss(in, _mm_rsqrt_ss(in)));
	return out;
}

inline auto
ms_from(const float& now, const float& last) -> float
{
	return (now - last) * 1000.F;
}

inline auto
weighted_average(const float& a, const float& b, const float& x, const float& y)
  -> float
{
	return (x * a + ((y - x) * b)) / y;
}

template<typename T>
inline auto
CalcClamp(T x, T l, T h) -> T
{
	return x > h ? h : (x < l ? l : x);
}

// template thingy for generating basic proportion scalers for pattern mods
// potentially super broken
template<typename T>
inline auto
pmod_prop(T a,
		  T b,
		  const float& s,
		  const float& min,
		  const float& max,
		  const float& base = 0.F) -> float
{
	return CalcClamp(
	  (static_cast<float>(a) / static_cast<float>(b) * s) + base, min, max);
}

// template thingy for generating basic proportion scalers for pattern mods
// potentially super broken
template<typename T>
inline auto
pmod_prop(const float& pool,
		  T a,
		  T b,
		  const float& s,
		  const float& min,
		  const float& max) -> float
{
	return CalcClamp(
	  pool - (static_cast<float>(a) / static_cast<float>(b) * s), min, max);
}

inline auto
mean(const vector<float>& v) -> float
{
	return std::accumulate(begin(v), end(v), 0.F) /
		   static_cast<float>(v.size());
}

inline auto
mean(const vector<int>& v) -> float
{
	return std::accumulate(begin(v), end(v), 0) / static_cast<float>(v.size());
}

inline auto
mean(const unordered_set<int>& v) -> float
{
	return std::accumulate(begin(v), end(v), 0) / static_cast<float>(v.size());
}

// Coefficient of variation
inline auto
cv(const vector<float>& input) -> float
{
	float sd = 0.F;
	float average = mean(input);
	for (float i : input) {
		sd += (i - average) * (i - average);
	}

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

inline auto
cv(const vector<int>& input) -> float
{
	float sd = 0.F;
	float average = mean(input);
	for (int i : input) {
		sd +=
		  (static_cast<float>(i) - average) * (static_cast<float>(i) - average);
	}

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

inline auto
cv(const unordered_set<int>& input) -> float
{
	float sd = 0.F;
	float average = mean(input);
	for (int i : input) {
		sd +=
		  (static_cast<float>(i) - average) * (static_cast<float>(i) - average);
	}

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

// cv of a vector truncated to a set number of values, or if below, filled with
// dummy values to reach the desired num_vals
inline auto
cv_trunc_fill(const vector<float>& input,
			  const int& num_vals,
			  const float& ms_dummy) -> float
{
	int moop = static_cast<int>(input.size());
	float welsh_pumpkin = 0.F;
	float average = 0.F;
	if (moop >= num_vals) {
		for (int i = 0; i < min(moop, num_vals); ++i) {
			average += input[i];
		}
		average /= num_vals;

		for (int i = 0; i < min(moop, num_vals); ++i) {
			welsh_pumpkin += (input[i] - average) * (input[i] - average);
		}

		// prize winning, even
		return fastsqrt(welsh_pumpkin / static_cast<float>(num_vals)) / average;
	}

	for (int i = 0; i < min(moop, num_vals); ++i) {
		average += input[i];
	}

	// fill with dummies if input is below desired number of values
	for (int i = 0; i < num_vals - moop; ++i) {
		average += ms_dummy;
	}
	average /= num_vals;

	for (int i = 0; i < min(moop, num_vals); ++i) {
		welsh_pumpkin += (input[i] - average) * (input[i] - average);
	}

	for (int i = 0; i < num_vals - moop; ++i) {
		welsh_pumpkin += (ms_dummy - average) * (ms_dummy - average);
	}

	return fastsqrt(welsh_pumpkin / static_cast<float>(num_vals)) / average;
}

inline auto
sum_trunc_fill(const vector<float>& input,
			   const int& num_vals,
			   const float& ms_dummy) -> float
{
	int moop = static_cast<int>(input.size());
	float smarmy_hamster = 0.F;
	// use up to num_vals
	for (int i = 0; i < min(moop, num_vals); ++i) {
		smarmy_hamster += input[i];
	}

	// got enough
	if (moop >= num_vals) {
		return smarmy_hamster;
	}

	// fill with dummies if input is below desired number of values
	for (int i = 0; i < num_vals - static_cast<int>(moop); ++i) {
		smarmy_hamster += ms_dummy;
	}

	// real piece of work this guy
	return smarmy_hamster;
}

inline auto
downscale_low_accuracy_scores(float f, float sg) -> float
{
	return sg >= 0.93F
			 ? f
			 : min(max(f / pow(1.F + (0.93F - sg), 0.75F), 0.F), 100.F);
}

inline void
Smooth(vector<float>& input, float neutral)
{
	float f1;
	float f2 = neutral;
	float f3 = neutral;

	for (float& i : input) {
		f1 = f2;
		f2 = f3;
		f3 = i;
		i = (f1 + f2 + f3) / 3.F;
	}
}

inline void
Smooth(vector<vector<float>>& input, float neutral)
{
	float f1;
	float f2 = neutral;
	for (auto& itv : input) {
		for (float& i : itv) {
			f1 = f2;
			f2 = i;
			i = (f1 + f2 * 2.F) / 3.F;
		}
	}
}

inline void
DifficultyMSSmooth(vector<float>& input)
{
	float f1;
	float f2 = 0.F;

	for (float& i : input) {
		f1 = f2;
		f2 = i;
		i = (f1 + f2) / 2.F;
	}
}

inline auto
AggregateScores(const vector<float>& skillsets, float rating, float resolution)
  -> float
{
	float sum;
	for (int iter = 1; iter <= 11; iter++) {
		do {
			rating += resolution;
			sum = 0.0F;
			for (float i : skillsets) {
				sum += 2.F / std::erfc(0.5F * (i - rating)) - 1.F;
			}
		} while (3 < sum);
		rating -= resolution;
		resolution /= 2.F;
	}
	return rating + 2.F * resolution;
}

inline auto
column_count(unsigned int note) -> unsigned int
{
	return note % 2 + note / 2 % 2 + note / 4 % 2 + note / 8 % 2;
}

auto
chord_proportion(const vector<NoteInfo>& NoteInfo, const int chord_size)
  -> float
{
	unsigned int taps = 0;
	unsigned int chords = 0;

	for (auto row : NoteInfo) {
		unsigned int notes = column_count(row.notes);
		taps += notes;
		if (notes == chord_size) {
			chords += notes;
		}
	}
	assert(taps > 0);
	return static_cast<float>(chords) / static_cast<float>(taps);
}

inline auto
max_val(vector<int>& v) -> int
{
	return *std::max_element(v.begin(), v.end());
}

inline auto
max_val(vector<float>& v) -> float
{
	return *std::max_element(v.begin(), v.end());
}

inline auto
max_index(vector<float>& v) -> int
{
	return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

inline auto
sum(vector<int>& v) -> int
{
	return std::accumulate(begin(v), end(v), 0);
}

inline auto
sum(deque<int>& v) -> int
{
	return std::accumulate(begin(v), end(v), 0);
}

inline auto
sum(vector<float>& v) -> float
{
	return std::accumulate(begin(v), end(v), 0.F);
}

void
Calc::TotalMaxPoints()
{
	MaxPoints = 0;
	for (int i = 0; i < left_hand.v_itvpoints.size(); i++) {
		MaxPoints += left_hand.v_itvpoints[i] + right_hand.v_itvpoints[i];
	}
}

void
Hand::InitPoints(const Finger& f1, const Finger& f2)
{
	v_itvpoints.clear();
	for (int ki_is_rising = 0; ki_is_rising < f1.size(); ++ki_is_rising) {
		v_itvpoints.emplace_back(f1[ki_is_rising].size() +
								 f2[ki_is_rising].size());
	}
}

inline auto
div_high_by_low(float a, float b) -> float
{
	if (b > a) {
		std::swap(a, b);
	}
	return a / b;
}

inline auto
div_low_by_high(float a, float b) -> float
{
	if (b > a) {
		std::swap(a, b);
	}
	return b / a;
}

inline auto
diff_high_by_low(int a, int b) -> int
{
	if (b > a) {
		std::swap(a, b);
	}
	return a - b;
}

#pragma endregion utils are an antipattern

#pragma region CalcBodyFunctions
#pragma region JackModelFunctions
inline void
Calc::JackStamAdjust(float x, int t, int mode, bool debug)
{
	const bool dbg = false;

	float stam_ceil = 1.075234F;
	float stam_mag = 75.F;
	float stam_fscale = 256.F;
	float stam_prop = 0.55424F;
	float stam_floor = 1.F;
	float mod = 1.F;
	float avs1 = 0.F;
	float avs2 = 0.F;
	float local_ceil = stam_ceil;
	float super_stam_ceil = 1.11F;
	if (mode == 4) {
		super_stam_ceil = 1.09F;
	}
	const auto& diff = jacks[mode][t];

	if (debug) {
		left_hand.debugValues[2][JackStamMod].resize(numitv);
		right_hand.debugValues[2][JackStamMod].resize(numitv);

		// each interval
		for (int i = 0; i < diff.size(); ++i) {
			float mod_sum = 0.F;
			// each jack in the interval
			for (int j = 0; j < diff[i].size(); ++j) {
				avs1 = avs2;
				avs2 = diff[i][j];

				if (dbg) {
					std::cout << "mod was : " << mod
							  << " at diff : " << diff[i][j] << std::endl;
				}

				mod +=
				  ((((avs1 + avs2) / 2.F) / (stam_prop * x)) - 1.F) / stam_mag;
				if (mod > 0.95F) {
					stam_floor += (mod - 0.95F) / stam_fscale;
				}
				local_ceil = stam_ceil * stam_floor;

				mod =
				  min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);

				if (dbg) {
					std::cout << "mod now : " << mod << std::endl;
				}

				mod_sum += mod;
				stam_adj_jacks[t][i][j] = diff[i][j] * mod;
			}
			// yes i know it's 1 col per hand atm
			float itv_avg = 1.F;
			if (diff[i].size() > 1) {
				itv_avg = mod_sum / static_cast<float>(diff[i].size());
			}

			if (t == 0) {
				left_hand.debugValues[2][JackStamMod][i] = itv_avg;
			} else if (t == 3) {
				right_hand.debugValues[2][JackStamMod][i] = itv_avg;
			}
		}
	} else {
		for (int i = 0; i < diff.size(); ++i) {
			for (int j = 0; j < diff[i].size(); ++j) {
				avs1 = avs2;
				avs2 = diff[i][j];
				mod +=
				  ((((avs1 + avs2) / 2.F) / (stam_prop * x)) - 1.F) / stam_mag;
				if (mod > 0.95F) {
					stam_floor += (mod - 0.95F) / stam_fscale;
				}
				local_ceil = stam_ceil * stam_floor;

				mod =
				  min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
				stam_adj_jacks[t][i][j] = diff[i][j] * mod;
			}
		}
	}
}
static const float magic_num = 7.5F;
inline auto
hit_the_road(float x, float y, int  /*mode*/) -> float
{
	return (CalcClamp(
	  magic_num - (magic_num * fastpow(x / y, 2.5F)), 0.F, magic_num));
}

// returns a positive number or 0, output should be subtracted
auto
Calc::JackLoss(float x, int mode, float mpl, bool stam, bool debug) -> float
{
	// mpl *= 1.5f;
	const bool dbg = false;
	// adjust for stam before main loop, since main loop is interval -> track
	// and not track -> interval, we could also try doing this on the fly with
	// an inline but i cba to mess with that atm
	if (stam) {
		for (auto t : { 0, 1, 2, 3 }) {
			JackStamAdjust(x, t, mode, debug);
		}
	}

	float total_point_loss = 0.F;
	//  we should just store jacks in intervals in the first place
	vector<float> left_loss(numitv);
	vector<float> right_loss(numitv);
	vector<float> flurbo(4);
	// this is pretty gross but for now lets treat every same hand jump like a
	// jumpjack, take the max between the cols on each hand, meaning we have to
	// loop through tracks in the interval loop
	for (int i = 0; i < numitv; ++i) {
		// yes i think this does just have to be this slow
		for (auto& t : zto3) {
			const auto& seagull =
			  stam ? stam_adj_jacks[t][i] : jacks[mode][t][i];
			float loss = 0.F;
			for (auto& j : seagull) {
				if (x >= j) {
					continue;
				}
				loss += hit_the_road(x, j, mode);

				if (dbg) {
					std::cout << "loss for diff : " << j
							  << " with pskill: " << x << " : "
							  << hit_the_road(x, j, mode) << std::endl;
				}
			}
			flurbo[t] = loss;
		}

		if (debugmode) {
			left_loss[i] = max(flurbo[0], flurbo[1]);
			right_loss[i] = max(flurbo[2], flurbo[3]);
			// slight optimization i guess, bail if we can no longer reach score
			// goal (but only outside of debug, //////and not for minijacks)
		} else if (total_point_loss > mpl) {
			return total_point_loss;
		}

		total_point_loss += max(flurbo[0], flurbo[1]);
		total_point_loss += max(flurbo[2], flurbo[3]);
	}
	if (debugmode) {
		left_hand.debugValues[2][JackPtLoss] = left_loss;
		right_hand.debugValues[2][JackPtLoss] = right_loss;
	}

	total_point_loss = CalcClamp(total_point_loss, 0.F, 10000.F);
	return total_point_loss;
}

inline auto
ms_to_bpm(float x) -> float
{
	return 15000.F / x;
}

void
Calc::SequenceJack(const Finger& f, int track, int mode)
{
	// the 4 -> 5 note jack difficulty spike is well known, we aim to reflect
	// this phenomena as best as possible. 500, 50, 50, 50, 50 should end up
	// significantly more difficult than 50, 50, 50, 50, 50

	// NOTE: in comments here "jack" will refer to exclusively the time between
	// two taps on the same column, a "sequence" of jacks is n number of jacks
	// which contain n + 1 number of total taps, a "component" of a sequence can
	// be any n consecutive jacks within a sequence. we are operating under the
	// assumption that the difficulty of any jack in any sequence is entirely
	// dependent on the previous components of the sequence, and what comes
	// afterwards is not relevant (usually true in actual gameplay, outside of
	// stuff like mines immediately after shortjacks)

	const int mode_windows[4] = { 1, 2, 3, 4 };

	int window_size = mode_windows[mode];
	vector<float> window_taps;
	window_taps.reserve(window_size);
for (int i = 0; i < window_size; ++i) {
		window_taps.push_back(1250.F);
	}

	vector<float> eff_scalers(window_size);

	// doge adventure etc (maybe don't set this too low yet)
	static const float max_diff = 55.F;

	// yes this is many loops, but we don't want to sacrifice legitimately
	// difficult minijacks in the name of proper evaluation of shortjacks and
	// longjack, so we're going to be dumb and hacky and run 3 separate passes
	// trying to identify the strength of each minijack type, skillsets within
	// the skillset as it were. An attempt was made to simply take the highest
	// jack value of any type for each value but this resulted in the
	// distribution being stretched out too far, we get better grouping this
	// way. Think jack speed skillsets. It sounds dumb if you only farmed
	// overrated files on 263 but really the difference between vibro, longjack
	// control, burst jacks and minijacks is quite vast

	// intervals, we don't care that we're looping through intervals because the
	// queue we build is interval agnostic, though it does make debug output
	// easier to accomplish

	float time = 0.F;
	float eff_ms = 0.F;
	float eff_bpm = 0.F;
	float ms = 0.F;
	const float mode_buffers[4] = { 12.5F, 250.F, 120.F, 225.F };
	static const float jack_global_scaler =
	  finalscaler * basescalers[Skill_JackSpeed] / 15.F;
	static const float mode_scalers[4] = {
		0.9555F, 0.003F * 35.12F / 36.F, 1.28F, 1.5F * 30.5F / 29.5F
	};
	jacks[mode][track].resize(numitv);
	float comp_diff = 0.F;
	for (int itv = 0; itv < f.size(); ++itv) {
		jacks[mode][track][itv].resize(f[itv].size());
		// taps in interval
		for (int ind = 0; ind < f[itv].size(); ++ind) {
			ms = f[itv][ind];
			time += ms;

			// shift older values back
			for (int i = 1; i < window_taps.size(); ++i) {
				window_taps[i - 1] = window_taps[i];
			}
			// add new value
			window_taps[window_size - 1] = ms;

			// effective bpm based on a hit window buffer
			eff_ms = sum(window_taps) + mode_buffers[mode];
			eff_bpm = ms_to_bpm(eff_ms / window_taps.size());
			if (mode == 1) {
				eff_bpm = pow(ms_to_bpm(eff_ms / window_taps.size()), 2.5F);
			}
			comp_diff = eff_bpm * jack_global_scaler;

			jacks[mode][track][itv][ind] =
			  CalcClamp(comp_diff * mode_scalers[mode], 0.F, max_diff);
		}
	}
}
#pragma endregion

auto
Calc::ProcessFinger(const vector<NoteInfo>& NoteInfo,
					unsigned int t,
					float music_rate,
					float offset,
					bool& joke_file_mon) -> Finger
{
	// optimization, just allocate memory here once and recycle this vector
	vector<float> temp_queue(max_nps_for_single_interval);
	vector<int> temp_queue_two(max_nps_for_single_interval);
	unsigned int row_counter = 0;
	unsigned int row_counter_two = 0;

	int Interval = 0;
	float last = -5.F;
	Finger AllIntervals(numitv, vector<float>());
	if (t == 0) {
		nervIntervals = vector<vector<int>>(numitv, vector<int>());
	}
	unsigned int column = 1U << t;

	for (int i = 0; i < NoteInfo.size(); i++) {
		// we have hardcoded mem allocation for up to 100 nps, bail out on the
		// entire file calc if we exceed that
		if (row_counter >= max_nps_for_single_interval ||
			row_counter_two >= max_nps_for_single_interval) {
			// yes i know this is jank
			joke_file_mon = true;
			return {};
		}
		float scaledtime = (NoteInfo[i].rowTime / music_rate) + offset;

		while (scaledtime > static_cast<float>(Interval + 1) * IntervalSpan) {
			// dump stored values before iterating to new interval
			// we're in a while loop to skip through empty intervals
			// so check the counter to make sure we didn't already assign
			if (row_counter > 0) {
				AllIntervals[Interval].resize(row_counter);
				for (unsigned int n = 0; n < row_counter; ++n) {
					AllIntervals[Interval][n] = temp_queue[n];
				}
			}

			if (row_counter_two > 0) {
				nervIntervals[Interval].resize(row_counter_two);
				for (unsigned int n = 0; n < row_counter_two; ++n) {
					nervIntervals[Interval][n] = temp_queue_two[n];
				}
			}

			// reset the counter and iterate interval
			row_counter = 0;
			row_counter_two = 0;
			++Interval;
		}

		if ((NoteInfo[i].notes & column) != 0U) {
			// log all rows for this interval in pre-allocated mem
			// this is clamped to stop 192nd single minijacks from having an
			// outsize influence on anything, they aren't actually that hard in
			// isolation due to hit windows
			temp_queue[row_counter] =
			  CalcClamp(1000.F * (scaledtime - last), 40.F, 5000.F);
			++row_counter;
			last = scaledtime;
		}

		if (t == 0 && NoteInfo[i].notes != 0) {
			temp_queue_two[row_counter_two] = i;
			++row_counter_two;
		}
	}
	return AllIntervals;
}

auto
Calc::CalcMain(const vector<NoteInfo>& NoteInfo,
			   float music_rate,
			   float score_goal) -> vector<float>
{
	// actual cancer
	debug_lmao = debugmode;

	// in flux
	float grindscaler =
	  CalcClamp(
		0.9F + (0.1F * ((NoteInfo.back().rowTime / music_rate) - 35.F) / 35.F),
		0.9F,
		1.F) *
	  CalcClamp(
		0.9F + (0.1F * ((NoteInfo.back().rowTime / music_rate) - 15.F) / 15.F),
		0.9F,
		1.F) *
	  CalcClamp(
		0.4F + (0.6F * ((NoteInfo.back().rowTime / music_rate) - 10.F) / 10.F),
		0.4F,
		1.F);

	float jprop = chord_proportion(NoteInfo, 2);
	float hprop = chord_proportion(NoteInfo, 3);
	float qprop = chord_proportion(NoteInfo, 4);
	float cprop = jprop + hprop + qprop;

	// for multi offset passes- super breaks stuff atm dunno why???
	// const int fo_rizzy = ssr ? 5 : 1;
	const int fo_rizzy = 1;
	vector<vector<float>> the_hizzle_dizzles(fo_rizzy);
	for (int WHAT_IS_EVEN_HAPPEN_THE_BOMB = 0;
		 WHAT_IS_EVEN_HAPPEN_THE_BOMB < fo_rizzy;
		 ++WHAT_IS_EVEN_HAPPEN_THE_BOMB) {

		bool continue_calc = InitializeHands(
		  NoteInfo, music_rate, 0.1F * WHAT_IS_EVEN_HAPPEN_THE_BOMB);

		// if we exceed max_nps_for_single_interval during
		// processing
		if (!continue_calc) {
			std::cout << "skipping junk file" << std::endl;
			return gertrude_the_all_max_output;
		}

		TotalMaxPoints();

		vector<float> mcbloop(NUM_Skillset);
		// overall and stam will be left as 0.f by this loop
		for (int i = 0; i < NUM_Skillset; ++i) {
			mcbloop[i] = Chisel(0.1F, 10.24F, score_goal, i, false);
		}

		// stam is based on which calc produced the highest
		// output without it
		int highest_base_skillset = max_index(mcbloop);
		float base = mcbloop[highest_base_skillset];

		// rerun all with stam on, optimize by starting at
		// the non-stam adjusted base value for each
		// skillset we can actually set the stam floor to <
		// 1 to shift the curve a bit
		// do we actually need to rerun _all_ with stam on?
		// we gain significant speed from not doing so, however the tradeoff is
		// files that are close in 2/3 skillsets will have the stam bonus
		// stripped from the second and third components, devaluing the file as
		// a whole, we could run it for the 2nd/3rd highest skillsets but i'm
		// too lazy to implement that right now, the major concern here is the
		// cost of jack stam, so i think we can just get away with throwing out
		// jack stam calculations for anything that isn't jackspeed (or tech
		// since atm they're doubling up a bit)
		for (int i = 0; i < NUM_Skillset; ++i) {
			if (i == Skill_JackSpeed) {
				if (highest_base_skillset == Skill_JackSpeed ||
					highest_base_skillset == Skill_Technical) {
					mcbloop[i] =
					  Chisel(mcbloop[i] * 1.F, 0.32F, score_goal, i, true);
				}
			} else {
				mcbloop[i] =
				  Chisel(mcbloop[i] * 0.9F, 0.32F, score_goal, i, true);
			}
		}

		// all relative scaling to specific skillsets should
		// occur before this point, not after (it ended up
		// this way due to the normalizers which were dumb
		// and removed) stam is the only skillset that
		// can/should be normalized to base values without
		// interfering with anything else (since it's not
		// based on a type of pattern)

		// stam jams, stamina should push up the base
		// ratings for files so files that are more
		// difficult by virtue of being twice as long for
		// more or less the same patterns don't get
		// underrated, however they shouldn't be pushed up a
		// huge amount either, we want high stream scores to
		// be equally achieveable on longer or shorter
		// files, ideally, the stam ratings itself is a
		// separate consideration and will be scaled to the
		// degree to which the stamina model affects the
		// base rating, so while stamina should affect the
		// base skillset ratings slightly we want the degree
		// to which it makes files harder to be catalogued
		// as the stamina rating scaling down stuff that has
		// no stamina component will help preventing
		// pollution of stamina leaderboards with charts
		// that are just very high rated but take no stamina
		float poodle_in_a_porta_potty = mcbloop[highest_base_skillset];

		// super lazy hack to make jackspeed not give stam
		if (highest_base_skillset == Skill_JackSpeed) {
			poodle_in_a_porta_potty *= 0.9F;
		}

		// the bigger this number the more stamina has to
		// influence a file before it counts in the stam
		// skillset, i.e. something that only benefits 2%
		// from the stam modifiers will drop below the 1.0
		// mark and move closer to 0 with the pow, resulting
		// in a very low stamina rating (we want this),
		// something that benefits 5.5% will have the 0.5%
		// overflow multiplied and begin gaining some stam,
		// and something that benefits 15% will max out the
		// possible stam rating, which is (currently) a 1.07
		// multiplier to the base maybe using a multiplier
		// and not a difference would be better?
		static const float stam_curve_shift = 0.015F;
		// ends up being a multiplier between ~0.8 and ~1
		float mcfroggerbopper =
		  pow((poodle_in_a_porta_potty / base) - stam_curve_shift, 2.5F);

		// we wanted to shift the curve down a lot before
		// pow'ing but it was too much to balance out, so we
		// need to give some back, this is roughly
		// equivalent of multiplying by 1.05 but also not
		// really because math we don't want to push up the
		// high end stuff anymore so just add to let stuff
		// down the curve catch up a little remember we're
		// operating on a multiplier
		mcfroggerbopper = CalcClamp(mcfroggerbopper, 0.8F, 1.08F);
		mcbloop[Skill_Stamina] = poodle_in_a_porta_potty * mcfroggerbopper *
								 basescalers[Skill_Stamina];

		// sets the 'proper' debug output, doesn't
		// (shouldn't) affect actual values this is the only
		// time debugoutput arg should be set to true
		if (debugmode) {
			Chisel(mcbloop[highest_base_skillset] - 0.16F,
				   0.32F,
				   score_goal,
				   highest_base_skillset,
				   true,
				   true);
		}

		// ZZZZZZZZZZZZZZZZZz
		pair<Hand&, vector<int>> spoopy[2] = { { left_hand, { 1, 2 } },
											   { right_hand, { 4, 8 } } };
		if (debugmode) {
			for (auto& hp : spoopy) {
				auto& h = hp.first;

				if (highest_base_skillset == Skill_Technical) {
					h.debugValues[0][TotalPatternMod].resize(numitv);
					for (int i = 0; i < h.soap[BaseNPS].size(); ++i) {
						float val = h.base_adj_diff[highest_base_skillset][i] /
									h.soap[BaseMSD][i];
						h.debugValues[0][TotalPatternMod][i] = val;
					}
				} else if (highest_base_skillset == Skill_Chordjack) {
					h.debugValues[0][TotalPatternMod].resize(numitv);
					for (int i = 0; i < h.soap[BaseNPS].size(); ++i) {
						float val = h.base_adj_diff[highest_base_skillset][i] /
									(h.soap[BaseMSD][i] + h.soap[BaseNPS][i]) /
									2.F;
						h.debugValues[0][TotalPatternMod][i] = val;
					}
				} else {
					h.debugValues[0][TotalPatternMod].resize(numitv);
					for (int i = 0; i < h.soap[BaseNPS].size(); ++i) {
						float val = h.base_adj_diff[highest_base_skillset][i] /
									h.soap[BaseNPS][i];
						h.debugValues[0][TotalPatternMod][i] = val;
					}
				}
			}
		}

		// the final push down, cap ssrs (score specific
		// ratings) to stop vibro garbage and calc abuse
		// from polluting leaderboards too much, a "true" 38
		// is still unachieved so a cap of 40 [sic] is
		// _extremely_ generous do this for SCORES only, not
		// cached file difficulties
		if (ssr) {
			static const float ssrcap = 40.F;
			for (auto& r : mcbloop) {
				// so 50%s on 60s don't give 35s
				r = downscale_low_accuracy_scores(r, score_goal);
				r = CalcClamp(r, r, ssrcap);
			}
		}

		// finished all modifications to skillset values, set overall
		mcbloop[Skill_Overall] = max_val(mcbloop);

		for (float bagles : mcbloop) {
			the_hizzle_dizzles[WHAT_IS_EVEN_HAPPEN_THE_BOMB].push_back(bagles);
		}
	}
	vector<float> yo_momma(NUM_Skillset);
	for (int farts = 0; farts < the_hizzle_dizzles[0].size(); ++farts) {
		vector<float> girls;
		girls.reserve(the_hizzle_dizzles.size());
for (auto& the_hizzle_dizzle : the_hizzle_dizzles) {
			girls.push_back(the_hizzle_dizzle[farts]);
		}
		yo_momma[farts] = mean(girls) * grindscaler;
		girls.clear();
	}

	return yo_momma;
}
#pragma endregion

#pragma region sequencing logic definitions

inline auto
is_col_type_single_tap(const col_type& col) -> bool
{
	return col == col_left || col == col_right;
}

static inline auto
determine_col_type(const unsigned& notes, const unsigned& hand_id) -> col_type
{
	unsigned shirt = notes & hand_id;
	if (shirt == 0) {
		return col_empty;
	}

	if (hand_id == 3) {
		if (shirt == 3) {
			return col_ohjump;
		}
		if (shirt == 1) {
			return col_left;
		}
		if (shirt == 2) {
			return col_right;
		}
	} else if (hand_id == 12) {
		if (shirt == 12) {
			return col_ohjump;
		}
		if (shirt == 8) {
			return col_right;
		}
		if (shirt == 4) {
			return col_left;
		}
	}
	assert(0);
	return col_init;
}

// inverting col state for col_left or col_right only
inline auto
invert_col(const col_type& col) -> col_type
{
	// assert(col == col_left || col == col_right);
	return col == col_left ? col_right : col_left;
}

// inverting cc state for left_right or right_left only
inline auto
invert_cc(const cc_type& cc) -> cc_type
{
	// assert(cc == cc_left_right || cc == cc_right_left);
	return cc == cc_left_right ? cc_right_left : cc_left_right;
}
#pragma endregion

#pragma region noteinfo bitwise operations
// bitwise operations on noteinfo.notes, they must be unsigned ints, and
// shouldn't be called on enums or row counts or anything like that
inline auto
is_single_tap(const unsigned& a) -> bool
{
	return (a & (a - 1)) == 0;
}

// between two successive rows usually... but i suppose this could be called
// outside of that limitation
inline auto
is_jack_at_col(const unsigned& id,
			   const unsigned& row_notes,
			   const unsigned& last_row_notes) -> bool
{
	return ((id & row_notes) != 0U) && ((id & last_row_notes) != 0U);
}

// doesn't check for jacks
inline auto
is_alternating_chord_single(const unsigned& a, const unsigned& b) -> bool
{
	return (a > 1 && b == 1) || (a == 1 && b > 1);
}

// ok lets stop being bad, find 1[n]1 or [n]1[n] with no jacks between first and
// second and second and third elements
inline auto
is_alternating_chord_stream(const unsigned& a,
							const unsigned& b,
							const unsigned& c) -> bool
{
	if (is_single_tap(a)) {
		if (is_single_tap(b)) {
			// single single, don't care, bail
			return false;
		}
		if (!is_single_tap(c)) { // single, chord, chord, bail
			return false;
		}
	} else {
		if (!is_single_tap(b)) {
			// chord chord, don't care, bail
			return false;
		}
		if (is_single_tap(c)) { // chord, single, single, bail
			return false;
		}
	}
	// we have either 1[n]1 or [n]1[n], check for any jacks
	return static_cast<int>(((a & b) != 0U) && ((b & c) != 0U)) == 0;
}
#pragma endregion

#pragma region moving window array helpers
static const int max_moving_window_size = 6;
// idk what im doin
template<typename T>
struct CalcWindow
{
	// ok there's actually a good reason for indexing this way because it's more
	// intuitive since we are scanning row by row the earliest values in the
	// window are the oldest
	inline void operator()(const T& new_val)
	{
		// update the window
		for (int i = 1; i < max_moving_window_size; ++i) {
			_itv_vals[i - 1] = _itv_vals[i];
		}

		// set new value at size - 1
		_itv_vals[max_moving_window_size - 1] = new_val;
	}

	// return type T
	inline auto operator[](const int& pos) const -> T
	{
		assert(pos > 0 && pos < max_moving_window_size);
		return _itv_vals[pos];
	}

	// return type T
	[[nodiscard]] inline auto get_now() const -> T
	{
		return _itv_vals[max_moving_window_size - 1];
	}
	[[nodiscard]] inline auto get_last() const -> T
	{
		return _itv_vals[max_moving_window_size - 2];
	}

	// return type T
	[[nodiscard]] inline auto get_total_for_window(const int& window) const -> T
	{
		T o = static_cast<T>(0);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals[i];
		}

		return o;
	}

	// return type T
	[[nodiscard]] inline auto get_max_for_window(const int& window) const -> T
	{
		T o = static_cast<T>(0);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o = _itv_vals[i] > o ? _itv_vals[i] : o;
		}

		return o;
	}

	// return type float
	[[nodiscard]] inline auto get_mean_of_window(const int& window) const
	  -> float
	{
		T o = static_cast<T>(0);

		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals[i];
		}

		return static_cast<float>(o) / static_cast<float>(window);
	}

	// return type float
	[[nodiscard]] inline auto get_total_for_windowf(const int& window) const
	  -> float
	{
		float o = 0.F;
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals[i];
		}

		return o;
	}

	// return type float
	[[nodiscard]] inline auto get_cv_of_window(const int& window) const -> float
	{
		float sd = 0.F;
		float avg = get_mean_of_window(window);

		// if window is 4, we check values 6/5/4/3, since this window is always
		// 6
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			sd += (static_cast<float>(_itv_vals[i]) - avg) *
				  (static_cast<float>(_itv_vals[i]) - avg);
		}

		return fastsqrt(sd / static_cast<float>(window)) / avg;
	}

	// set everything to zero
	inline void zero()
	{
		for (auto& v : _itv_vals) {
			v = static_cast<T>(0);
		}
	}

	CalcWindow() { zero(); }

  protected:
	T _itv_vals[max_moving_window_size];
};
#pragma endregion

#pragma region new pattern mod structure
// meta info is information that is derived from two or more consecutive
// noteinfos, the first level of pattern abstraction is generated from noteinfo
// , the second level of abstraction is generated from the meta info produced by
// the first level of abstraction, and so on and so forth. meta info is
// constructed on the fly per row and not preserved in memory unless ulbu is in
// debug and each row's meta info is able to look back 3-4 rows into the past
// for relevant pattern information, in that sense meta info contains
// information that persists beyond the explicit bounds of its generation point,
// and that information may carry forward into the next generation point,
// functionally speaking this has the effect of carrying pattern sequence
// detection through intervals, reducing the error caused by interval splicing

// accumulates raw note info across an interval as it's processed by row
struct ItvInfo
{
	int total_taps = 0;
	int chord_taps = 0;
	int taps_by_size[num_tap_size] = { 0, 0, 0, 0 };
	int mixed_hs_density_tap_bonus = 0;

	// resets all the stuff that accumulates across intervals
	inline void reset()
	{
		// self explanatory
		total_taps = 0;

		// number of non-single taps
		chord_taps = 0;

		for (auto& t : taps_by_size) {
			t = 0;
		}
	}

	inline void update_tap_counts(const int& row_count)
	{
		total_taps += row_count;

		// ALWAYS COUNT NUMBER OF TAPS IN CHORDS
		if (row_count > 1) {
			chord_taps += row_count;
		}

		// ALWAYS COUNT NUMBER OF TAPS IN CHORDS
		taps_by_size[row_count - 1] += row_count;

		// maybe move this to metaitvinfo?
		// we want mixed hs/js to register as hs, even at relatively sparse hand
		// density
		if (taps_by_size[hand] > 0) {
			// this seems kinda extreme? it'll add the number of jumps in the
			// whole interval every hand? maybe it needs to be that extreme?
			mixed_hs_density_tap_bonus += taps_by_size[jump];
		}
	}
};
// meta info for intervals
struct metaItvInfo
{
	ItvInfo _itvi;

	int _idx = 0;
	// meta info for this interval extracted from the base noterow progression
	int seriously_not_js = 0;
	int definitely_not_jacks = 0;
	int actual_jacks = 0;
	int actual_jacks_cj = 0;
	int not_js = 0;
	int not_hs = 0;
	int zwop = 0;
	int shared_chord_jacks = 0;
	bool dunk_it = false;

	// ok new plan instead of a map, keep an array of 3, run a comparison loop
	// that sets 0s to a new value if that value doesn't match any non 0 value,
	// and set a bool flag if we have filled the array with unique values
	unsigned row_variations[3] = { 0, 0, 0 };
	int num_var = 0;
	// unique(noteinfos for interval) < 3, or row_variations[2] == 0 by interval
	// end
	bool basically_vibro = true;

	inline void reset(const int& idx)
	{
		_idx = idx;
		// isn't reset, preserve behavior. this essentially just tracks longer
		// sequences of single notes, we don't want it to be reset with
		// intervals, also there's probably a better way to implement this setup
		// seriously_not_js = 0;

		// alternating chordstream detected (used by cj only atm)
		definitely_not_jacks = 0;

		// number of shared jacks between to successive rows, used by js/hs to
		// depress jumpjacks
		actual_jacks = 0;

		// almost same thing as above (see comment in jack_scan)
		actual_jacks_cj = 0;

		// increased by detecting either long runs of single notes
		// (definitely_not_jacks > 3) or by encountering jumptrills, either
		// splithand or two hand, not_js and not_hs are the same thing, this
		// entire operation and setup should probably be split up and made more
		// explicit in each thing it detects and how those things are used
		not_js = 0;
		not_hs = 0;

		// recycle var for any int assignments
		zwop = 0;

		// self explanatory and unused atm
		shared_chord_jacks = 0;

		for (auto& t : row_variations) {
			t = 0;
		}
		num_var = 0;

		// see def
		basically_vibro = true;
		dunk_it = false;
		// reset our interval info ref
		_itvi.reset();
	}
};
struct metaRowInfo
{
	static const bool dbg = false;
	static const bool dbg_lv2 = false;

	float time = s_init;
	// time from last row (ms)
	float ms_now = ms_init;
	int count = 0;
	int last_count = 0;
	int last_last_count = 0;
	unsigned notes = 0;
	unsigned last_notes = 0;
	unsigned last_last_notes = 0;

	// per row bool flags, these must be directly set every row
	bool alternating_chordstream = false;
	bool alternating_chord_single = false;
	bool gluts_maybe = false; // not really used/tested yet
	bool twas_jack = false;

	inline void set_row_variations(metaItvInfo& mitvi)
	{
		// already determined there's enough variation in this interval
		if (!mitvi.basically_vibro) {
			return;
		}

		// trying to fill array with up to 3 unique row_note configurations
		for (auto& t : mitvi.row_variations) {
			// already a stored value here
			if (t != 0) {
				// already have one of these
				if (t == notes) {
					return;
				}
			} else if (t == 0) {
				// nothing stored here and isn't a duplicate, store it and
				// iterate num_var
				t = notes;
				++mitvi.num_var;

				// check if we filled the array with unique values. since we
				// start by assuming anything is basically vibro, set the flag
				// to false if it is
				if (mitvi.row_variations[2] != 0) {
					mitvi.basically_vibro = false;
				}
				return;
			}
		}
	}

	// scan for jacks and jack counts between this row and the last
	inline void jack_scan(metaItvInfo& mitvi)
	{
		twas_jack = false;

		for (auto& id : col_ids) {
			if (is_jack_at_col(id, notes, last_notes)) {
				if (dbg_lv2) {
					std::cout << "actual jack with notes: " << note_map[notes]
							  << " : " << note_map[last_notes] << std::endl;
				}
				// not scaled to the number of jacks anymore
				++mitvi.actual_jacks;
				twas_jack = true;
				// try to pick up gluts maybe?
				if (count > 1 && column_count(last_notes) > 1) {
					++mitvi.shared_chord_jacks;
				}
			}
		}

		// if we used the normal actual_jack for CJ too we're saying something
		// like "chordjacks" are harder if they share more columns from chord to
		// chord" which is not true, it is in fact either irrelevant or the
		// inverse depending on the scenario, this is merely to catch stuff like
		// splithand jumptrills registering as chordjacks when they shouldn't be
		if (twas_jack) {
			++mitvi.actual_jacks_cj;
		}
	}

	inline void basic_row_sequencing(const metaRowInfo& last,
									 metaItvInfo& mitvi)
	{
		jack_scan(mitvi);
		set_row_variations(mitvi);

		// check if we have a bunch of stuff like [123]4[123] [12]3[124] which
		// isn't actually chordjack, its just broken hs/js, and in fact with the
		// level of leniency that is currently being applied to generic
		// proportions, lots of heavy js/hs is being counted as cj for their 2nd
		// rating, and by a close margin too, we can't just look for [123]4, we
		// need to finish the sequence to be sure i _think_ we only want to do
		// this for single notes, we could abstract it to a more generic pattern
		// template, but let's be restrictive for now
		alternating_chordstream =
		  is_alternating_chord_stream(notes, last_notes, last.last_notes);
		if (alternating_chordstream) {
			if (dbg_lv2) {
				std::cout << "good hot js/hs !!!!: " << std::endl;
			}
			++mitvi.definitely_not_jacks;
		}

		if (alternating_chordstream) {
			// put mixed density stuff here later
		}

		// only cares about single vs chord, not jacks
		alternating_chord_single =
		  is_alternating_chord_single(count, last.count);
		if (alternating_chord_single) {
			if (!twas_jack) {
				if (dbg_lv2) {
					std::cout << "good hot js/hs: " << std::endl;
				}
				mitvi.seriously_not_js -= 3;
			}
		}

		if (last.count == 1 && count == 1) {
			mitvi.seriously_not_js = max(mitvi.seriously_not_js, 0);
			++mitvi.seriously_not_js;
			if (dbg_lv2) {
				std::cout << "consecutive single note: "
						  << mitvi.seriously_not_js << std::endl;
			}

			// light js really stops at [12]321[23] kind of
			// density, anything below that should be picked up
			// by speed, and this stop rolls between jumps
			// getting floated up too high
			if (mitvi.seriously_not_js > 3) {
				if (dbg) {
					std::cout << "exceeding light js/hs tolerance: "
							  << mitvi.seriously_not_js << std::endl;
				}
				mitvi.not_js += mitvi.seriously_not_js;
				// give light hs the light js treatment
				mitvi.not_hs += mitvi.seriously_not_js;
			}
		} else if (last.count > 1 && count > 1) {
			// suppress jumptrilly garbage a little bit
			if (dbg) {
				std::cout << "sequential chords detected: " << std::endl;
			}
			mitvi.not_hs += count;
			mitvi.not_js += count;

			// might be overkill
			if ((notes & last_notes) == 0) {
				if (dbg) {
					std::cout << "bruh they aint even jacks: " << std::endl;
				}
				++mitvi.not_hs;
				++mitvi.not_js;
			} else {
				gluts_maybe = true;
			}
		}

		if ((notes & last_notes) == 0 && count > 1 && last_count > 1) {
			if ((last_notes & last.last_notes) == 0 && last_count > 1) {
				mitvi.dunk_it = true;
			}
		}
	}

	inline void operator()(const metaRowInfo& last,
						   metaItvInfo& mitvi,
						   const float& row_time,
						   const int& row_count,
						   const unsigned& row_notes)
	{
		time = row_time;
		last_last_count = last.last_count;
		last_count = last.count;
		count = row_count;

		last_last_notes = last.last_notes;
		last_notes = last.notes;
		notes = row_notes;

		ms_now = ms_from(time, last.time);

		mitvi._itvi.update_tap_counts(count);
		basic_row_sequencing(last, mitvi);
	}
};
// accumulates hand specific info across an interval as it's processed by row
struct ItvHandInfo
{
	inline void set_col_taps(const col_type& col)
	{
		// this could be more efficient but at least it's clear (ish)?
		switch (col) {
			case col_left:
			case col_right:
				++_col_taps[col];
				break;
			case col_ohjump:
				++_col_taps[col_left];
				++_col_taps[col_right];
				_col_taps[col] += 2;
				break;
			default:
				assert(0);
				break;
		}
	}

	// handle end of interval behavior here
	inline void interval_end()
	{
		// update interval mw for hand taps
		_mw_hand_taps(_col_taps[col_left] + _col_taps[col_right]);

		// update interval mws for col taps
		for (auto& ct : ct_loop) {
			_mw_col_taps[ct](_col_taps[ct]);
		}

		// reset taps per col on this hand
		for (auto& t : _col_taps) {
			t = 0;
		}

		// reset offhand taps
		_offhand_taps = 0;
	}

	// zeroes out all values for everything, complete reset for when we swap
	// hands maybe move to constructor and reconstruct when swapping hands??
	inline void zero()
	{
		for (auto& v : _col_taps) {
			v = 0;
		}

		_offhand_taps = 0;

		for (auto& mw : _mw_col_taps) {
			mw.zero();
		}
		_mw_hand_taps.zero();
	}

	/* access functions for col tap counts */
	[[nodiscard]] inline auto get_col_taps_nowi(const col_type& ct) const -> int
	{
		assert(ct < num_col_types);
		return _mw_col_taps[ct].get_now();
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] inline auto get_col_taps_nowf(const col_type& ct) const
	  -> float
	{
		assert(ct < num_col_types);
		return static_cast<float>(_mw_col_taps[ct].get_now());
	}

	[[nodiscard]] inline auto get_col_taps_windowi(const col_type& ct,
												   const int& window) const
	  -> int
	{
		assert(ct < num_col_types && window < max_moving_window_size);
		return _mw_col_taps[ct].get_total_for_window(window);
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] inline auto get_col_taps_windowf(const col_type& ct,
												   const int& window) const
	  -> float
	{
		assert(ct < num_col_types && window < max_moving_window_size);
		return static_cast<float>(
		  _mw_col_taps[ct].get_total_for_window(window));
	}

	// col operations
	[[nodiscard]] inline auto cols_equal_now() const -> bool
	{
		return get_col_taps_nowi(col_left) == get_col_taps_nowi(col_right);
	}

	[[nodiscard]] inline auto cols_equal_window(const int& window) const -> bool
	{
		return get_col_taps_windowi(col_left, window) ==
			   get_col_taps_windowi(col_right, window);
	}

	[[nodiscard]] inline auto get_col_prop_high_by_low() const -> float
	{
		return div_high_by_low(get_col_taps_nowf(col_left),
							   get_col_taps_nowf(col_right));
	}

	[[nodiscard]] inline auto get_col_prop_low_by_high() const -> float
	{
		return div_low_by_high(get_col_taps_nowf(col_left),
							   get_col_taps_nowf(col_right));
	}

	[[nodiscard]] inline auto get_col_prop_high_by_low_window(
	  const int& window) const -> float
	{
		return div_high_by_low(get_col_taps_windowf(col_left, window),
							   get_col_taps_windowf(col_right, window));
	}

	[[nodiscard]] inline auto get_col_prop_low_by_high_window(
	  const int& window) const -> float
	{
		return div_low_by_high(get_col_taps_windowf(col_left, window),
							   get_col_taps_windowf(col_right, window));
	}

	[[nodiscard]] inline auto get_col_diff_high_by_low() const -> int
	{
		return diff_high_by_low(get_col_taps_nowi(col_left),
								get_col_taps_nowi(col_right));
	}

	[[nodiscard]] inline auto get_col_diff_high_by_low_window(
	  const int& window) const -> int
	{
		return diff_high_by_low(get_col_taps_windowi(col_left, window),
								get_col_taps_windowi(col_right, window));
	}

	/* access functions for hand tap counts */

	[[nodiscard]] inline auto get_taps_nowi() const -> int
	{
		return _mw_hand_taps.get_now();
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] inline auto get_taps_nowf() const -> float
	{
		return static_cast<float>(_mw_hand_taps.get_now());
	}

	[[nodiscard]] inline auto get_taps_windowi(const int& window) const -> int
	{
		assert(window < max_moving_window_size);
		return _mw_hand_taps.get_total_for_window(window);
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] inline auto get_taps_windowf(const int& window) const -> float
	{
		assert(window < max_moving_window_size);
		return static_cast<float>(_mw_hand_taps.get_total_for_window(window));
	}

	// uhh we uhh.. something sets this i think... this is not handled well
  public:
	int _offhand_taps = 0;

  protected:
	int _col_taps[num_col_types] = { 0, 0, 0 };

	// switch to keeping generic moving windows here, if any mod needs a moving
	// window query for anything here, we've already saved computation. any mod
	// that needs custom moving windows based on sequencing will have to keep
	// its own container, but otherwise these should be referenced
	CalcWindow<int> _mw_col_taps[num_col_types];
	CalcWindow<int> _mw_hand_taps;
};

// this _may_ prove to be overkill
struct metaItvHandInfo
{
	ItvHandInfo _itvhi;

	// handle end of interval
	inline void interval_end()
	{
		for (auto& v : _cc_types) {
			v = 0;
		}
		for (auto& v : _cc_types) {
			v = 0;
		}

		_itvhi.interval_end();
	}

	// zero everything out for end of hand loop so the trailing values from the
	// left hand don't end up in the start of the right (not that it would make
	// a huge difference, but it might be abusable
	inline void zero()
	{
		for (auto& v : _cc_types) {
			v = 0;
		}
		for (auto& v : _cc_types) {
			v = 0;
		}

		_itvhi.zero();
	}

	int _cc_types[num_cc_types] = { 0, 0, 0, 0, 0, 0 };
	int _meta_types[num_meta_types] = { 0, 0, 0, 0, 0, 0 };
};

// big brain stuff
inline auto
detecc_oht(const cc_type& a, const cc_type& b, const cc_type& c) -> bool
{
	// we are flipping b with invert col so make sure it's left_right or
	// right_left single note, if either of the other two aren't this will fail
	// anyway and it's fine
	if (b != cc_left_right && b != cc_right_left) {
		return false;
	}

	bool loot = a == invert_cc(b);
	bool doot = a == c;
	// this is kind of big brain so if you don't get it that's ok
	return loot && doot;
}

inline auto
detecc_ccacc(const cc_type& a, const cc_type& b) -> bool
{
	if (a != cc_left_right && a != cc_right_left) {
		return false;
	}

	// now we know we have cc_left_right or cc_right_left, so, xy, we are
	// looking for xyyx, meaning last_last would be the inverion of now
	return invert_cc(a) == b;
}

inline auto
detecc_acca(const cc_type& a, const cc_type& b, const cc_type& c) -> bool
{
	// 1122, 2211, etc
	return a == cc_single_single &&
		   (b == cc_left_right || b == cc_right_left) && c == cc_single_single;
}

// WHOMST'D'VE
inline auto
detecc_ccsjjs(const cc_type& a, const cc_type& b, const cc_type& c) -> bool
{
	if (c != cc_left_right && c != cc_right_left) {
		return false;
	}
	return b == cc_single_jump && a == cc_jump_single;
}

// bpm flux float precision etc
static const float anchor_buffer_ms = 10.F;
struct Anchor_Sequencing
{
	col_type _col = col_init;

	// logically speaking we either shouldn't need this, or metahandinfo should
	// be deriving its tc_ms values from here... because tc_ms logic in
	// metanoteinfo at first glance seems not usable for this, which is like,
	// bad, because it should be
	float _temp_ms = 0.F;
	float _max_ms = ms_init;

	// row_time of last note on this col
	float _last = s_init;

	int _len = 0;

	inline auto col_check(const col_type col) -> bool
	{
		return col == _col || col == col_ohjump;
	}

	inline void operator()(const col_type col, const float& now)
	{
		if (col_check(col)) {
			_temp_ms = ms_from(now, _last);

			// break the anchor if the next note is too much slower than the
			// lowest one in the sequence
			if (_temp_ms > _max_ms + anchor_buffer_ms) {
				_len = 1;
				_max_ms = ms_init;
			} else {
				// increase anchor length and set new cutoff point
				++_len;
				_max_ms = _temp_ms;
			}
		}
		_last = now;
	}
};

struct AnchorSequencer
{
	Anchor_Sequencing anch[num_cols_per_hand];
	int max_seen[num_cols_per_hand] = { 0, 0 };

	// track windows of highest anchor per col seen during an interval
	CalcWindow<int> _mw_max[num_cols_per_hand];

	AnchorSequencer()
	{
		anch[col_left]._col = col_left;
		anch[col_right]._col = col_right;
	}

	// aaaaa we should be able to pass it tcms from mhi but maybe mhi is doing
	// it wrong and it should use what this stores as tcms???
	inline void operator()(const col_type col, const float& row_time)
	{
		// update the one
		if (col == col_left || col == col_right) {
			anch[col](col, row_time);

			// set max seen for this col
			max_seen[col] =
			  anch[col]._len > max_seen[col] ? anch[col]._len : max_seen[col];
		} else if (col == col_ohjump) {

			// update both
			for (auto& c : { col_left, col_right }) {
				anch[c](col, row_time);
				max_seen[c] =
				  anch[c]._len > max_seen[c] ? anch[c]._len : max_seen[c];
			}
		}
	}

	// returns max anchor length seen for the requested window
	[[nodiscard]] inline auto get_max_for_window_and_col(
	  const col_type& col,
	  const int& window) const -> int
	{
		assert(col < num_cols_per_hand);
		return _mw_max[col].get_max_for_window(window);
	}

	inline void handle_interval_end()
	{
		for (auto& c : { col_left, col_right }) {
			_mw_max[c](max_seen[c]);
			max_seen[c] = 0;
		}
	}
};

// this should contain most everything needed for the generic pattern mods,
// extremely specific sequencing will take place in separate areas like with
// rm_seuqencing, and widerange scalers should track their own interval queues
// metanoteinfo is generated per row, from current noteinfo and the previous
// metanoteinfo object, each metanoteinfo stores some basic information from the
// last object, allowing us to look back 3-4 rows into the past without having
// to explicitly store more than 2 mni objects, and we can recycle the pointers
// as we generate the info
// metanoteinfo is generated per _hand_, not per note or column. it contains the
// relevant information for determining what the column configuation of each
// hand is for any row, and it contains timestamp arrays for each column, so it
// is unnecessary to generate information per note, even though in some ways it
// might be more convenient or clearer
struct metaHandInfo
{
	// time (s) of the last seen note in each column
	float row_time = s_init;
	unsigned row_notes = 0;

	float col_time[num_cols_per_hand] = { s_init, s_init };
	float col_time_no_jumps[num_cols_per_hand] = { s_init, s_init };

	// col
	col_type col = col_init;
	col_type last_col = col_init;

	// type of cross column hit
	cc_type cc = cc_init;
	cc_type last_cc = cc_init;

	// needed for the BIGGEST BRAIN PLAYS
	cc_type last_last_cc = cc_init;

	// whomst've
	meta_type mt = meta_init;
	meta_type last_mt = meta_init;

	// number of offhand taps before this row
	int offhand_taps = 0;
	int offhand_ohjumps = 0;

	// ms from last cross column note
	float cc_ms_any = ms_init;

	// ms from last cross column note, ignoring any oh jumps
	float cc_ms_no_jumps = ms_init;

	// ms from last note in this column
	float tc_ms = ms_init;

	inline void full_reset()
	{
		row_time = s_init;
		row_notes;

		for (auto& v : col_time) {
			v = s_init;
		}
		for (auto& v : col_time_no_jumps) {
			v = s_init;
		}

		col = col_init;
		last_col = col_init;

		cc = cc_init;
		last_cc = cc_init;
		last_last_cc = cc_init;

		mt = meta_init;
		last_mt = meta_init;

		offhand_taps = 0;
		offhand_ohjumps = 0;

		cc_ms_any = ms_init;
		cc_ms_no_jumps = ms_init;
		tc_ms = ms_init;
	}

	inline void update_col_times(const float& val)
	{
		// update both
		if (col == col_ohjump) {
			col_time[col_left] = val;
			col_time[col_right] = val;
			return;
		}
		col_time[col] = val;
		col_time_no_jumps[col] = val;
	}

	// sets time from last note in the same column, and last note in the
	// opposite column, handling for jumps is not completely fleshed out yet
	// maybe, i think any case specific handling of their timings can be done
	// with the information already given
	inline void set_timings(const float last[], const float last_no_jumps[])
	{
		switch (cc) {
			case cc_init:
			case cc_left_right:
			case cc_right_left:
			case cc_single_single:
			case cc_jump_single:
				// either we know the end col so we know the start col, or the
				// start col doesn't matter
				cc_ms_any = ms_from(col_time[col], last[invert_col(col)]);
				cc_ms_no_jumps =
				  ms_from(col_time[col], last_no_jumps[invert_col(col)]);

				// technically doesn't matter if we use last_col to index, if
				// it's single -> single we know it's an anchor so it's more
				// intuitive to use col twice
				tc_ms = ms_from(col_time[col], last[col]);
				break;
			case cc_single_jump:
				// tracking this for now, use the higher value of the array
				// (lower ms time, i.e. the column closest to this jump)
				if (last[col_left] < last[col_right]) {
					cc_ms_any = ms_from(col_time[col_left], last[col_right]);
				} else {
					cc_ms_any = ms_from(col_time[col_right], last[col_left]);
				}

				// make sure this doesn't make sense
				cc_ms_no_jumps = ms_init;

				// logically the same as cc_ms_any in 1[12] 1 is the anchor
				// timing with 1 and also the cross column timing with 2
				tc_ms = cc_ms_any;
				break;
			case cc_jump_jump:
				cc_ms_any = ms_init;
				// make sure this doesn't make sense
				cc_ms_no_jumps = ms_init;

				// indexes don't matter-- except that we can't use col or
				// last_col (because index 2 is outside array size)
				tc_ms = ms_from(col_time[0], last[0]);
				break;
			default:
				assert(0);
				break;
		}
	}

	inline auto determine_cc_type(const col_type& last) -> cc_type
	{
		if (last == col_init) {
			return cc_init;
		}

		bool single_tap = is_col_type_single_tap(col);
		if (last == col_ohjump) {
			if (single_tap) {
				return cc_jump_single;
			}
			{ // can't be anything else
				return cc_jump_jump;
			}
		} else if (!single_tap) {
			return cc_single_jump;
			// if we are on left col _now_, we are right to left
		} else if (col == col_left && last == col_right) {
			return cc_right_left;
		} else if (col == col_right && last == col_left) {
			return cc_left_right;
		} else if (col == last) {
			// anchor/jack
			return cc_single_single;
		}

		// makes no logical sense
		assert(0);
		return cc_init;
	}

	inline void set_cc_type() { cc = determine_cc_type(last_col); }

	inline auto big_brain_sequencing(const metaHandInfo& last) -> meta_type
	{
		if (detecc_oht(cc, last_cc, last.last_cc)) {
			return meta_oht;
		}
		if (detecc_ccacc(cc, last.last_cc)) {
			return meta_ccacc;
		}
		if (detecc_acca(cc, last_cc, last.last_cc)) {
			return meta_acca;
		}

		if (cc == cc_left_right || cc == cc_right_left) {
			if (detecc_ccsjjs(last_cc, last_last_cc, last.last_last_cc)) {
				if (cc == last.last_last_cc) {
					return meta_ccsjjscc;
				}
				{
					return meta_ccsjjscc_inverted;
				}
			}
		}
		if (last.mt == meta_enigma) {
			return meta_meta_enigma;
		}
		return meta_enigma;
	}

	inline void operator()(const metaHandInfo& last,
						   CalcWindow<float>& _mw_cc_ms_any,
						   const float& now,
						   const col_type& ct,
						   const unsigned& notes)
	{
		col = ct;
		row_time = now;
		row_notes = notes;
		last_col = last.col;
		last_last_cc = last.last_cc;
		last_cc = last.cc;
		last_mt = last.mt;
		col_time[col_left] = last.col_time[col_left];
		col_time[col_right] = last.col_time[col_right];
		col_time_no_jumps[col_left] = last.col_time_no_jumps[col_left];
		col_time_no_jumps[col_right] = last.col_time_no_jumps[col_right];

		// update this hand's cc type for this row
		set_cc_type();

		// now that we have determined cc_type, we can look for more complex
		// patterns
		mt = big_brain_sequencing(last);

		// every note has at least 2 ms values associated with it, the
		// ms value from the last cross column note (on the same hand),
		// and the ms value from the last note on it's/this column both
		// are useful for different things, and we want to track both.
		// for ohjumps, we will track the ms from the last non-jump on
		// either finger, there are situations where we may want to
		// consider jumps as having a cross column ms value of 0 with
		// itself, not sure if they should be set to this or left at the
		// init values of 5000 though

		// we will need to update time for one or both cols
		update_col_times(now);
		set_timings(last.col_time, last.col_time_no_jumps);

		// keep track of these ms values here so we aren't doing it in
		// potentially 5 different pattern mods
		_mw_cc_ms_any(cc_ms_any);
	}
};

// pmod stuff that was being mega coopy poostered
inline void
mod_set(const CalcPatternMod& pmod,
		vector<float> doot[],
		const int& i,
		const float& mod)
{
	doot[pmod][i] = mod;
}

inline void
neutral_set(const CalcPatternMod& pmod, vector<float> doot[], const int& i)
{
	doot[pmod][i] = neutral;
}

inline void
dbg_neutral_set(const vector<CalcPatternMod>& dbg,
				vector<float> doot[],
				const int& i)
{
	if (debug_lmao) {
		for (auto& pmod : dbg) {
			doot[pmod][i] = neutral;
		}
	}
}

// since the calc skillset balance now operates on +- rather than
// just - and then normalization, we will use this to depress the
// stream rating for non-stream files.
struct StreamMod
{
	const CalcPatternMod _pmod = Stream;
	const std::string name = "StreamMod";
	const int _tap_size = single;

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.0F;
	float prop_buffer = 1.F;
	float prop_scaler = 1.428F; // ~10/7

	float jack_pool = 4.F;
	float jack_comp_min = 0.5F;
	float jack_comp_max = 1.F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "prop_buffer", &prop_buffer },
		{ "prop_scaler", &prop_scaler },

		{ "jack_pool", &jack_pool },
		{ "jack_comp_min", &jack_comp_min },
		{ "jack_comp_max", &jack_comp_max },
	};
#pragma endregion params and param map
	float prop_component = 0.F;
	float jack_component = 0.F;
	float pmod = min_mod;

#pragma region generic functions
	inline void setup(vector<float> doot[], const int& i)
	{
		doot[_pmod].resize(i);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline auto handle_case_optimizations(const ItvInfo& itvi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// 1 tap is by definition a single tap
		if (itvi.total_taps < 2) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		if (itvi.taps_by_size[_tap_size] == 0) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}

		return false;
	}

	inline void operator()(const metaItvInfo& mitvi, vector<float> doot[])
	{
		const auto& itvi = mitvi._itvi;
		if (handle_case_optimizations(itvi, doot, mitvi._idx)) {
			return;
		}

		// we want very light js to register as stream,
		// something like jumps on every other 4th, so 17/19
		// ratio should return full points, but maybe we should
		// allow for some leeway in bad interval slicing this
		// maybe doesn't need to be so severe, on the other
		// hand, maybe it doesn'ting need to be not needing'nt
		// to be so severe

		// we could make better use of sequencing here since now it's easy

		prop_component =
		  static_cast<float>(itvi.taps_by_size[_tap_size] + prop_buffer) /
		  static_cast<float>(itvi.total_taps - prop_buffer) * prop_scaler;

		// allow for a mini/triple jack in streams.. but not more than that
		jack_component = CalcClamp(
		  jack_pool - mitvi.actual_jacks, jack_comp_min, jack_comp_max);
		pmod = fastsqrt(prop_component * jack_component);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		// actual mod
		doot[_pmod][mitvi._idx] = pmod;
	}
};
struct JSMod
{
	const CalcPatternMod _pmod = JS;
	const vector<CalcPatternMod> _dbg = { JSS, JSJ };
	const std::string name = "JSMod";
	const int _tap_size = jump;

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.1F;
	float mod_base = 0.F;
	float prop_buffer = 1.F;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 2.714F; // ~19/7

	float split_hand_pool = 1.45F;
	float split_hand_min = 0.85F;
	float split_hand_max = 1.F;
	float split_hand_scaler = 1.F;

	float jack_pool = 1.35F;
	float jack_min = 0.5F;
	float jack_max = 1.F;
	float jack_scaler = 1.F;

	float decay_factor = 0.05F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },
		{ "prop_buffer", &prop_buffer },

		{ "total_prop_scaler", &total_prop_scaler },
		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },

		{ "split_hand_pool", &split_hand_pool },
		{ "split_hand_min", &split_hand_min },
		{ "split_hand_max", &split_hand_max },
		{ "split_hand_scaler", &split_hand_scaler },

		{ "jack_pool", &jack_pool },
		{ "jack_min", &jack_min },
		{ "jack_max", &jack_max },
		{ "jack_scaler", &jack_scaler },

		{ "decay_factor", &decay_factor },
	};
#pragma endregion params and param map
	float total_prop = 0.F;
	float jumptrill_prop = 0.F;
	float jack_prop = 0.F;
	float last_mod = min_mod;
	float pmod = min_mod;
	float t_taps = 0.F;
#pragma region generic functions
	inline void setup(vector<float> doot[], const int& i)
	{
		doot[_pmod].resize(i);

		if (debug_lmao) {
			for (auto& mod : _dbg) {
				doot[mod].resize(i);
			}
		}
	}

	inline void decay_mod()
	{
		pmod = CalcClamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void set_dbg(vector<float> doot[], const int& i)
	{
		if (debug_lmao) {
			doot[JSS][i] = jumptrill_prop;
			doot[JSJ][i] = jack_prop;
		}
	}

	inline auto handle_case_optimizations(const ItvInfo& itvi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// empty interval, don't decay js mod or update last_mod
		if (itvi.total_taps == 0) {
			neutral_set(_pmod, doot, i);
			dbg_neutral_set(_dbg, doot, i);
			return true;
		}

		// at least 1 tap but no jumps
		if (itvi.taps_by_size[_tap_size] == 0) {
			decay_mod();
			doot[_pmod][i] = pmod;
			dbg_neutral_set(_dbg, doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const metaItvInfo& mitvi, vector<float> doot[])
	{
		const auto& itvi = mitvi._itvi;
		if (handle_case_optimizations(itvi, doot, mitvi._idx)) {
			return;
		}

		t_taps = static_cast<float>(itvi.total_taps);

		// creepy banana
		total_prop =
		  static_cast<float>(itvi.taps_by_size[_tap_size] + prop_buffer) /
		  (t_taps - prop_buffer) * total_prop_scaler;
		total_prop =
		  CalcClamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// punish lots splithand jumptrills
		// uhh this might also catch oh jumptrills can't remember
		jumptrill_prop = CalcClamp(
		  split_hand_pool - (static_cast<float>(mitvi.not_js) / t_taps),
		  split_hand_min,
		  split_hand_max);

		// downscale by jack density rather than upscale like cj
		// theoretically the ohjump downscaler should handle
		// this but handling it here gives us more flexbility
		// with the ohjump mod
		jack_prop = CalcClamp(
		  jack_pool - (static_cast<float>(mitvi.actual_jacks) / t_taps),
		  jack_min,
		  jack_max);

		pmod =
		  CalcClamp(total_prop * jumptrill_prop * jack_prop, min_mod, max_mod);
		if (mitvi.dunk_it) {
			pmod *= 0.99F;
		}
		doot[_pmod][mitvi._idx] = pmod;
		set_dbg(doot, mitvi._idx);

		// set last mod, we're using it to create a decaying mod that won't
		// result in extreme spikiness if files alternate between js and
		// hs/stream
		last_mod = pmod;
	}
};
struct HSMod
{
	const CalcPatternMod _pmod = HS;
	const vector<CalcPatternMod> _dbg = { HSS, HSJ };
	const std::string name = "HSMod";
	const int _tap_size = hand;

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.1F;
	float mod_base = 0.4F;
	float prop_buffer = 1.F;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 5.571F; // ~32/7
	float total_prop_base = 0.4F;

	float split_hand_pool = 1.45F;
	float split_hand_min = 0.89F;
	float split_hand_max = 1.F;
	float split_hand_scaler = 1.F;

	float jack_pool = 1.35F;
	float jack_min = 0.5F;
	float jack_max = 1.F;
	float jack_scaler = 1.F;

	float decay_factor = 0.05F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },
		{ "prop_buffer", &prop_buffer },

		{ "total_prop_scaler", &total_prop_scaler },
		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },
		{ "total_prop_base", &total_prop_base },

		{ "split_hand_pool", &split_hand_pool },
		{ "split_hand_min", &split_hand_min },
		{ "split_hand_max", &split_hand_max },
		{ "split_hand_scaler", &split_hand_scaler },

		{ "jack_pool", &jack_pool },
		{ "jack_min", &jack_min },
		{ "jack_max", &jack_max },
		{ "jack_scaler", &jack_scaler },

		{ "decay_factor", &decay_factor },
	};
#pragma endregion params and param map
	float total_prop = 0.F;
	float jumptrill_prop = 0.F;
	float jack_prop = 0.F;
	float last_mod = min_mod;
	float pmod = min_mod;
	float t_taps = 0.F;
#pragma region generic functions
	inline void setup(vector<float> doot[], const int& i)
	{
		doot[_pmod].resize(i);

		if (debug_lmao) {
			for (auto& mod : _dbg) {
				doot[mod].resize(i);
			}
		}
	}

	inline void decay_mod()
	{
		pmod = CalcClamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void set_dbg(vector<float> doot[], const int& i)
	{
		if (debug_lmao) {
			doot[HSS][i] = jumptrill_prop;
			doot[HSJ][i] = jack_prop;
		}
	}

	inline auto handle_case_optimizations(const ItvInfo& itvi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// empty interval, don't decay mod or update last_mod
		if (itvi.total_taps == 0) {
			neutral_set(_pmod, doot, i);
			dbg_neutral_set(_dbg, doot, i);
			return true;
		}

		// look ma no hands
		if (itvi.taps_by_size[_tap_size] == 0) {
			decay_mod();
			doot[_pmod][i] = pmod;
			dbg_neutral_set(_dbg, doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const metaItvInfo& mitvi, vector<float> doot[])
	{
		const auto& itvi = mitvi._itvi;
		if (handle_case_optimizations(itvi, doot, mitvi._idx)) {
			return;
		}

		t_taps = static_cast<float>(itvi.total_taps);

		// when bark of dog into canyon scream at you
		total_prop = total_prop_base +
					 (static_cast<float>((itvi.taps_by_size[_tap_size] +
										  itvi.mixed_hs_density_tap_bonus) +
										 prop_buffer) /
					  (t_taps - prop_buffer) * total_prop_scaler);
		total_prop =
		  CalcClamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// downscale jumptrills for hs as well
		jumptrill_prop = CalcClamp(
		  split_hand_pool - (static_cast<float>(mitvi.not_hs) / t_taps),
		  split_hand_min,
		  split_hand_max);

		// downscale by jack density rather than upscale, like cj does
		jack_prop = CalcClamp(
		  jack_pool - (static_cast<float>(mitvi.actual_jacks) / t_taps),
		  jack_min,
		  jack_max);

		pmod =
		  CalcClamp(total_prop * jumptrill_prop * jack_prop, min_mod, max_mod);

		if (mitvi.dunk_it) {
			pmod *= 0.99F;
		}

		doot[_pmod][mitvi._idx] = pmod;
		set_dbg(doot, mitvi._idx);

		// set last mod, we're using it to create a decaying mod that won't
		// result in extreme spikiness if files alternate between js and
		// hs/stream
		last_mod = pmod;
	}
};
struct CJMod
{
	const CalcPatternMod _pmod = CJ;
	const vector<CalcPatternMod> _dbg = { CJS, CJJ };
	const std::string name = "CJMod";

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.1F;
	float mod_base = 0.4F;
	float prop_buffer = 1.F;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 5.428F; // ~38/7

	float jack_base = 2.F;
	float jack_min = 0.625F;
	float jack_max = 1.F;
	float jack_scaler = 1.F;

	float not_jack_pool = 1.2F;
	float not_jack_min = 0.4F;
	float not_jack_max = 1.F;
	float not_jack_scaler = 1.F;

	float vibro_flag = 1.F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },
		{ "prop_buffer", &prop_buffer },

		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },
		{ "total_prop_scaler", &total_prop_scaler },

		{ "jack_base", &jack_base },
		{ "jack_min", &jack_min },
		{ "jack_max", &jack_max },
		{ "jack_scaler", &jack_scaler },

		{ "not_jack_pool", &not_jack_pool },
		{ "not_jack_min", &not_jack_min },
		{ "not_jack_max", &not_jack_max },
		{ "not_jack_scaler", &not_jack_scaler },

		{ "vibro_flag", &vibro_flag },
	};
#pragma endregion params and param map
	float total_prop = 0.F;
	float jack_prop = 0.F;
	float not_jack_prop = 0.F;
	float pmod = min_mod;
	float t_taps = 0.F;
#pragma region generic functions
	inline void setup(vector<float> doot[], const int& i)
	{
		doot[_pmod].resize(i);

		if (debug_lmao) {
			for (auto& mod : _dbg) {
				doot[mod].resize(i);
			}
		}
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void set_dbg(vector<float> doot[], const int& i)
	{
		if (debug_lmao) {
			doot[CJS][i] = not_jack_prop;
			doot[CJJ][i] = jack_prop;
		}
	}

	inline auto handle_case_optimizations(const ItvInfo& itvi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		if (itvi.total_taps == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		// no chords
		if (itvi.chord_taps == 0) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}
		return false;
	}

	inline void operator()(const metaItvInfo& mitvi, vector<float> doot[])
	{
		const auto& itvi = mitvi._itvi;
		if (handle_case_optimizations(itvi, doot, mitvi._idx)) {
			return;
		}

		t_taps = static_cast<float>(itvi.total_taps);

		// we have at least 1 chord we want to give a little leeway for single
		// taps but not too much or sections of [12]4[123] [123]4[23] will be
		// flagged as chordjack when they're really just broken chordstream, and
		// we also want to give enough leeway so that hyperdense chordjacks at
		// lower bpms aren't automatically rated higher than more sparse jacks
		// at higher bpms
		total_prop = static_cast<float>(itvi.chord_taps + prop_buffer) /
					 (t_taps - prop_buffer) * total_prop_scaler;
		total_prop =
		  CalcClamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// make sure there's at least a couple of jacks
		jack_prop =
		  CalcClamp(mitvi.actual_jacks_cj - jack_base, jack_min, jack_max);

		// explicitly detect broken chordstream type stuff so we can give more
		// leeway to single note jacks brop_two_return_of_brop_electric_bropaloo
		not_jack_prop = CalcClamp(
		  not_jack_pool -
			(static_cast<float>(mitvi.definitely_not_jacks * not_jack_scaler) /
			 t_taps),
		  not_jack_min,
		  not_jack_max);

		pmod =
		  CalcClamp(total_prop * jack_prop * not_jack_prop, min_mod, max_mod);

		// ITS JUST VIBRO THEN(unique note permutations per interval < 3 ), use
		// this other places ?
		if (mitvi.basically_vibro) {
			// we shouldn't be hitting empty intervals here
			assert(mitvi.num_var > 0);
			if (mitvi.num_var == 1) {
				pmod *= 0.5F * vibro_flag;
			} else if (mitvi.num_var == 2) {
				pmod *= 0.9F * vibro_flag;
			} else if (mitvi.num_var == 3) {
				pmod *= 0.95F * vibro_flag;
			}
			assert(mitvi.num_var < 4);
		}

		doot[_pmod][mitvi._idx] = pmod;
		set_dbg(doot, mitvi._idx);
	}
};

// i actually have no idea why i made this a separate mod instead of a prop
// scaler like everything else but there's like a 20% chance there was a good
// reason so i'll leave it

// ok i remember now its because i wanted to smooth the mod before applying to
// cj
struct CJQuadMod
{
	const CalcPatternMod _pmod = CJQuad;
	// const vector<CalcPatternMod> _dbg = { CJS, CJJ };
	const std::string name = "CJQuadMod";
	const int _tap_size = quad;

#pragma region params
	
	float min_mod = 0.9F;
	float max_mod = 1.3F;
	float base = 0.1F;

	float jump_scaler = 1.F;
	float hand_scaler = 1.33F;
	float quad_scaler = 2.F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "jump_scaler", &jump_scaler },
		{ "hand_scaler", &hand_scaler },
		{ "quad_scaler", &quad_scaler },
	};
#pragma endregion params and param map
	float pmod = min_mod;
#pragma region generic functions
	inline void setup(vector<float> doot[], const int& i)
	{
		doot[_pmod].resize(i);

		//		if (debug_lmao)
		//			for (auto& mod : _dbg)
		//				doot[mod].resize(i);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline auto handle_case_optimizations(const ItvInfo& itvi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		if (itvi.total_taps == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const metaItvInfo& mitvi, vector<float> doot[])
	{
		const auto& itvi = mitvi._itvi;
		if (handle_case_optimizations(itvi, doot, mitvi._idx)) {
			return;
		}

		float t_taps = itvi.total_taps;
		float a1 = static_cast<float>(itvi.taps_by_size[jump] * jump_scaler) / t_taps;
		float a2 = static_cast<float>(itvi.taps_by_size[hand] * hand_scaler) / t_taps;
		float a3 = static_cast<float>(itvi.taps_by_size[quad] * quad_scaler) / t_taps;

		float aaa = a1 + a2 + a3;

		pmod = CalcClamp(base + fastsqrt(aaa), min_mod, max_mod);

		doot[_pmod][mitvi._idx] = pmod;
		// set_dbg(doot, mitvi._idx);
	}
};

// note we do want the occasional single ohjump in js to count as a sequence of
// length 1
struct OHJ_Sequencing
{
	// COUNT TAPS IN JUMPS
	int cur_seq_taps = 0;
	int max_seq_taps = 0;

	inline auto get_largest_seq_taps() -> int
	{
		return cur_seq_taps > max_seq_taps ? cur_seq_taps : max_seq_taps;
	}

	inline void complete_seq()
	{
		// negative values should not be possible
		assert(cur_seq_taps >= 0);

		// set the largest ohj sequence
		max_seq_taps = get_largest_seq_taps();
		// reset
		cur_seq_taps = 0;
	}

	inline void zero()
	{
		cur_seq_taps = 0;
		max_seq_taps = 0;
	}

	inline void operator()(const metaHandInfo& now)
	{
		// do nothing for offhand taps
		if (now.col == col_empty) {
			return;
		}

		if (cur_seq_taps == 0) {
			// if we aren't in a sequence and aren't going to start one, bail
			if (now.col != col_ohjump) {
				return;
			}
			{ // allow sequences of 1 by starting any time we hit an ohjump
				cur_seq_taps += 2;
			}
		}

		// we know between the following that the latter is more
		// difficult [12][12][12]222[12][12][12]
		// [12][12][12]212[12][12][12]
		// so we want to penalize not only any break in the ohj
		// sequence but further penalize breaks which contain
		// cross column taps this should also reflect the
		// difference between [12]122[12], [12]121[12] cases
		// like 121[12][12]212[12][12]121 should probably have
		// some penalty but likely won't with this setup, but
		// everyone hates that anyway and it would be quite
		// difficult to force the current setup to do so without
		// increasing complexity significantly (probably)

		// don't reset immediately on a single note, wait to see
		// what comes next, if now.last_cc == cc_jump_single, we have just
		// broken a sequence (technically this can be simply something like
		// [12]2[12]2[12]2 so the ohjumps wouldn't really be a sequence
		switch (now.cc) {
			case cc_jump_jump:
				// continuing sequence
				cur_seq_taps += 2;
				break;
			case cc_jump_single:
				// just came out of a jump seq, do nothing... wait to see what
				// happens
				break;
			case cc_left_right:
			case cc_right_left:
				// we should only get here if we recently broke seq
				assert(now.last_cc == cc_jump_single);

				// if we have an actual cross column tap now, and if we just
				// came from a jump -> single, then we have something like
				// [12]21, which is much harder than [12]22, so penalize the
				// sequence slightly before completing
				if (cur_seq_taps == 2) {
					cur_seq_taps -= 1;
				} else {
					cur_seq_taps -= 3;
				}
				complete_seq();
				break;
			case cc_single_single:
				// we should only get here if we recently broke seq
				assert(now.last_cc == cc_jump_single);

				// we have something like [12]22, complete the sequence
				// without the penalty that the cross column incurs
				complete_seq();
				break;
			case cc_single_jump:
				// [12]1[12]... we broke a sequence and went right back into
				// one.. reset sequence for now but come back to revsit this, we
				// might want to have different behavior, but we'd need to track
				// the columns of the single notes in the chain
				// if (now.last_cc == cc_jump_single)
				//	complete_seq();
				// else
				complete_seq();
				break;
			case cc_init:
				// do nothing, we don't have enough info yet
				break;
			default:
				assert(0);
				break;
		}
	}
};

// ok this should be more manageable now
struct OHJumpModGuyThing
{
	const CalcPatternMod _pmod = OHJumpMod;
	const vector<CalcPatternMod> _dbg = { OHJBaseProp, OHJPropComp, OHJSeqComp,
										  OHJMaxSeq,   OHJCCTaps,   OHJHTaps };
	const std::string name = "OHJumpMod";

#pragma region params

	float min_mod = 0.75F;
	float max_mod = 1.F;

	float max_seq_weight = 0.65F;
	float max_seq_pool = 1.25F;
	float max_seq_scaler = 1.F;

	float prop_pool = 1.4F;
	float prop_scaler = 1.F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "max_seq_weight", &max_seq_weight },
		{ "max_seq_pool", &max_seq_pool },
		{ "max_seq_scaler", &max_seq_scaler },

		{ "prop_pool", &prop_pool },
		{ "prop_scaler", &prop_scaler },
	};
#pragma endregion params and param map

	OHJ_Sequencing ohj;
	int max_ohjump_seq_taps = 0;
	int cc_taps = 0;

	float floatymcfloatface = 0.F;
	// number of jumps scaled to total taps in hand
	float base_seq_prop = 0.F;
	// size of sequence scaled to total taps in hand
	float base_jump_prop = 0.F;

	float max_seq_component = neutral;
	float prop_component = neutral;
	float pmod = neutral;

#pragma region generic functions
	inline void setup(vector<float> doot[], const int& size)
	{
		doot[_pmod].resize(size);
		if (debug_lmao) {
			for (auto& mod : _dbg) {
				doot[mod].resize(size);
			}
		}
	}

	inline void full_reset()
	{
		ohj.zero();

		max_ohjump_seq_taps = 0;
		cc_taps = 0;

		floatymcfloatface = 0.F;
		base_seq_prop = 0.F;
		base_jump_prop = 0.F;

		max_seq_component = neutral;
		prop_component = neutral;
		pmod = neutral;
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const metaHandInfo& now) { ohj(now); }

	// build component based on max sequence relative to hand taps
	inline void set_max_seq_comp()
	{
		max_seq_component = max_seq_pool - (base_seq_prop * max_seq_scaler);
		max_seq_component = max_seq_component < 0.1F ? 0.1F : max_seq_component;
		max_seq_component = fastsqrt(max_seq_component);
	}

	// build component based on number of jumps relative to hand taps
	inline void set_prop_comp()
	{
		prop_component = prop_pool - (base_jump_prop * prop_scaler);
		prop_component = prop_component < 0.1F ? 0.1F : prop_component;
		prop_component = fastsqrt(prop_component);
	}

	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// nothing here or there are no ohjumps
		if (itvhi.get_taps_nowi() == 0 ||
			itvhi.get_col_taps_nowi(col_ohjump) == 0) {
			neutral_set(_pmod, doot, i);
			dbg_neutral_set(_dbg, doot, i);
			return true;
		}

		// everything in the interval is in an ohj sequence
		if (max_ohjump_seq_taps >= itvhi.get_taps_nowi()) {
			mod_set(_pmod, doot, i, min_mod);
			set_debug_output(doot, i);
			return true;
		}

		// if there was a jump, we have at least a seq of 1
		assert(max_ohjump_seq_taps > 0);

		// no repeated oh jumps, prop scale only based on jumps taps in hand
		// taps if the jump was immediately broken by a cross column single tap
		// we can have values of 1, otherwise 2
		if (max_ohjump_seq_taps < 3) {

			// need to set now
			base_jump_prop =
			  itvhi.get_col_taps_nowf(col_ohjump) / itvhi.get_taps_nowf();
			set_prop_comp();

			pmod = CalcClamp(prop_component, min_mod, max_mod);
			doot[OHJumpMod][i] = pmod;
			set_debug_output(doot, i);
			return true;
		}

		// if this is true we have some combination of single notes
		// and jumps where the single notes are all on the same
		// column
		if (cc_taps == 0) {
			// we don't want to treat 2[12][12][12]2222 2222[12][12][12]2
			// differently, so use the max sequence here exclusively
			// shortcut mod calculations, we need the base props now

			// build now
			floatymcfloatface = static_cast<float>(max_ohjump_seq_taps);
			base_seq_prop = floatymcfloatface / itvhi.get_taps_nowf();
			set_max_seq_comp();

			pmod = CalcClamp(max_seq_component, min_mod, max_mod);

			doot[OHJumpMod][i] = pmod;
			set_debug_output(doot, i);
			return true;
		}

		return false;
	}

	inline void set_debug_output(vector<float> doot[], const int& i)
	{
		if (debug_lmao) {
			doot[OHJSeqComp][i] = max_seq_component;
			doot[OHJPropComp][i] = prop_component;
			doot[OHJBaseProp][i] = base_seq_prop;
			doot[OHJMaxSeq][i] = floatymcfloatface;
			doot[OHJCCTaps][i] = static_cast<float>(cc_taps);
		}
	}

	void operator()(const metaItvHandInfo& mitvhi,
					vector<float> doot[],
					const int& i)
	{
		const auto& itvhi = mitvhi._itvhi;
		// normally we only set these if we use them, bring them to 1 to avoid
		// confusion
		if (debug_lmao) {
			max_seq_component = neutral;
			prop_component = neutral;
		}

		cc_taps =
		  mitvhi._cc_types[cc_left_right] + mitvhi._cc_types[cc_right_left];

		assert(cc_taps >= 0);

		// if cur_seq > max when we ended the interval, grab it
		max_ohjump_seq_taps = ohj.cur_seq_taps > ohj.max_seq_taps
								? ohj.cur_seq_taps
								: ohj.max_seq_taps;

		// handle simple cases first, execute this block if nothing easy is
		// detected, fill out non-component debug info and handle interval
		// resets at end
		if (handle_case_optimizations(mitvhi._itvhi, doot, i)) {
			interval_reset();
			return;
		}

		// for js we lean into max sequences more, since they're better
		// indicators of inflated difficulty

		// set either after case optimizations or in case optimizations, after
		// the simple checks, for optimization
		floatymcfloatface = static_cast<float>(max_ohjump_seq_taps);
		base_seq_prop = floatymcfloatface / mitvhi._itvhi.get_taps_nowf();
		set_max_seq_comp();
		max_seq_component = CalcClamp(max_seq_component, 0.1F, max_mod);

		base_jump_prop =
		  itvhi.get_col_taps_nowf(col_ohjump) / itvhi.get_taps_nowf();
		set_prop_comp();
		prop_component = CalcClamp(prop_component, 0.1F, max_mod);

		pmod = weighted_average(
		  max_seq_component, prop_component, max_seq_weight, 1.F);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[OHJumpMod][i] = pmod;
		set_debug_output(doot, i);

		interval_reset();
	}

	inline void interval_reset()
	{
		// reset any interval stuff here
		cc_taps = 0;
		ohj.max_seq_taps = 0;
		max_ohjump_seq_taps = 0;
	}
};

struct BalanceMod
{

	const CalcPatternMod _pmod = Balance;
	const std::string name = "BalanceMod";

#pragma region params

	float min_mod = 0.95F;
	float max_mod = 1.05F;
	float mod_base = 0.325F;
	float buffer = 1.F;
	float scaler = 1.F;
	float other_scaler = 4.F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },   { "max_mod", &max_mod },
		{ "mod_base", &mod_base }, { "buffer", &buffer },
		{ "scaler", &scaler },	 { "other_scaler", &other_scaler },
	};
#pragma endregion params and param map

	float pmod = neutral;

#pragma region generic functions

	inline void full_reset() { pmod = neutral; }

	inline void setup(vector<float> doot[], const int& size)
	{
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		// same number of taps on each column
		if (itvhi.cols_equal_now()) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}

		// probably should NOT do this but leaving enabled for now so i can
		// verify structural changes dont change output diff
		// jack, dunno if this is worth bothering about? it would only matter
		// for tech and it may matter too much there? idk
		if (itvhi.get_col_taps_nowi(col_left) == 0 ||
			itvhi.get_col_taps_nowi(col_right) == 0) {
			mod_set(_pmod, doot, i, max_mod);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo& itvhi,
						   vector<float> doot[],
						   const int& i)
	{
		if (handle_case_optimizations(itvhi, doot, i)) {
			return;
		}

		pmod = itvhi.get_col_prop_low_by_high();
		pmod = (mod_base + (buffer + (scaler / pmod)) / other_scaler);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[_pmod][i] = pmod;
	}
};

struct RollMod
{
	const CalcPatternMod _pmod = Roll;
	const std::string name = "RollMod";

#pragma region params
	float min_mod = 0.5F;
	float max_mod = 1.0F;
	float pool = 1.25F;
	float base = 0.15F;

	float cv_reset = 0.5F;
	float cv_threshhold = 0.25F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },   { "max_mod", &max_mod },
		{ "pool", &pool },		   { "base", &base },
		{ "cv_reset", &cv_reset }, { "cv_threshhold", &cv_threshhold },
	};
#pragma endregion params and param map

	float pmod = neutral;

	vector<float> seq_ms = { 0.F, 0.F, 0.F };
	// uhhh lazy way out of tracking all the floats i think
	float moving_cv = cv_reset;

#pragma region generic functions

	inline void full_reset()
	{
		for (auto& v : seq_ms) {
			v = 0.F;
		}
		pmod = neutral;
		moving_cv = cv_reset;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	static inline void advance_sequencing(const metaHandInfo& now) {}

	static inline auto handle_case_optimizations(vector<float>  /*doot*/[],
												 const int&  /*i*/) -> bool
	{
		return false;
	}

	inline void operator()(const metaItvHandInfo&  /*mitvhi*/,
						   vector<float> doot[],
						   const int& i)
	{

		doot[_pmod][i] = 1.F;

		interval_reset();
	}

	// may be unneeded for this function but it's probably good practice to have
	// this and always reset anything that needs to be on handling case
	// optimizations, even if the case optimizations don't require us to reset
	// anything
	static inline void interval_reset() {}
};

static const int max_trills_per_interval = 4;
// almost identical to wrr, refer to comments there
struct OHTrillMod
{
	const CalcPatternMod _pmod = OHTrill;
	const std::string name = "OHTrillMod";

#pragma region params

	float window_param = 3.F;

	float min_mod = 0.5F;
	float max_mod = 1.F;
	float base = 1.35F;
	float suppression = 0.4F;

	float cv_reset = 1.F;
	float cv_threshhold = 0.45F;

	// this is for base trill 1->2 2->1 1->2, 4 notes, 3 timings, however we can
	// extend the window for ms values such that, for example, we require 2 oht
	// meta detections, and on the third, we check a window of 5 ms values,
	// dunno what the benefits or drawbacks are of either system atm but they
	// are both implementable easily
	float oht_cc_window = 6.F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "cv_reset", &cv_reset },
		{ "cv_threshhold", &cv_threshhold },

		{ "oht_cc_window", &oht_cc_window },
	};
#pragma endregion params and param map

	int window = 0;
	int cc_window = 0;

	bool luca_turilli = false;

	// ok new plan, ohj, wrjt and wrr are relatively well tuned so i'll try this
	// here, handle merging multiple sequences in a single interval into one
	// value at interval end and keep a window of that. suppose we have two
	// intervals of 12 notes with 8 in trill formation, one has an 8 note trill
	// and the other has two 4 note trills at the start/end, we want to punish
	// the 8 note trill harder, this means we _will_ be resetting the
	// consecutive trill counter every interval, but will not be resetting the
	// trilling flag, this way we don't have to futz around with awkward
	// proportion math, similar to thing 1 and thing 2
	CalcWindow<float> badjuju;

	CalcWindow<int> _mw_oht_taps;

	int foundyatrills[max_trills_per_interval] = { 0, 0, 0, 0 };

	int found_oht = 0;
	int oht_len = 0;
	int oht_taps = 0;

	float hello_my_name_is_goat = 0.F;

	float moving_cv = cv_reset;
	float pmod = min_mod;

#pragma region generic functions

	inline void full_reset()
	{
		badjuju.zero();

		luca_turilli = false;
		found_oht = 0;
		oht_len = 0;

		for (auto& v : foundyatrills) {
			v = 0;
		}

		moving_cv = cv_reset;
		pmod = neutral;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		cc_window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline auto make_thing(const float& itv_taps) -> float
	{
		hello_my_name_is_goat = 0.F;

		if (found_oht == 0) {
			return 0.F;
		}

		for (auto& v : foundyatrills) {
			if (v == 0) {
				continue;
			}

			// water down smaller sequences
			hello_my_name_is_goat =
			  (static_cast<float>(v) / itv_taps) - suppression;
		}
		return CalcClamp(hello_my_name_is_goat, 0.1F, 1.F);
	}

	inline void complete_seq()
	{
		if (!luca_turilli || oht_len == 0) {
			return;
		}

		luca_turilli = false;
		foundyatrills[found_oht] = oht_len;
		oht_len = 0;
		++found_oht;
		moving_cv = (moving_cv + cv_reset) / 2.F;
	}

	inline auto oht_timing_check(const CalcWindow<float>& cc_ms_any) -> bool
	{
		moving_cv = (moving_cv + cc_ms_any.get_cv_of_window(cc_window)) / 2.F;
		// the primary difference from wrr, just check cv on the base ms values,
		// we are looking for values that are all close together without any
		// manipulation
		return moving_cv < cv_threshhold;
	}

	inline void wifflewaffle()
	{
		if (luca_turilli) {
			++oht_len;
			++oht_taps;
		} else {
			luca_turilli = true;
			oht_len += 3;
			oht_taps += 3;
		}
	}

	inline void advance_sequencing(const metaHandInfo& now,
								   const CalcWindow<float>& cc_ms_any)
	{

		switch (now.mt) {
			case meta_oht:
				if (oht_timing_check(cc_ms_any)) {
					wifflewaffle();
				} else {
					complete_seq();
				}
				break;
			case meta_ccacc:
				// wait to see what happens
				break;
			case meta_enigma:
			case meta_meta_enigma:
				// also wait to see what happens, but not if last was ccacc,
				// since we only don't complete there if we don't immediately go
				// back into ohts

				// this seems to be overkill with how lose the detection is
				// already anyway

				//if (now.last_cc == meta_ccacc) {
				//	complete_seq();
				//}
				//break;
			default:
				complete_seq();
				break;
		}
	}

	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// no taps, no trills
		if (itvhi.get_taps_windowi(window) == 0 ||
			_mw_oht_taps.get_total_for_window(window) == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		// full oht
		if (itvhi.get_taps_windowi(window) ==
			_mw_oht_taps.get_total_for_window(window)) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo& itvhi,
						   vector<float> doot[],
						   const int& i)
	{
		if (oht_len > 0) {
			foundyatrills[found_oht] = oht_len;
			++found_oht;
		}

		_mw_oht_taps(oht_taps);
		if (handle_case_optimizations(itvhi, doot, i)) {
			interval_end();
			return;
		}

		badjuju(make_thing(itvhi.get_taps_nowf()));

		pmod = base - badjuju.get_mean_of_window(window);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[_pmod][i] = pmod;

		interval_end();
	}

	inline void interval_end()
	{
		for (auto& v : foundyatrills) {
			v = 0;
		}

		found_oht = 0;
		oht_len = 0;
		oht_taps = 0;
	}
};

// slightly different implementation of the old chaos mod, basically picks up
// polyishness and tries to detect awkward transitions
struct ChaosMod
{
	const CalcPatternMod _pmod = Chaos;
	const std::string name = "ChaosMod";

#pragma region params

	float min_mod = 0.95F;
	float max_mod = 1.05F;
	float base = -0.1F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
	};
#pragma endregion params and param map

	// don't allow this to be a modifiable param
	const int window = 6;

	CalcWindow<float> _u;
	CalcWindow<float> _wot;

	float pmod = neutral;

#pragma region generic functions

	inline void full_reset()
	{
		_u.zero();
		_wot.zero();
		pmod = neutral;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const CalcWindow<float>& _mw_cc_ms_any)
	{
		// most recent value
		float a = _mw_cc_ms_any.get_now();

		// previous value
		float b = _mw_cc_ms_any.get_last();

		if (a == 0.F || b == 0.F || a == b) {
			_u(1.F);
			_wot(_u.get_mean_of_window(window));
			return;
		}

		float prop = div_high_by_low(a, b);
		int mop = static_cast<int>(prop);
		float flop = prop - static_cast<float>(mop);

		if (flop == 0.F) {
			flop = 1.F;
		} else if (flop >= 0.5F) {
			flop = abs(flop - 1.F) + 1.F;

		} else if (flop < 0.5F) {
			flop += 1.F;
		}

		_u(flop);
		_wot(_u.get_mean_of_window(window));
	}

	inline auto handle_case_optimizations(const ItvInfo& itvi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		if (itvi.total_taps == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo&  /*itvh*/,
						   vector<float> doot[],
						   const int& i)
	{
		pmod = base + _wot.get_mean_of_window(max_moving_window_size);
		pmod = CalcClamp(pmod, min_mod, max_mod);
		doot[_pmod][i] = pmod;
	}
};

struct RM_Sequencing
{
	// params.. loaded by runningman and then set from there
	int max_oht_len = 0;
	int max_off_spacing = 0;
	int max_burst_len = 0;
	int max_jack_len = 0;

	inline void set_params(const float& moht,
						   const float& moff,
						   const float& mburst,
						   const float& mjack)
	{
		max_oht_len = static_cast<int>(moht);
		max_off_spacing = static_cast<int>(moff);
		max_burst_len = static_cast<int>(mburst);
		max_jack_len = static_cast<int>(mjack);
	}

	col_type anchor_col = col_init;
	col_type now_col = col_init;

	// sequencing counters
	// only allow this rm's anchor col to start sequences
	bool in_the_nineties = false;
	// try to allow 1 burst?
	bool is_bursting = false;
	bool had_burst = false;

	int ran_taps = 0;
	int anchor_len = 0;

	int off_taps_same = 0;
	int oht_taps = 0;
	int oht_len = 0;
	int off_taps = 0;
	int off_len = 0;

	int jack_taps = 0;
	int jack_len = 0;

	float max_ms = ms_init;
	float now = 0.F;
	float temp_ms = 0.F;
	float last_anchor_time = s_init;

#pragma region functions

	inline void reset()
	{
		// don't touch anchor col

		// now_col and now don't need to be reset either

		in_the_nineties = false;
		is_bursting = false;
		had_burst = false;

		ran_taps = 0;
		anchor_len = 0;

		off_taps_same = 0;
		oht_taps = 0;
		oht_len = 0;
		off_taps = 0;
		off_len = 0;

		jack_taps = 0;
		jack_len = 0;

		max_ms = ms_init;
		last_anchor_time = ms_init;

		// if we are resetting and this column is the anchor col, restart again
		if (anchor_col == now_col) {
			handle_anchor_progression();
		}
	}

	inline void full_reset()
	{
		// don't touch anchor col

		reset();
		now = 0.F;
		now_col = col_init;
	}

	inline void handle_off_tap()
	{
		if (!in_the_nineties) {
			return;
		}

		++ran_taps;
		++off_taps;
		++off_len;

		// offnote, reset jack length & oht length
		jack_len = 0;

		// handle progression for increasing off_len
		handle_off_tap_progression();
	}

	inline void handle_off_tap_completion()
	{
		// if we end while bursting due to hitting an anchor, complete it
		if (is_bursting) {
			is_bursting = false;
			had_burst = true;
		}
		// reset off_len counter
		off_len = 0;
	}

	inline void handle_off_tap_progression()
	{
		// resume off tap progression caused by another consecutive off tap
		// normal behavior if we have already allowed for 1 burst, reset if the
		// offtap sequence exceeds the spacing limit; this will also catch
		// bursts that exceed the max burst length
		if (had_burst) {
			if (off_len > max_off_spacing) {
				reset();
				return;
			}
			// don't care about any other behavior here
			return;
		}

		// if we are in a burst, allow it to finish and when it does set the
		// had_burst flag rather than resetting, if the burst continues beyond
		// the max burst length then it will be reset via other means
		// (we must be in a burst if off_len == max_burst_len)
		if (off_len == max_burst_len) {
			handle_off_tap_completion();
			return;
		}

		// haven't had or started a burst yet, if we exceed max_off_spacing, set
		// is_bursting to true and allow it to continue, otherwise, do nothing
		if (off_len > max_off_spacing) {
			is_bursting = true;
		}
	}

	inline void handle_anchor_progression()
	{
		// start a sequence whenever we hit this rm's anchor col, if we aren't
		// already in one
		if (in_the_nineties) {
			// break the anchor if the next note is too much slower than the
			// lowest one, but only after we've already set the initial anchor
			temp_ms = ms_from(now, last_anchor_time);
			// account for float precision error and small bpm flux
			if (temp_ms > max_ms + 5.F) {
				reset();
			} else {
				max_ms = temp_ms;
			}
		} else {
			// set first anchor val
			max_ms = 5000.F;
			in_the_nineties = true;
		}

		last_anchor_time = now;
		++ran_taps;
		++anchor_len;

		// handle completion of off tap progression
		handle_off_tap_completion();
	}

	inline void handle_jack_progression()
	{
		++ran_taps;
		//++anchor_len; // do this for jacks?
		++jack_len;
		++jack_taps;

		// handle completion of off tap progression
		handle_off_tap_completion();

		// make sure to set the anchor col when resetting if we exceed max jack
		// len
		if (jack_len > max_jack_len) {
			reset();
		}
	}

	inline void handle_cross_column_branching()
	{
		// we are comparing 2 different enum types here, but this is what we
		// want. cc_left_right is 0, col_left is 0. if we are cc_left_right then
		// we have landed on the right column, so if we have cc (0) ==
		// anchor_col (0), we are entering the off column (right) of the anchor
		// (left). perhaps left_right and right_left should be flipped in the
		// cc_type enum to make this more intuitive (but probably not)

		// NOT an anchor
		if (anchor_col != now_col && in_the_nineties) {
			handle_off_tap();
			// same hand offtap
			++off_taps_same;
			return;
		}
		handle_anchor_progression();
	}

	inline void handle_oht_progression()
	{
		// we only care about ohts that end off-anchor
		if (now_col != anchor_col) {
			++oht_len;
			++oht_taps;
			if (oht_len > max_oht_len) {
				reset();
			}
		}
	}

	inline void operator()(const metaHandInfo& mhi)
	{

		now_col = mhi.col;
		now = mhi.row_time;

		// play catch up, treat offhand jumps like 2 offtaps
		if (in_the_nineties && mhi.offhand_taps > 0) {
			// reset oht len if we hit this (not really robust buuuut)
			oht_len = 0;

			for (int i = 0; i < mhi.offhand_taps; ++i) {
				handle_off_tap();
			}
		}

		// cosmic brain
		if (mhi.mt == meta_oht) {
			handle_oht_progression();
		}

		switch (mhi.cc) {
			case cc_left_right:
			case cc_right_left:
				handle_cross_column_branching();
				break;
			case cc_jump_single:
				if (mhi.offhand_taps > 0) {
					// if we have a jump -> single, and the last
					// note was an offhand tap, and the single
					// is the anchor col, then we have an anchor
					if ((mhi.col == col_left && anchor_col == col_left) ||
						(mhi.col == col_right && anchor_col == col_right)) {
						handle_anchor_progression();
					} else {
						// otherwise we have an off anchor tap
						handle_off_tap();
						// same hand offtap
						++off_taps_same;
					}
				} else {
					// if we are jump -> single and the last
					// note was not an offhand hand tap, we have
					// a jack
					handle_jack_progression();
				}
				break;
			case cc_single_single:
			case cc_single_jump:
				// if last note was an offhand tap, this is by
				// definition part of the anchor
				if (mhi.offhand_taps > 0) {
					handle_anchor_progression();
				} else {
					// if not, a jack
					handle_jack_progression();
				}
				break;
			case cc_jump_jump:
				// this is kind of a gray area, given that
				// the difficulty of runningmen comes from
				// the tight turns on the same hand... we
				// will treat this as a jack even though
				// technically it's an "anchor" when the
				// last tap was an offhand tap
				handle_jack_progression();
				break;
			case cc_init:
				if (now_col == anchor_col) {
					handle_anchor_progression();
				}
				break;
			default:
				assert(0);
				break;
		}
	}
#pragma endregion
};

struct RunningManMod
{
	const CalcPatternMod _pmod = RanMan;
	const vector<CalcPatternMod> _dbg{ RanLen,		RanAnchLen, RanAnchLenMod,
									   RanJack,		RanOHT,		RanOffS,
									   RanPropAll,  RanPropOff, RanPropOHT,
									   RanPropOffS, RanPropJack };
	const std::string name = "RunningManMod";

#pragma region params

	float min_mod = 0.95F;
	float max_mod = 1.35F;
	float base = 0.8F;
	float min_anchor_len = 5.F;
	float min_taps_in_rm = 1.F;
	float min_off_taps_same = 1.F;

	float total_prop_scaler = 1.F;
	float total_prop_min = 0.F;
	float total_prop_max = 1.F;

	float off_tap_prop_scaler = 1.3F;
	float off_tap_prop_min = 0.F;
	float off_tap_prop_max = 1.25F;
	float off_tap_prop_base = 0.05F;

	float off_tap_same_prop_scaler = 1.F;
	float off_tap_same_prop_min = 0.F;
	float off_tap_same_prop_max = 1.25F;
	float off_tap_same_prop_base = 0.25F;

	float anchor_len_divisor = 2.5F;

	float min_jack_taps_for_bonus = 1.F;
	float jack_bonus_base = 0.1F;

	float min_oht_taps_for_bonus = 1.F;
	float oht_bonus_base = 0.1F;

	// params for rm_sequencing, these define conditions for resetting
	// runningmen sequences
	float max_oht_len = 2.F;
	float max_off_spacing = 2.F;
	float max_burst_len = 6.F;
	float max_jack_len = 1.F;

	const vector<pair<std::string, float*>> _params{

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "min_anchor_len", &min_anchor_len },
		{ "min_taps_in_rm", &min_taps_in_rm },
		{ "min_off_taps_same", &min_off_taps_same },

		{ "total_prop_scaler", &total_prop_scaler },
		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },

		{ "off_tap_prop_scaler", &off_tap_prop_scaler },
		{ "off_tap_prop_min", &off_tap_prop_min },
		{ "off_tap_prop_max", &off_tap_prop_max },
		{ "off_tap_prop_base", &off_tap_prop_base },

		{ "off_tap_same_prop_scaler", &off_tap_same_prop_scaler },
		{ "off_tap_same_prop_min", &off_tap_same_prop_min },
		{ "off_tap_same_prop_max", &off_tap_same_prop_max },
		{ "off_tap_same_prop_base", &off_tap_same_prop_base },

		{ "anchor_len_divisor", &anchor_len_divisor },

		{ "min_jack_taps_for_bonus", &min_jack_taps_for_bonus },
		{ "jack_bonus_base", &jack_bonus_base },

		{ "min_oht_taps_for_bonus", &min_oht_taps_for_bonus },
		{ "oht_bonus_base", &oht_bonus_base },

		// params for rm_sequencing
		{ "max_oht_len", &max_oht_len },
		{ "max_off_spacing", &max_off_spacing },
		{ "max_burst_len", &max_burst_len },
		{ "max_jack_len", &max_jack_len },
	};
#pragma endregion params and param map

	// stuff for making mod
	RM_Sequencing rms[2];
	// longest sequence for this interval
	RM_Sequencing rm;

	int test = 0;
	float total_prop = 0.F;
	float off_tap_prop = 0.F;
	float off_tap_same_prop = 0.F;

	float anchor_len_comp = 0.F;
	float jack_bonus = 0.F;
	float oht_bonus = 0.F;

	float pmod = neutral;

#pragma region generic functions

	inline void full_reset()
	{
		rm.full_reset();
		for (auto& rm : rms) {
			rm.full_reset();
		}

		test = 0;
		total_prop = 0.F;
		off_tap_prop = 0.F;
		off_tap_same_prop = 0.F;

		anchor_len_comp = 0.F;
		jack_bonus = 0.F;
		oht_bonus = 0.F;

		pmod = neutral;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		// don't try to figure out which column a prospective anchor is on, just
		// run two passes with each assuming a different column
		rms[0].anchor_col = col_left;
		rms[1].anchor_col = col_right;
		rms[0].set_params(
		  max_oht_len, max_off_spacing, max_burst_len, max_jack_len);
		rms[1].set_params(
		  max_oht_len, max_off_spacing, max_burst_len, max_jack_len);

		doot[_pmod].resize(size);
		if (debug_lmao) {
			for (auto& mod : _dbg) {
				doot[mod].resize(size);
			}
		}
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const metaHandInfo& now)
	{
		// sequencing objects should be moved int mhi itself
		for (auto& rm : rms) {
			rm(now);
		}

		// use the biggest anchor that has existed in this interval
		test = rms[0].anchor_len > rms[1].anchor_len ? 0 : 1;

		if (rms[test].anchor_len > rm.anchor_len) {
			rm = rms[test];
		}
	}

	inline void set_dbg(vector<float> doot[], const int& i)
	{
		if (debug_lmao) {
			doot[RanLen][i] = 1.F;
			doot[RanAnchLen][i] =
			  (static_cast<float>(rm.anchor_len) / 30.F) + 0.5F;
			doot[RanAnchLenMod][i] = anchor_len_comp;
			doot[RanOHT][i] = static_cast<float>(rm.oht_taps);
			doot[RanOffS][i] = static_cast<float>(rm.off_taps_same);
			doot[RanJack][i] = static_cast<float>(rm.jack_taps);
			doot[RanPropAll][i] = total_prop;
			doot[RanPropOff][i] = off_tap_prop;
			doot[RanPropOffS][i] = off_tap_same_prop;
			doot[RanPropOHT][i] = oht_bonus;
			doot[RanPropJack][i] = jack_bonus;
		}
	}

	inline auto handle_case_optimizations(const RM_Sequencing& rm,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// we could mni check for empty intervals like the other mods but it
		// doesn't really matter and this is probably more useful for debug
		// output

		// we could decay in this but it may conflict/be redundant with how
		// runningmen sequences are constructed, if decays are used we would
		// probably generate the mod not from the highest of any interval, but
		// from whatever sequences are still alive by the end
		if (rm.anchor_len < min_anchor_len) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}
		if (rm.ran_taps < min_taps_in_rm) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}
		if (rm.off_taps_same < min_off_taps_same) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}
		return false;
	}

	inline void operator()(vector<float> doot[], const int& i)
	{
		if (handle_case_optimizations(rm, doot, i)) {
			set_dbg(doot, i);
			rm.reset();
			return;
		}

		// the pmod template stuff completely broke the js/hs/cj mods.. so..
		// these might also be broken... investigate later

		// taps in runningman / total taps in interval... i think? can't
		// remember when i reset total taps tbh.. this might be useless
		total_prop = 1.F;

		// number anchor taps / number of non anchor taps
		off_tap_prop = fastpow(pmod_prop(rm.anchor_len,
										 rm.ran_taps,
										 off_tap_prop_scaler,
										 off_tap_prop_min,
										 off_tap_prop_max,
										 off_tap_prop_base),
							   2.F);

		// number of same hand off anchor taps / anchor taps, basically stuff is
		// really hard when this is high (a value of 0.5 is a triplet every
		// other anchor)
		off_tap_same_prop = pmod_prop(rm.off_taps_same,
									  rm.anchor_len,
									  off_tap_same_prop_scaler,
									  off_tap_same_prop_min,
									  off_tap_same_prop_max,
									  off_tap_same_prop_base);

		// anchor length component
		anchor_len_comp =
		  static_cast<float>(rm.anchor_len) / anchor_len_divisor;

		// jacks in anchor component, give a small bonus i guess
		jack_bonus =
		  rm.jack_taps >= min_jack_taps_for_bonus ? jack_bonus_base : 0.F;

		// ohts in anchor component, give a small bonus i guess
		// not done
		oht_bonus =
		  rm.oht_taps >= min_oht_taps_for_bonus ? oht_bonus_base : 0.F;

		// we could scale the anchor to speed if we want but meh
		// that's really complicated/messy/error prone
		pmod = base + anchor_len_comp + jack_bonus + oht_bonus;
		pmod = CalcClamp(
		  fastsqrt(pmod * total_prop * off_tap_prop /** off_tap_same_prop*/),
		  min_mod,
		  max_mod);

		doot[_pmod][i] = pmod;
		set_dbg(doot, i);

		// reset interval highest when we're done
		rm.reset();
	}
};

// big brain stuff
static const float wrjt_cv_factor = 3.F;

// should update detection so it's more similar to updated wrr
// probably needs better debugoutput
struct WideRangeJumptrillMod
{
	const CalcPatternMod _pmod = { WideRangeJumptrill };
	const std::string name = "WideRangeJumptrillMod";

#pragma region params

	float window_param = 3.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float base = 0.4F;

	float cv_reset = 0.5F;
	float cv_threshhold = 0.15F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "cv_reset", &cv_reset },
		{ "cv_threshhold", &cv_threshhold },
	};
#pragma endregion params and param map

	int window = 0;
	CalcWindow<int> _mw_jt;
	int jt_counter = 0;

	bool bro_is_this_file_for_real = false;
	bool last_passed_check = false;

	float pmod = neutral;

	// swap to my container maybe unless it SUCKS
	vector<float> seq_ms = { 0.F, 0.F, 0.F };
	// uhhh lazy way out of tracking all the floats i think
	// put this back again? seems to work well for wrr, however wrr is already
	// more generalized anyway
	// float moving_cv = moving_cv_init;

#pragma region generic functions

	inline void full_reset()
	{
		_mw_jt.zero();
		jt_counter = 0;

		for (auto& v : seq_ms) {
			v = 0.F;
		}

		bro_is_this_file_for_real = false;
		last_passed_check = false;
		pmod = neutral;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline auto handle_ccacc_timing_check() -> bool
	{
		// we don't want to suppress actual streams that use this pattern, so we
		// will keep a fairly tight requirement on the ms variance

		// we are currently assuming we have xyyx always, and are not interested
		// in xxyy as a part of xxyyxxyyxx transitions, given these conditions
		// we can do some pretty neat stuff

		// seq_ms 0 and 2 will both be cross column ms values, or left->right /
		// right->left, seq_ms 1 will always be an anchor value, so,
		// right->right for example. our interest is in hard nerfing long chains
		// of xyyx patterns that won't get picked up by any of the roll scalers
		// or balance scalers, but are still jumptrillable, for this condition
		// to be true the anchor ms length has to be within a certain ratio of
		// the cross column ms lengths, enabling the cross columns to be hit
		// like jumptrilled flams, the optimal ratio for inflating nps is about
		// 3:1, this is short enough that the nps boost is still high, but long
		// enough that it doesn't become endless minijacking on the anchor
		// given these conditions we can divide seq_ms[1] by 3 and cv check the
		// array, with 2 identical values cc values, even if the anchor ratio
		// floats between 2:1 and 4:1 the cv should still be below 0.25, which
		// is a sensible cutoff that should avoid punishing happenstances of
		// this pattern in just regular files

		seq_ms[1] /= wrjt_cv_factor;
		last_passed_check = cv(seq_ms) < cv_threshhold;
		seq_ms[1] *= wrjt_cv_factor;

		return last_passed_check;
	}

	inline auto handle_acca_timing_check() -> bool
	{
		seq_ms[1] *= wrjt_cv_factor;
		last_passed_check = cv(seq_ms) < cv_threshhold;
		seq_ms[1] /= wrjt_cv_factor;

		return last_passed_check;
	}

	inline auto handle_roll_timing_check() -> bool
	{
		// see ccacc timing check in wrjt for explanations, it's basically the
		// same but we have to invert the multiplication depending on which
		// value is higher between seq_ms[0] and seq_ms[1] (easiest to dummy up
		// a roll in an editor to see why)

		// multiply seq_ms[1] by 3 for the cv check, then put it back so it
		// doesn't interfere with the next round
		if (seq_ms[0] > seq_ms[1]) {
			seq_ms[1] *= wrjt_cv_factor;
			last_passed_check = cv(seq_ms) < cv_threshhold;
			seq_ms[1] /= wrjt_cv_factor;
			return last_passed_check;
		}
		// same thing but divide
		seq_ms[1] /= wrjt_cv_factor;
		last_passed_check = cv(seq_ms) < cv_threshhold;
		seq_ms[1] *= wrjt_cv_factor;
		return last_passed_check;
	}

	inline void update_seq_ms(const metaHandInfo& now)
	{
		seq_ms[0] = seq_ms[1]; // last_last
		seq_ms[1] = seq_ms[2]; // last

		// update now
		// for anchors, track tc_ms
		if (now.cc == cc_single_single) {
			seq_ms[2] = now.tc_ms;
			// for cc_left_right or cc_right_left, track cc_ms
		} else {
			seq_ms[2] = now.cc_ms_any;
		}
	}

	inline auto check_last_mt(const meta_type& mt) -> bool
	{
		if (mt == meta_acca || mt == meta_ccacc || mt == meta_oht) {
			if (last_passed_check) {
				return true;
			}
		}
		return false;
	}

	inline void bibblybop(const meta_type& mt)
	{
		++jt_counter;
		if (bro_is_this_file_for_real) {
			++jt_counter;
		}
		if (check_last_mt(mt)) {
			++jt_counter;
			bro_is_this_file_for_real = true;
		}
	}

	inline void advance_sequencing(const metaHandInfo& now)
	{
		// ignore if we hit a jump
		if (now.col == col_ohjump) {
			return;
		}

		update_seq_ms(now);

		// look for stuff thats jumptrillyable.. if that stuff... then leads
		// into more stuff.. that is jumptrillyable... then .... badonk it

		switch (now.mt) {
			case meta_oht:
				if (handle_roll_timing_check()) {
					bibblybop(now.last_mt);
					return;
				}
				break;
			case meta_ccacc:
				if (handle_ccacc_timing_check()) {
					bibblybop(now.last_mt);
					return;
				}
				break;
			case meta_acca:
				// don't bother adding if the ms values look benign
				if (handle_acca_timing_check()) {
					bibblybop(now.last_mt);
					return;
				}
				break;
			default:
				break;
		}

		bro_is_this_file_for_real = false;
	}

	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// no taps, no jt
		if (itvhi.get_taps_windowi(window) == 0 ||
			_mw_jt.get_total_for_window(window) == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo& itvhi,
						   vector<float> doot[],
						   const int& i)
	{
		_mw_jt(jt_counter);

		if (handle_case_optimizations(itvhi, doot, i)) {
			interval_reset();
			return;
		}

		pmod =
		  itvhi.get_taps_windowf(window) / _mw_jt.get_total_for_windowf(window);

		pmod = CalcClamp(pmod, min_mod, max_mod);
		doot[_pmod][i] = pmod;

		interval_reset();
	}

	inline void interval_reset()
	{
		// we could count these in metanoteinfo but let's do it here for now,
		// reset every interval when finished
		jt_counter = 0;
	}
};

// ok new plan we will incloop the joomp
struct WideRangeRollMod
{
	const CalcPatternMod _pmod = WideRangeRoll;
	const std::string name = "WideRangeRollMod";

#pragma region params

	float window_param = 5.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float base = 0.15F;
	float scaler = 0.9F;

	float cv_reset = 1.F;
	float cv_threshold = 0.35F;
	float other_cv_threshold = 0.3F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
		{ "scaler", &scaler },

		{ "cv_reset", &cv_reset },
		{ "cv_threshold", &cv_threshold },
		{ "other_cv_threshold", &other_cv_threshold },
	};
#pragma endregion params and param map

	int window = 0;

	// moving window of longest roll sequences seen in the interval
	CalcWindow<int> _mw_max;

	// we want to keep custom adjusted ms values here
	CalcWindow<float> _mw_adj_ms;

	bool last_passed_check = false;
	int nah_this_file_aint_for_real = 0;
	int max_thingy = 0;
	float hi_im_a_float = 0.F;

	vector<float> idk_ms = { 0.F, 0.F, 0.F, 0.F };
	vector<float> seq_ms = { 0.F, 0.F, 0.F };

	float moving_cv = cv_reset;
	float pmod = min_mod;

#pragma region generic functions

	inline void full_reset()
	{
		_mw_max.zero();
		_mw_adj_ms.zero();

		last_passed_check = false;
		nah_this_file_aint_for_real = 0;
		max_thingy = 0;
		hi_im_a_float = 0.F;

		for (auto& v : seq_ms) {
			v = 0.F;
		}
		for (auto& v : idk_ms) {
			v = 0.F;
		}

		moving_cv = cv_reset;
		pmod = neutral;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void zoop_the_woop(const int& pos,
							  const float& div,
							  const float& scaler = 1.F)
	{
		seq_ms[pos] /= div;
		last_passed_check = do_timing_thing(scaler);
		seq_ms[pos] *= div;
	}

	inline void woop_the_zoop(const int& pos,
							  const float& mult,
							  const float& scaler = 1.F)
	{
		seq_ms[pos] *= mult;
		last_passed_check = do_timing_thing(scaler);
		seq_ms[pos] /= mult;
	}

	inline auto do_timing_thing(const float& scaler) -> bool
	{
		_mw_adj_ms(seq_ms[1]);

		if (_mw_adj_ms.get_cv_of_window(window) > other_cv_threshold) {
			return false;
		}

		hi_im_a_float = cv(seq_ms);

		// ok we're pretty sure it's a roll don't bother with the test
		if (hi_im_a_float < 0.12F) {
			moving_cv = (hi_im_a_float + moving_cv + hi_im_a_float) / 3.F;
			return true;
		}
		{
			moving_cv = (hi_im_a_float + moving_cv) / 2.f;
		}

		return moving_cv < cv_threshold / scaler;
	}

	inline auto do_other_timing_thing(const float& scaler) -> bool
	{
		_mw_adj_ms(idk_ms[1]);
		_mw_adj_ms(idk_ms[2]);

		if (_mw_adj_ms.get_cv_of_window(window) > other_cv_threshold) {
			return false;
		}

		hi_im_a_float = cv(idk_ms);

		// ok we're pretty sure it's a roll don't bother with the test
		if (hi_im_a_float < 0.12F) {
			moving_cv = (hi_im_a_float + moving_cv + hi_im_a_float) / 3.F;
			return true;
		}
		{
			moving_cv = (hi_im_a_float + moving_cv) / 2.f;
		}

		return moving_cv < cv_threshold / scaler;
	}

	inline void handle_ccacc_timing_check() { zoop_the_woop(1, 2.5F, 1.25F); }

	inline void handle_roll_timing_check()
	{
		if (seq_ms[1] > seq_ms[0]) {
			zoop_the_woop(1, 2.5F);
		} else {
			seq_ms[0] /= 2.5F;
			seq_ms[2] /= 2.5F;
			last_passed_check = do_timing_thing(1.F);
			seq_ms[0] *= 2.5F;
			seq_ms[2] *= 2.5F;
		}
	}

	inline void handle_ccsjjscc_timing_check(const float& now)
	{
		// translate over the values
		idk_ms[2] = seq_ms[0];
		idk_ms[1] = seq_ms[1];
		idk_ms[0] = seq_ms[2];

		// add the new value
		idk_ms[3] = now;

		// run 2 tests so we can keep a stricter cutoff
		// need to put cv in array thingy mcboop
		// check 1
		idk_ms[1] /= 2.5F;
		idk_ms[2] /= 2.5F;

		do_other_timing_thing(1.25F);

		idk_ms[1] *= 2.5F;
		idk_ms[2] *= 2.5F;

		if (last_passed_check) {
			return;
		}

		// test again
		idk_ms[1] /= 3.F;
		idk_ms[2] /= 3.F;

		do_other_timing_thing(1.25F);

		idk_ms[1] *= 3.F;
		idk_ms[2] *= 3.F;
	}

	inline void complete_seq()
	{
		if (nah_this_file_aint_for_real > 0) {
			max_thingy = max(max_thingy, nah_this_file_aint_for_real);
		}
		nah_this_file_aint_for_real = 0;
	}

	inline void bibblybop(const meta_type& last_mt)
	{
		// see below
		if (last_mt == meta_enigma) {
			moving_cv = (moving_cv + hi_im_a_float) / 2.F;
		} else if (last_mt == meta_meta_enigma) {
			moving_cv = (moving_cv + hi_im_a_float + hi_im_a_float) / 3.F;
		}

		if (!last_passed_check) {
			complete_seq();
			return;
		}

		++nah_this_file_aint_for_real;

		// if we are here and mt.last == meta enigma, we skipped 1 note
		// before we identified a jumptrillable roll continuation, if meta
		// meta enigma, 2

		// borp it
		if (last_mt == meta_enigma) {
			++nah_this_file_aint_for_real;
		}

		// same but even more-er
		if (last_mt == meta_meta_enigma) {
			nah_this_file_aint_for_real += 2;
		}
	}

	inline void advance_sequencing(const metaHandInfo& now)
	{
		// we will let ohjumps through here

		update_seq_ms(now);
		if (now.cc == cc_single_jump || now.cc == cc_jump_single) {
			return;
		}

		if (now.cc == cc_jump_jump) {
			// its an actual jumpjack/jumptrill, don't bother with timing checks
			// disable for now
			if (nah_this_file_aint_for_real > 0) {
				bibblybop(now.last_mt);
			}
			return;
		}

		// look for stuff thats jumptrillyable.. if that stuff... then leads
		// into more stuff.. that is jumptrillyable... then .... badonk it
		switch (now.mt) {
			case meta_acca:
				// unlike wrjt we want to complete and reset on these
				complete_seq();
				break;
			case meta_oht:
				handle_roll_timing_check();
				bibblybop(now.last_mt);
				break;
			case meta_ccacc:
				handle_ccacc_timing_check();
				bibblybop(now.last_mt);
				break;
			case meta_ccsjjscc:
			case meta_ccsjjscc_inverted:
				handle_ccsjjscc_timing_check(now.cc_ms_any);
				bibblybop(now.last_mt);
				break;
			case meta_init:
			case meta_enigma:
				// this could yet be something we are interested in, but we
				// don't know yet, so just wait and see
				break;
			case meta_meta_enigma:
				// it's been too long... your vision becomes blurry.. your
				// memory fades... why are we here again? what are we trying
				// to do? who are we....
				complete_seq();
				break;
			default:
				assert(0);
				break;
		}
	}

	inline void update_seq_ms(const metaHandInfo& now)
	{
		seq_ms[0] = seq_ms[1]; // last_last
		seq_ms[1] = seq_ms[2]; // last

		// update now
		// for anchors, track tc_ms
		if (now.cc == cc_single_single) {
			seq_ms[2] = now.tc_ms;
			// for cc_left_right or cc_right_left, track cc_ms
		} else {
			seq_ms[2] = now.cc_ms_any;
		}
	}

	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// check taps for _this_ interval, if there's none, and there was a
		// powerful roll mod before, the roll mod will extend into the empty
		// interval at minimum value due to 0/n, and then the smoother will push
		// that push that into the adjecant intervals
		// then check for the window values, perhaps we should also neutral set
		// if a large sequence has just ended on this interval, but that may
		// change too much and the tuning is already looking good anyway
		if (itvhi.get_taps_nowi() == 0 || itvhi.get_taps_windowi(window) == 0 ||
			_mw_max.get_total_for_window(window) == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo& itvhi,
						   vector<float> doot[],
						   const int& i)
	{
		max_thingy = nah_this_file_aint_for_real > max_thingy
					   ? nah_this_file_aint_for_real
					   : max_thingy;

		_mw_max(max_thingy);

		if (handle_case_optimizations(itvhi, doot, i)) {
			interval_reset();
			return;
		}

		// really uncertain about the using the total of _mw_max here, but
		// that's what it was, so i'll keep it for now
		float zomg = itvhi.get_taps_windowf(window) /
					 _mw_max.get_total_for_windowf(window);

		pmod *= zomg;
		pmod = CalcClamp(base + fastsqrt(pmod), min_mod, max_mod);
		doot[_pmod][i] = pmod;

		interval_reset();
	}

	inline void interval_reset() { max_thingy = 0; }
};

struct flam
{
	// cols seen
	unsigned unsigned_unseen = 0;

	// size in ROWS, not columns, if flam size == 1 we do not yet have a flam
	// and we have no current relevant values in ms[], any that are set will be
	// leftovers from the last sequence, this is to optimize out setting
	// rowtimes or calculating ms times
	int size = 1;

	// size > 1, is this actually more efficient than calling a bool check func?
	bool flammin = false;

	// ms values, 3 ms values = 4 rows, optimize by just recycling values
	// without resetting and indexing up to the size counter to get duration
	float ms[3] = {
		0.F,
		0.F,
		0.F,
	};

	// is this row exclusively additive with the current flam sequence?
	inline auto comma_comma_coolmeleon(const unsigned& notes) -> bool
	{
		return (unsigned_unseen & notes) == 0;
	}

	// to avoid keeping another float ??? idk
	inline auto get_dur() -> float
	{
		// cba to loop
		switch (size) {
			case 1:
				// can't have 1 row flams
				assert(0);
			case 2:
				return ms[0];
				break;
			case 3:
				return ms[0] + ms[1];
				break;
			case 4:
				return ms[0] + ms[1] + ms[2];
				break;
			default:
				assert(0);
				break;
		}
		assert(0);
		return 0.F;
	}

	inline void start(const float& ms_now, const unsigned& notes)
	{
		flammin = true;
		grow(ms_now, notes);
	}

	inline void grow(const float& ms_now, const unsigned& notes)
	{
		unsigned_unseen |= notes;

		assert(size < 5);

		ms[size - 1] = ms_now;

		// adjust size after setting ms, size starts at 1
		++size;
	}

	inline void reset()
	{
		flammin = false;
		size = 1;
	}
};

// jk this is actually pretty optimized and even if it isn't it's like 0.03% of
// samples
struct FJ_Sequencing
{
	flam flim;

	// scan for flam chords in this window
	float group_tol = 0.F;
	// tolerance for each column step
	float step_tol = 0.F;
	float mod_scaler = 0.F;

	// number of flams
	int flam_counter = 0;

	// track up to 4 flams per interval, if the number of flams exceeds this
	// number we'll just min_set (OR we could keep a moving array of flams, and
	// not flams, which would make consecutive flams much more debilitating and
	// interval proof the sequencing.. however.. it's probably not necessary to
	// get that fancy

	float mod_parts[4] = { 1.F, 1.F, 1.F, 1.F };

	// there's too many flams already, don't bother with new sequencing and
	// shortcut into a minset in flamjammod
	// technically this means we won't start constructing sequences again until
	// the next interval.. not sure if this is desired behavior
	bool the_fifth_flammament = false;

	inline void set_params(const float& gt, const float& st, const float& ms)
	{
		group_tol = gt;
		step_tol = st;
		mod_scaler = ms;
	}

	inline void complete_seq()
	{
		assert(flim.size > 1);
		mod_parts[flam_counter] = construct_mod_part();
		++flam_counter;

		// bro its just flams
		if (flam_counter > 4) {
			the_fifth_flammament = true;
		}

		flim.reset();
	}

	inline auto flammin_col_check(const unsigned& notes) -> bool
	{
		// this function should never be used to start a flam
		assert(flim.flammin);

		// note : in order to prevent the last row of a quad flam from being
		// elibible to start a new flam (logically it makes no sense), instead
		// of catching full quads and resetting when we get them, we'll let them
		// pass throgh into the next note row, no matter what the row is it will
		// fail the xor check and be reset then, making only the row _after_ the
		// full flam eligible for a new start
		return flim.comma_comma_coolmeleon(notes);
	}

	// check for anything that would break the sequence
	inline auto flammin_tol_check(const float& ms_now) -> bool
	{
		// check if ms from last row is greater than the group tolerance
		if (ms_now > group_tol) {
			return false;
		}

		// check if the new flam duration would exceed the group tolerance with
		// the current row added
		if (flim.get_dur() + ms_now > group_tol) {
			return false;
		}

		// we may be able to continue the sequence, run the col check
		return true;
	}

	inline void operator()(const float& ms_now, const unsigned& notes)
	{
		// if we already have the max number of flams
		// (maybe should remove this shortcut optimization)
		// seems like we never even hit it so...
		if (the_fifth_flammament) {
			return;
		}

		// haven't started, if we're under the step tolerance, start
		if (!flim.flammin) {
			// 99.99% of cases
			if (ms_now > step_tol) {
				return;
			}
			{
				flim.start(ms_now, notes);
			}
		} else {
			// passed the tolerance checks, run the col checks
			if (flammin_tol_check(ms_now)) {

				// passed col check, advance flam
				if (flammin_col_check(notes)) {
					flim.grow(ms_now, notes);
				} else {
					// we failed the col check, but we've passed the tol checks,
					// which means this row is eligible to begin a new flam
					// sequence, complete the one that exists and start again
					complete_seq();
					flim.start(ms_now, notes);
				}
			} else {
				// reset if we exceed tolerance checks
				complete_seq();
			}
		}
	}

	inline void reset()
	{
		// we probably don't want to do this, just let it build potential
		// sequences across intervals
		// flam.reset();

		the_fifth_flammament = false;
		flam_counter = 0;

		// reset everything to 1, as we build flams we will replace 1 with < 1
		// values, the more there are, the lower (stronger) the pattern mod
		for (auto& v : mod_parts) {
			v = 1.F;
		}
	}

	inline auto construct_mod_part() -> float
	{
		// total duration of flam
		float dur = flim.get_dur();

		// scale to size of flam, we want jumpflams to punish less than quad
		// flams (while still downscaling jumptrill flams)
		// flams that register as 95% of the size adjusted window will be
		// punished less than those that register at 2%
		float dur_prop = dur / group_tol;
		dur_prop /= (static_cast<float>(flim.size) / mod_scaler);
		dur_prop = CalcClamp(dur_prop, 0.F, 1.F);

		return fastsqrt(dur_prop);
	}
};

// MAKE FLAM WIDE RANGE?
struct FlamJamMod
{
	const CalcPatternMod _pmod = FlamJam;
	const std::string name = "FlamJamMod";

#pragma region params
	float min_mod = 0.5F;
	float max_mod = 1.F;
	float mod_scaler = 2.75F;

	float group_tol = 35.F;
	float step_tol = 17.5F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_scaler", &mod_scaler },

		// params for fj_sequencing
		{ "group_tol", &group_tol },
		{ "step_tol", &step_tol },
	};
#pragma endregion params and param map

	// sequencer
	FJ_Sequencing fj;
	float pmod = min_mod;

#pragma region generic functions
	inline void setup(vector<float> doot[], const int& size)
	{
		fj.set_params(group_tol, step_tol, mod_scaler);

		doot[_pmod].resize(size);
		/*if (debug_lmao)
			for (auto& mod : _dbg)
				doot[mod].resize(size);*/
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const metaRowInfo& now)
	{
		fj(now.ms_now, now.notes);
	}

	inline void set_dbg(vector<float> doot[], const int& i)
	{
		//
	}

	inline auto handle_case_optimizations(const FJ_Sequencing& fj,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// no flams
		if (fj.mod_parts[0] == 1.F) {
			neutral_set(_pmod, doot, i);
		}

		if (fj.the_fifth_flammament) {
			mod_set(_pmod, doot, i, min_mod);
		}

		return false;
	}

	inline void operator()(vector<float> doot[], const int& i)
	{
		if (handle_case_optimizations(fj, doot, i)) {
			// set_dbg(doot, i);
			fj.reset();
			return;
		}

		// water down single flams
		pmod = 1.F;
		for (auto& mp : fj.mod_parts) {
			pmod += mp;
		}
		pmod /= 5.F;
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[_pmod][i] = pmod;
		// set_dbg(doot, i);

		// reset flags n stuff
		fj.reset();
	}
};

// this should mayb track offhand taps like the old behavior did
struct WideRangeBalanceMod
{
	const CalcPatternMod _pmod = WideRangeBalance;
	const std::string name = "WideRangeBalanceMod";

#pragma region params

	float window_param = 2.F;

	float min_mod = 0.94F;
	float max_mod = 1.05F;
	float base = 0.425F;

	float buffer = 1.F;
	float scaler = 1.F;
	float other_scaler = 4.F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "buffer", &buffer },
		{ "scaler", &scaler },
		{ "other_scaler", &other_scaler },
	};
#pragma endregion params and param map

	int window = 0;
	float pmod = neutral;

#pragma region generic functions

	static inline void full_reset() { float pmod = neutral; }

	inline void setup(vector<float> doot[], const int& size)
	{
		// setup should be run after loading params from disk
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		// same number of taps on each column for this window
		if (itvhi.cols_equal_window(window)) {
			mod_set(_pmod, doot, i, min_mod);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo& itvhi,
						   vector<float> doot[],
						   const int& i)
	{
		if (handle_case_optimizations(itvhi, doot, i)) {
			return;
		}

		pmod = itvhi.get_col_prop_low_by_high_window(window);

		pmod = (base + (buffer + (scaler / pmod)) / other_scaler);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[_pmod][i] = pmod;
	}
};

// cj tailored anchor mod maybe thing possibly
struct WideRangeAnchorMod
{
	const CalcPatternMod _pmod = WideRangeAnchor;
	const std::string name = "WideRangeAnchorMod";

#pragma region params

	float window_param = 4.F;

	float min_mod = 1.F;
	float max_mod = 1.2F;
	float base = 1.F;

	float diff_min = 2.F;
	float diff_max = 12.F;
	float scaler = 0.1F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "diff_min", &diff_min },
		{ "diff_max", &diff_max },
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	int window = 0;
	int a = 0;
	int b = 0;
	int diff = 0;
	float divisor = diff_max - diff_min;
	float pmod = min_mod;

#pragma region generic functions

	static inline void full_reset() { float pmod = neutral; }

	inline void setup(vector<float> doot[], const int& size)
	{
		// setup should be run after loading params from disk
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
		divisor = diff_max - diff_min;

		doot[_pmod].resize(size);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline auto handle_case_optimizations(const ItvHandInfo& itvhi,
										  const AnchorSequencer& as,
										  vector<float> doot[],
										  const int& i) -> bool
	{
		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		// set max mod if either is 0
		if (itvhi.get_col_taps_nowi(col_left) == 0 ||
			itvhi.get_col_taps_nowi(col_right) == 0) {
			mod_set(_pmod, doot, i, max_mod);
			return true;
		}

		// now we need these
		a = as.get_max_for_window_and_col(col_left, window);
		b = as.get_max_for_window_and_col(col_right, window);

		// will be set for use after we return from here
		diff = diff_high_by_low(a, b);

		// difference won't matter
		if (diff <= diff_min) {
			neutral_set(_pmod, doot, i);
			return true;
		}

		// would max anyway
		if (diff > diff_max) {
			mod_set(_pmod, doot, i, max_mod);
			return true;
		}

		return false;
	}

	inline void operator()(const ItvHandInfo& itvhi,
						   const AnchorSequencer& as,
						   vector<float> doot[],
						   const int& i)
	{
		if (handle_case_optimizations(itvhi, as, doot, i)) {
			return;
		}

		pmod =
		  base + (scaler * ((static_cast<float>(diff) - diff_min) / divisor));
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[_pmod][i] = pmod;
	}
};

// find [xx]a[yy]b[zz]
struct the_slip
{
	enum to_slide_or_not_to_slide
	{
		slip_unbeginninged,
		needs_single,
		needs_23_jump,
		needs_opposing_single,
		needs_opposing_ohjump,
		slip_complete
	};

	// what caused us to slip
	unsigned slip = 0;
	// are we slipping
	bool slippin_till_ya_slips_come_true = false;
	// how far those whomst'd've been slippinging
	int slide = 0;

	// ms values, 4 ms values = 5 rows, optimize by just recycling values
	// without resetting and indexing up to the size counter to get duration
	// float ms[4] = {
	//	0.f,
	//	0.f,
	//	0.f,
	//	0.f,
	//};

	// couldn't figure out how to make slip & slide work smh
	inline auto the_slip_is_the_boot(const unsigned& notes) -> bool
	{
		switch (slide) {
			// just started, need single note with no jack between our starting
			// point and [23]
			case needs_single:
				// 1100 requires 0001
				if (slip == 3 || slip == 7) {
					if (notes == 8) {
						return true;
					}
				} else
				  // if it's not a left hand jump, it's a right hand one, we
				  // need 1000
				  if (notes == 1) {
					return true;
				}
				break;
			case needs_23_jump:
				// has to be [23]
				if (notes == 6) {
					return true;
				}
				break;
			case needs_opposing_single:
				// same as 1 but reversed

				// 1100
				// 0001
				// 0110
				// requires 1000
				if (slip == 3 || slip == 7) {
					if (notes == 1) {
						return true;
					}
				} else
				  // if it's not a left hand jump, it's a right hand one, we
				  // need 0001
				  if (notes == 8) {
					return true;
				}
				break;
			case needs_opposing_ohjump:
				if (slip == 3 || slip == 7) {
					// if we started on 1100, we end on 0011
					// make detecc more inclusive i guess by allowing 0100
					if (notes == 12 || notes == 14) {
						return true;
					}
				} else
				  // starting on 0011 ends on 1100
				  // make detecc more inclusive i guess by allowing 0010
				  if (notes == 3 || notes == 7) {
					return true;
				}
				break;
			default:
				assert(0);
				break;
		}
		return false;
	}

	inline void start(const float& ms_now, const unsigned& notes)
	{
		slip = notes;
		slide = 0;
		slippin_till_ya_slips_come_true = true;
		grow(ms_now, notes);
	}

	inline void grow(const float&  /*ms_now*/, const unsigned&  /*notes*/)
	{
		// ms[slide] = ms_now;
		++slide;
	}

	inline void reset() { slippin_till_ya_slips_come_true = false; }
};

// sort of the same concept as fj, slightly different implementation
struct TT_Sequencing
{
	the_slip fizz;
	int slip_counter = 0;
	static const int max_slips = 4;
	float mod_parts[max_slips] = { 1.F, 1.F, 1.F, 1.F };

	float scaler = 0.F;

	inline void set_params(const float&  /*gt*/, const float&  /*st*/, const float& ms)
	{
		// group_tol = gt;
		// step_tol = st;
		scaler = ms;
	}

	inline void complete_slip(const float& ms_now, const unsigned& notes)
	{
		if (slip_counter < max_slips) {
			mod_parts[slip_counter] = construct_mod_part();
		}
		++slip_counter;

		// any time we complete a slip we can start another slip, so just
		// start again
		fizz.start(ms_now, notes);
	}

	// only start if we pick up ohjump or hand with an ohjump, not a quad, not
	// singles
	static inline auto start_test(const unsigned& notes) -> bool
	{
		// either left hand jump or a hand containing left hand jump
		// or right hand jump or a hand containing right hand jump
	
		return notes == 3 || notes == 7 || notes == 12 || notes == 14;
	}

	inline void operator()(const float& ms_now, const unsigned& notes)
	{
		// ignore quads
		if (notes == 15) {
			// reset if we are in a sequence
			if (fizz.slippin_till_ya_slips_come_true) {
				fizz.reset();
			}
			return;
		}

		// haven't started
		if (!fizz.slippin_till_ya_slips_come_true) {
			// col check to start
			if (start_test(notes)) {
				fizz.start(ms_now, notes);
			}
			return;
		}
		// run the col checks for continuation
		if (fizz.the_slip_is_the_boot(notes)) {
			fizz.grow(ms_now, notes);
			// we found... the thing
			if (fizz.slide == 5) {
				complete_slip(ms_now, notes);
			}
		} else {
			// reset if we fail col check
			fizz.reset();
		}
		return;

		assert(0);
	}

	inline void reset()
	{
		slip_counter = 0;
		for (auto& v : mod_parts) {
			v = 1.F;
		}
	}

	inline auto construct_mod_part() -> float { return scaler; }
};

// the a things, they are there, we must find them...
// probably add a timing check to this as well
struct TheThingLookerFinderThing
{
	static const CalcPatternMod _pmod = TheThing;
	const std::string name = "TheThingMod";

#pragma region params

	float min_mod = 0.15F;
	float max_mod = 1.F;
	float base = 0.05F;

	// params for tt_sequencing
	float group_tol = 35.F;
	float step_tol = 17.5F;
	float scaler = 0.15F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		//{ "group_tol", &group_tol },
		//{ "step_tol", &step_tol },
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	// sequencer
	TT_Sequencing tt;
	float pmod = min_mod;

#pragma region generic functions
	inline void setup(vector<float> doot[], const int& size)
	{
		tt.set_params(group_tol, step_tol, scaler);

		doot[_pmod].resize(size);
		/*if (debug_lmao)
			for (auto& mod : _dbg)
				doot[mod].resize(size);*/
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const metaRowInfo& now)
	{
		tt(now.ms_now, now.notes);
	}

	inline void operator()(vector<float> doot[], const int& i)
	{
		pmod =
		  tt.mod_parts[0] + tt.mod_parts[1] + tt.mod_parts[2] + tt.mod_parts[3];
		pmod /= 4.F;
		pmod = CalcClamp(base + pmod, min_mod, max_mod);
		doot[_pmod][i] = pmod;

		// reset flags n stuff
		tt.reset();
	}
};

// find [12]3[24]1[34]2[13]4[12]
struct the_slip2
{
	enum to_slide_or_not_to_slide
	{
		slip_unbeginninged,
		needs_single,
		needs_door,
		needs_blaap,
		needs_opposing_ohjump,
		slip_complete
	};

	// what caused us to slip
	unsigned slip = 0;
	// are we slipping
	bool slippin_till_ya_slips_come_true = false;
	// how far those whomst'd've been slippinging
	int slide = 0;

	// ms values, 4 ms values = 5 rows, optimize by just recycling values
	// without resetting and indexing up to the size counter to get duration
	// float ms[4] = {
	//	0.f,
	//	0.f,
	//	0.f,
	//	0.f,
	//};

	// couldn't figure out how to make slip & slide work smh
	inline auto the_slip_is_the_boot(const unsigned& notes) -> bool
	{
		switch (slide) {
			// just started, need single note with no jack between our starting
			// point and [23]
			case needs_single:
				// 1100 requires 0010
				if (slip == 3) {
					if (notes == 4) {
						return true;
					}
				} else if (notes == 2) {
					return true;
				}
				break;
			case needs_door:
				if (slip == 3) {
					if (notes == 10) {
						return true;
					}
				} else if (notes == 5) {
					return true;
				}
				break;
			case needs_blaap:
				// it's alive

				// requires 1000
				if (slip == 3) {
					if (notes == 1) {
						return true;
					}
				} else if (notes == 8) {
					return true;
				}
				break;
			case needs_opposing_ohjump:
				if (slip == 3) {
					// if we started on 1100, we end on 0011
					if (notes == 12) {
						return true;
					}
				} else
				  // starting on 0011 ends on 1100
				  if (notes == 3) {
					return true;
				}
				break;
			default:
				assert(0);
				break;
		}
		return false;
	}

	inline void start(const float& ms_now, const unsigned& notes)
	{
		slip = notes;
		slide = 0;
		slippin_till_ya_slips_come_true = true;
		grow(ms_now, notes);
	}

	inline void grow(const float&  /*ms_now*/, const unsigned&  /*notes*/)
	{
		// ms[slide] = ms_now;
		++slide;
	}

	inline void reset() { slippin_till_ya_slips_come_true = false; }
};

// sort of the same concept as fj, slightly different implementation
struct TT_Sequencing2
{
	the_slip2 fizz;
	int slip_counter = 0;
	static const int max_slips = 4;
	float mod_parts[max_slips] = { 1.F, 1.F, 1.F, 1.F };

	float scaler = 0.F;

	inline void set_params(const float&  /*gt*/, const float&  /*st*/, const float& ms)
	{
		// group_tol = gt;
		// step_tol = st;
		scaler = ms;
	}

	inline void complete_slip(const float& ms_now, const unsigned& notes)
	{
		if (slip_counter < max_slips) {
			mod_parts[slip_counter] = construct_mod_part();
		}
		++slip_counter;

		// any time we complete a slip we can start another slip, so just
		// start again
		fizz.start(ms_now, notes);
	}

	// only start if we pick up ohjump or hand with an ohjump, not a quad, not
	// singles
	static inline auto start_test(const unsigned& notes) -> bool
	{
		// either left hand jump or a hand containing left hand jump
		// or right hand jump or a hand containing right hand jump
		return notes == 3 || notes == 12;
	}

	inline void operator()(const float& ms_now, const unsigned& notes)
	{
		// ignore quads
		if (notes == 15) {
			// reset if we are in a sequence
			if (fizz.slippin_till_ya_slips_come_true) {
				fizz.reset();
			}
			return;
		}

		// haven't started
		if (!fizz.slippin_till_ya_slips_come_true) {
			// col check to start
			if (start_test(notes)) {
				fizz.start(ms_now, notes);
			}
			return;
		}
		// run the col checks for continuation
		if (fizz.the_slip_is_the_boot(notes)) {
			fizz.grow(ms_now, notes);
			// we found... the thing
			if (fizz.slide == 5) {
				complete_slip(ms_now, notes);
			}
		} else {
			// reset if we fail col check
			fizz.reset();
		}
		return;

		assert(0);
	}

	inline void reset()
	{
		slip_counter = 0;
		for (auto& v : mod_parts) {
			v = 1.F;
		}
	}

	inline auto construct_mod_part() -> float { return scaler; }
};

// the a things, they are there, we must find them...
// probably add a timing check to this as well
struct TheThingLookerFinderThing2
{
	static const CalcPatternMod _pmod = TheThing2;
	const std::string name = "TheThing2Mod";

#pragma region params

	float min_mod = 0.15F;
	float max_mod = 1.F;
	float base = 0.05F;

	// params for tt2_sequencing
	float group_tol = 35.F;
	float step_tol = 17.5F;
	float scaler = 0.15F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		/*{ "group_tol", &group_tol },
		{ "step_tol", &step_tol },*/
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	// sequencer
	TT_Sequencing2 tt2;
	float pmod = min_mod;

#pragma region generic functions
	inline void setup(vector<float> doot[], const int& size)
	{
		tt2.set_params(group_tol, step_tol, scaler);

		doot[_pmod].resize(size);
		/*if (debug_lmao)
			for (auto& mod : _dbg)
				doot[mod].resize(size);*/
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : _params) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const metaRowInfo& now)
	{
		tt2(now.ms_now, now.notes);
	}

	inline void operator()(vector<float> doot[], const int& i)
	{
		pmod = tt2.mod_parts[0] + tt2.mod_parts[1] + tt2.mod_parts[2] +
			   tt2.mod_parts[3];
		pmod /= 4.F;
		pmod = CalcClamp(base + pmod, min_mod, max_mod);
		doot[_pmod][i] = pmod;

		// reset flags n stuff
		tt2.reset();
	}
};

#pragma endregion
struct TheGreatBazoinkazoinkInTheSky
{
	bool dbg = false;

	// basic data we need
	vector<float>* _doots[num_hands]{};
	vector<float>* _diffs[num_hands]{};
	vector<NoteInfo> _ni;
	vector<vector<int>> _itv_rows;
	float _rate = 0.F;
	int hand = 0;

	// to generate these

	// keeps track of occurrences of basic row based sequencing, mostly for
	// skilset detection, contains itvinfo as well, the very basic metrics used
	// for detection
	metaItvInfo _mitvi;

	// meta row info keeps track of basic pattern sequencing as we scan down
	// the notedata rows, we will recyle two pointers (we want each row to be
	// able to "look back" at the meta info generated at the last row so the mhi
	// generation requires the last generated mhi object as an arg
	unique_ptr<metaRowInfo> _last_mri;
	unique_ptr<metaRowInfo> _mri;

	// tracks meta hand info as well as basic interval tracking data for hand
	// dependent stuff, like metaitvinfo and itvinfo
	metaItvHandInfo _mitvhi;

	// meta hand info is the same as meta row info, however it tracks
	// pattern progression on individual hands rather than on generic rows
	unique_ptr<metaHandInfo> _last_mhi;
	unique_ptr<metaHandInfo> _mhi;

	// i dont want to keep doing the last swap stuff every time i add something
	// new, so just put it here and pass it
	CalcWindow<float> _mw_cc_ms_any;

	// so we can make pattern mods
	StreamMod _s;
	JSMod _js;
	HSMod _hs;
	CJMod _cj;
	CJQuadMod _cjq;
	OHJumpModGuyThing _ohj;
	RollMod _roll;
	BalanceMod _bal;
	OHTrillMod _oht;
	ChaosMod _ch;
	RunningManMod _rm;
	WideRangeJumptrillMod _wrjt;
	WideRangeRollMod _wrr;
	WideRangeBalanceMod _wrb;
	WideRangeAnchorMod _wra;
	FlamJamMod _fj;
	TheThingLookerFinderThing _tt;
	TheThingLookerFinderThing2 _tt2;

	// maybe it makes sense to move generic sequencers here
	AnchorSequencer _as;

	inline void recieve_sacrifice(const vector<NoteInfo>& ni)
	{
#ifndef RELWITHDEBINFO
#if NDEBUG
		load_calc_params_from_disk();
#endif
#endif
		// ok so the problem atm is the multithreading of songload, if we
		// want to update the file on disk with new values and not just
		// overwrite it we have to write out after loading the values player
		// defined, so the quick hack solution to do that is to only do it
		// during debug output generation, which is fine for the time being,
		// though not ideal
		if (debug_lmao) {
			write_params_to_disk();
		}

		// setup our data pointers
		_last_mri = std::make_unique<metaRowInfo>();
		_mri = std::make_unique<metaRowInfo>();
		_last_mhi = std::make_unique<metaHandInfo>();
		_mhi = std::make_unique<metaHandInfo>();

		// doesn't change with offset or anything, and we may do
		// multi-passes at some point
		_ni = ni;
	}

	// for cj, will be sorted from teh above, but we dont care
	static inline auto CalcMSEstimateTWOOOOO(const vector<float>& input)
	  -> float
	{
		if (input.empty()) {
			return 1.F;
		}

		float looloo = mean(input);
		float doodoo = ms_to_bpm(looloo);
		float trootroo = doodoo / 15.F;
		return trootroo * finalscaler;
	}

	inline void heres_my_diffs(vector<float> lsoap[], vector<float> rsoap[])
	{
		_diffs[lh] = lsoap;
		_diffs[rh] = rsoap;
	}

	inline void operator()(const vector<vector<int>>& itv_rows,
						   const float& rate,
						   vector<float> ldoot[],
						   vector<float> rdoot[])
	{
		// set interval/offset pass specific stuff
		_doots[lh] = ldoot;
		_doots[rh] = rdoot;
		_itv_rows = itv_rows;
		_rate = rate;

		run_agnostic_pmod_loop();
		run_dependent_pmod_loop();
	}

#pragma region hand agnostic pmod loop
	inline void advance_agnostic_sequencing()
	{
		_fj.advance_sequencing(*_mri);
		_tt.advance_sequencing(*_mri);
		_tt2.advance_sequencing(*_mri);
	}
	inline void setup_agnostic_pmods()
	{
		// these pattern mods operate on all columns, only need basic meta
		// interval data, and do not need any more advanced pattern
		// sequencing
		for (auto& a : _doots) {
			_s.setup(a, _itv_rows.size());
			_js.setup(a, _itv_rows.size());
			_hs.setup(a, _itv_rows.size());
			_cj.setup(a, _itv_rows.size());
			_cjq.setup(a, _itv_rows.size());
			_fj.setup(a, _itv_rows.size());
			_tt.setup(a, _itv_rows.size());
			_tt2.setup(a, _itv_rows.size());
		}
	}

	inline void set_agnostic_pmods(vector<float> doot[], const int& itv)
	{
		// these pattern mods operate on all columns, only need basic meta
		// interval data, and do not need any more advanced pattern
		// sequencing just set only one hand's values and we'll copy them
		// over (or figure out how not to need to) later
		_s(_mitvi, doot);
		_js(_mitvi, doot);
		_hs(_mitvi, doot);
		_cj(_mitvi, doot);
		_cjq(_mitvi, doot);
		_fj(doot, itv);
		_tt(doot, itv);
		_tt2(doot, itv);
	}

	inline void run_agnostic_smoothing_pass(vector<float> doot[])
	{
		Smooth(doot[_s._pmod], neutral);
		Smooth(doot[_js._pmod], neutral);
		Smooth(doot[_js._pmod], neutral);
		Smooth(doot[_hs._pmod], neutral);
		Smooth(doot[_cj._pmod], neutral);
		Smooth(doot[_cjq._pmod], neutral);
		Smooth(doot[_fj._pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing::_pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing::_pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing2::_pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing2::_pmod], neutral);
	}

	inline void bruh_they_the_same()
	{
		_doots[1][_s._pmod] = _doots[0][_s._pmod];
		_doots[1][_js._pmod] = _doots[0][_js._pmod];
		_doots[1][_hs._pmod] = _doots[0][_hs._pmod];
		_doots[1][_cj._pmod] = _doots[0][_cj._pmod];
		_doots[1][_cjq._pmod] = _doots[0][_cjq._pmod];
		_doots[1][_fj._pmod] = _doots[0][_fj._pmod];
		_doots[1][TheThingLookerFinderThing::_pmod] =
		  _doots[0][TheThingLookerFinderThing::_pmod];
		_doots[1][TheThingLookerFinderThing::_pmod] =
		  _doots[0][TheThingLookerFinderThing::_pmod];
		_doots[1][TheThingLookerFinderThing2::_pmod] =
		  _doots[0][TheThingLookerFinderThing2::_pmod];
		_doots[1][TheThingLookerFinderThing2::_pmod] =
		  _doots[0][TheThingLookerFinderThing2::_pmod];
	}

	inline void run_agnostic_pmod_loop()
	{
		setup_agnostic_pmods();

		// don't use s_init here, we know the first row is always 0.f and
		// therefore the first interval starts at 0.f (unless we do offset
		// passes but that's for later)
		float row_time = 0.F;
		int row_count = 0;
		unsigned row_notes = 0;

		// boop
		for (int itv = 0; itv < _itv_rows.size(); ++itv) {
			// reset any accumulated interval info and set cur index number
			_mitvi.reset(itv);

			// run the row by row construction for interval info
			for (auto& row : _itv_rows[itv]) {
				row_time = _ni[row].rowTime / _rate;
				row_notes = _ni[row].notes;
				row_count = column_count(row_notes);

				(*_mri)(*_last_mri, _mitvi, row_time, row_count, row_notes);

				advance_agnostic_sequencing();

				// we only need to look back 1 metanoterow object, so we can
				// swap the one we just built into last and recycle the two
				// pointers instead of keeping track of everything
				swap(_mri, _last_mri);
			}

			// run pattern mod generation for hand agnostic mods
			set_agnostic_pmods(_doots[lh], itv);
		}
		run_agnostic_smoothing_pass(_doots[lh]);
		bruh_they_the_same();
	}
#pragma endregion

#pragma region hand dependent pmod loop
	// some pattern mod detection builds across rows, see rm_sequencing for
	// an example, actually all sequencing should be done in objects
	// following rm_sequencing's template and be stored in mhi, and then
	// passed to whichever mods need them, but that's for later
	inline void handle_row_dependent_pattern_advancement()
	{
		_ohj.advance_sequencing(*_mhi);
		RollMod::advance_sequencing(*_mhi);
		_oht.advance_sequencing(*_mhi, _mw_cc_ms_any);
		_rm.advance_sequencing(*_mhi);
		_wrr.advance_sequencing(*_mhi);
		_wrjt.advance_sequencing(*_mhi);
		_ch.advance_sequencing(_mw_cc_ms_any);
	}

	inline void setup_dependent_mods(vector<float> _doot[])
	{
		_ohj.setup(_doot, _itv_rows.size());
		_bal.setup(_doot, _itv_rows.size());
		_roll.setup(_doot, _itv_rows.size());
		_oht.setup(_doot, _itv_rows.size());
		_ch.setup(_doot, _itv_rows.size());
		_rm.setup(_doot, _itv_rows.size());
		_wrr.setup(_doot, _itv_rows.size());
		_wrjt.setup(_doot, _itv_rows.size());
		_wrb.setup(_doot, _itv_rows.size());
		_wra.setup(_doot, _itv_rows.size());
	}

	inline void set_dependent_pmods(vector<float> doot[], const int& itv)
	{
		_ohj(_mitvhi, doot, itv);
		_bal(_mitvhi._itvhi, doot, itv);
		_roll(_mitvhi, doot, itv);
		_oht(_mitvhi._itvhi, doot, itv);
		_ch(_mitvhi._itvhi, doot, itv);
		_rm(doot, itv);
		_wrr(_mitvhi._itvhi, doot, itv);
		_wrjt(_mitvhi._itvhi, doot, itv);
		_wrb(_mitvhi._itvhi, doot, itv);
		_wra(_mitvhi._itvhi, _as, doot, itv);
	}

	inline void run_dependent_smoothing_pass(vector<float> doot[])
	{
		// need to split upohj and cjohj into 2 pmod objects
		Smooth(doot[_ohj._pmod], neutral);
		Smooth(doot[_bal._pmod], neutral);
		Smooth(doot[_roll._pmod], neutral);
		Smooth(doot[_oht._pmod], neutral);
		Smooth(doot[_ch._pmod], neutral);
		Smooth(doot[_rm._pmod], neutral);
		Smooth(doot[_wrr._pmod], neutral);
		Smooth(doot[_wrjt._pmod], neutral);
		Smooth(doot[_wrb._pmod], neutral);
		Smooth(doot[_wra._pmod], neutral);
	}

	// reset any moving windows or values when starting the other hand, this
	// shouldn't matter too much practically, but we should be disciplined
	// enough to do it anyway
	inline void full_hand_reset()
	{
		_ohj.full_reset();
		_bal.full_reset();
		_roll.full_reset();
		_oht.full_reset();
		_ch.full_reset();
		_rm.full_reset();
		_wrr.full_reset();
		_wrjt.full_reset();
		WideRangeBalanceMod::full_reset();
		WideRangeAnchorMod::full_reset();

		// zero out moving windows at the start of each hand
		_mw_cc_ms_any.zero();

		_mitvhi.zero();
		_mhi->full_reset();
		_last_mhi->full_reset();
	}

	inline void handle_dependent_interval_end(const int& itv)
	{
		// invoke metaintervalhandinfo interval end FUNCTION
		_mitvhi.interval_end();

		// test putting generic sequencers here
		_as.handle_interval_end();

		// run pattern mod generation for hand dependent mods
		set_dependent_pmods(_doots[hand], itv);
	}

	inline void run_dependent_pmod_loop()
	{
		float row_time = 0.F;
		int row_count = 0;
		int last_row_count = 0;
		int last_last_row_count = 0;
		unsigned row_notes = 0;
		col_type ct = col_init;

		full_hand_reset();

		for (auto& ids : hand_col_ids) {
			setup_dependent_mods(_doots[hand]);

			// so we are technically doing this again (twice) and don't to
			// be doing it, but it makes debugging much less of a pita if we
			// aren't doing something like looping over intervals, running
			// agnostic pattern mods, then looping over hands for dependent
			// mods in the same interval, we may still want to do that or at
			// least have an optional set for that in case a situation
			// arises where something might need both types of info (we'd
			// also need to have 2 itvhandinfo objects, or just for general
			// performance (though the redundancy on this pass vs agnostic
			// the pass is limited to like... a couple floats and 2 ints)
			vector<float> the_simpsons;
			for (int itv = 0; itv < _itv_rows.size(); ++itv) {
				the_simpsons.clear();

				// run the row by row construction for interval info
				for (auto& row : _itv_rows[itv]) {
					row_time = _ni[row].rowTime / _rate;
					row_notes = _ni[row].notes;
					row_count = column_count(row_notes);

					ct = determine_col_type(row_notes, ids);

					// log offhand tap info (could be more performance and
					// information efficient)
					if (ct == col_empty) {

						// think itvhi wants this as well as mhi
						++_mitvhi._itvhi._offhand_taps;
						++_mhi->offhand_taps;

						if (column_count(row_notes) == 2) {
							++_mhi->offhand_ohjumps;
							++_mhi->offhand_taps;

							++_mitvhi._itvhi._offhand_taps;
						}
					}

					// only do anything else for rows with actual stuff on this
					// hand, especially the swap
					if (ct == col_empty) {
						continue;
					}

					_as(ct, row_time);

					(*_mhi)(*_last_mhi, _mw_cc_ms_any, row_time, ct, row_notes);

					if ((last_row_count > 1 && row_count > 1) ||
						(last_row_count > 1 && last_last_row_count > 1)) {
						the_simpsons.push_back(
						  max(75.F, min(_mhi->cc_ms_any, _mhi->tc_ms)));
					}

					last_last_row_count = row_count;
					last_row_count = row_count;

					_mitvhi._itvhi.set_col_taps(ct);

					if (ct != col_init) {
						++_mitvhi._cc_types[_mhi->cc];
						++_mitvhi._meta_types[_mhi->mt];
					}

					handle_row_dependent_pattern_advancement();

					std::swap(_last_mhi, _mhi);
					_mhi->offhand_ohjumps = 0;
					_mhi->offhand_taps = 0;
				}

				handle_dependent_interval_end(itv);

				_diffs[hand][BaseMS][itv] = CalcMSEstimateTWOOOOO(the_simpsons);
			}
			run_dependent_smoothing_pass(_doots[hand]);
			DifficultyMSSmooth(_diffs[hand][BaseMS]);

			// ok this is pretty jank LOL, just increment the hand index
			// when we finish left hand
			++hand;
		}
	}
#pragma endregion

	inline void load_calc_params_from_disk()
	{
		std::string fn = calc_params_xml;
		int iError;
		std::unique_ptr<RageFileBasic> pFile(
		  FILEMAN->Open(fn, RageFile::READ, iError));
		if (pFile == nullptr) {
			return;
		}

		XNode params;
		if (!XmlFileUtil::LoadFromFileShowErrors(params, *pFile)) {
			return;
		}

		// ignore params from older versions
		std::string vers;
		params.GetAttrValue("vers", vers);
		if (vers.empty() || stoi(vers) != GetCalcVersion()) {
			return;
		}

		_s.load_params_from_node(&params);
		_js.load_params_from_node(&params);
		_hs.load_params_from_node(&params);
		_cj.load_params_from_node(&params);
		_cjq.load_params_from_node(&params);
		_ohj.load_params_from_node(&params);
		_bal.load_params_from_node(&params);
		_roll.load_params_from_node(&params);
		_oht.load_params_from_node(&params);
		_ch.load_params_from_node(&params);
		_rm.load_params_from_node(&params);
		_wrr.load_params_from_node(&params);
		_wrjt.load_params_from_node(&params);
		_wrb.load_params_from_node(&params);
		_wra.load_params_from_node(&params);
		_fj.load_params_from_node(&params);
		_tt.load_params_from_node(&params);
		_tt2.load_params_from_node(&params);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* calcparams = new XNode("CalcParams");
		calcparams->AppendAttr("vers", GetCalcVersion());

		calcparams->AppendChild(_s.make_param_node());
		calcparams->AppendChild(_js.make_param_node());
		calcparams->AppendChild(_hs.make_param_node());
		calcparams->AppendChild(_cj.make_param_node());
		calcparams->AppendChild(_cjq.make_param_node());
		calcparams->AppendChild(_ohj.make_param_node());
		calcparams->AppendChild(_bal.make_param_node());
		calcparams->AppendChild(_roll.make_param_node());
		calcparams->AppendChild(_oht.make_param_node());
		calcparams->AppendChild(_ch.make_param_node());
		calcparams->AppendChild(_rm.make_param_node());
		calcparams->AppendChild(_wrr.make_param_node());
		calcparams->AppendChild(_wrjt.make_param_node());
		calcparams->AppendChild(_wrb.make_param_node());
		calcparams->AppendChild(_wra.make_param_node());
		calcparams->AppendChild(_fj.make_param_node());
		calcparams->AppendChild(_tt.make_param_node());
		calcparams->AppendChild(_tt2.make_param_node());

		return calcparams;
	}
#pragma endregion

	inline void write_params_to_disk()
	{
		std::string fn = calc_params_xml;
		std::unique_ptr<XNode> xml(make_param_node());

		std::string err;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE)) {
			return;
		}
		XmlFileUtil::SaveToFile(xml.get(), f, "", false);
	}
};

auto
Calc::InitializeHands(const vector<NoteInfo>& NoteInfo,
					  float music_rate,
					  float offset) -> bool
{
	numitv = static_cast<int>(
	  std::ceil(NoteInfo.back().rowTime / (music_rate * IntervalSpan)));

	bool junk_file_mon = false;
	ProcessedFingers fingers;
	for (auto t : zto3) {
		fingers.emplace_back(
		  ProcessFinger(NoteInfo, t, music_rate, offset, junk_file_mon));

		// don't bother with this file
		if (junk_file_mon) {
			return false;
		}
	}

	// sequence jack immediately so we can ref pass & sort in calc
	// msestimate without things going be wackying
	for (auto m : zto3) {
		jacks[m]->resize(4);
		for (auto t : zto3) {
			SequenceJack(fingers[t], t, m);

			// resize stam adjusted jack vecs, technically if we flattened
			// the vector we could allocate only once for all rate passes
			// when doing caching, but for various other reasons it was
			// easier to keep them split by intervals in a double vector,
			// this should maybe be changed?
			stam_adj_jacks[t].resize(fingers[t].size());
			for (int i = 0; i < fingers[t].size(); ++i) {
				stam_adj_jacks[t][i].resize(fingers[t][i].size());
			}
		}
	}

	pair<Hand&, vector<int>> spoopy[2] = { { left_hand, { 1, 2 } },
										   { right_hand, { 4, 8 } } };

	TheGreatBazoinkazoinkInTheSky ulbu_that_which_consumes_all;
	ulbu_that_which_consumes_all.recieve_sacrifice(NoteInfo);

	// loop to help with hand specific stuff, we could do this stuff
	// in the class but that's more structural work and this is
	// simple
	for (auto& hp : spoopy) {
		auto& hand = hp.first;
		const auto& fv = hp.second;

		// these definitely do change with every chisel test
		hand.stam_adj_diff.resize(numitv);
	}

	// do these last since calcmsestimate modifies the interval ms values of
	// fingers with sort, anything that is derivative of those values that
	// requires them to be in sequential order should be done before this
	// point
	left_hand.InitBaseDiff(fingers[0], fingers[1]);
	left_hand.InitPoints(fingers[0], fingers[1]);

	right_hand.InitBaseDiff(fingers[2], fingers[3]);
	right_hand.InitPoints(fingers[2], fingers[3]);

	ulbu_that_which_consumes_all.heres_my_diffs(left_hand.soap,
												right_hand.soap);

	ulbu_that_which_consumes_all(
	  nervIntervals, music_rate, left_hand.doot, right_hand.doot);

	left_hand.InitAdjDiff();
	right_hand.InitAdjDiff();
	// Smooth(left_hand.base_adj_diff[Skill_Jumpstream], 1.f);
	// Smooth(right_hand.base_adj_diff[Skill_Jumpstream], 1.f);
	// debug info loop
	if (debugmode) {
		for (auto& hp : spoopy) {
			auto& hand = hp.first;

			// pattern mods and base msd never change, set degbug output
			// for them now

			// 3 = number of different debug types
			hand.debugValues.resize(3);
			hand.debugValues[0].resize(ModCount);
			hand.debugValues[1].resize(NUM_CalcDiffValue);
			hand.debugValues[2].resize(NUM_CalcDebugMisc);

			for (int i = 0; i < ModCount; ++i) {
				hand.debugValues[0][i] = hand.doot[i];
			}

			// set everything but final adjusted output here
			for (int i = 0; i < NUM_CalcDiffValue - 1; ++i) {
				hand.debugValues[1][i] = hand.soap[i];
			}
		}
	}
	return true;
}

auto
Hand::CalcMSEstimate(vector<float>& input, const int& burp) -> float
{
	static const bool dbg = false;

	// how many ms values we use from here, if there are fewer than this
	// number we'll mock up some values to water down intervals with a
	// single extremely fast minijack, if there are more, we will truncate
	unsigned int num_used = burp;

	if (input.empty()) {
		return 0.F;
	}

	// avoiding this for now because of smoothing
	// single ms value, dunno if we want to do this? technically the tail
	// end of an insanely hard burst that gets lopped off at the last note
	// is still hard? if (input.size() < 2) return 1.f;

	// sort before truncating/filling
	sort(input.begin(), input.end());

	// truncate if we have more values than what we care to sample, we're
	// looking for a good estimate of the hardest part of this interval
	// if above 1 and below used_ms_vals, fill up the stuff with dummies
	// my god i was literally an idiot for doing what i was doing before
	static const float ms_dummy = 360.F;

	// mostly try to push down stuff like jumpjacks, not necessarily to push
	// up "complex" stuff (this will push up intervals with few fast ms
	// values kinda hard but it shouldn't matter as their base ms diff
	// should be extremely low
	float cv_yo = cv_trunc_fill(input, burp, ms_dummy) + 0.5F;
	cv_yo = CalcClamp(cv_yo, 0.5F, 1.25F);

	if (dbg && debug_lmao) {
		std::string moop;
		for (auto& v : input) {
			moop.append(std::to_string(v));
			moop.append(", ");
		}

		std::cout << "notes: " << moop << std::endl;
	}

	if (dbg && debug_lmao) {
		std::cout << "cv : " << cv_yo << std::endl;
	}

	// basically doing a jank average, bigger m = lower difficulty
	float m = sum_trunc_fill(input, burp, ms_dummy);

	if (dbg && debug_lmao) {
		std::cout << "m : " << m << std::endl;
	}

	// add 1 to num_used because some meme about sampling
	// same thing as jack stuff, convert to bpm and then nps
	float bpm_est = ms_to_bpm(m / (num_used + 1));
	float nps_est = bpm_est / 15.F;
	float fdiff = nps_est * cv_yo;
	if (dbg && debug_lmao) {
		std::cout << "diff : " << fdiff << std::endl;
	}
	return fdiff;
}

void
Hand::InitBaseDiff(Finger& f1, Finger& f2)
{
	static const bool dbg = false;

	for (int i = 0; i < NUM_CalcDiffValue - 1; ++i) {
		soap[i].resize(f1.size());
	}

	for (int i = 0; i < f1.size(); i++) {

		if (dbg && debug_lmao) {
			std::cout << "\ninterval : " << i << std::endl;
		}

		// scaler for things with higher things
		static const float higher_thing_scaler = 1.175F;
		float nps = 1.6F * static_cast<float>(f1[i].size() + f2[i].size());

		auto do_diff_thingy = [this](vector<float>& input,
									 const float& scaler) {
			float mwerp = CalcMSEstimate(input, 3);
			if (input.size() > 3) {
				mwerp = max(mwerp, CalcMSEstimate(input, 4) * scaler);
			}
			if (input.size() > 4) {
				mwerp = max(mwerp, CalcMSEstimate(input, 5) * scaler * scaler);
			}
			return mwerp;
		};

		float left_diff = do_diff_thingy(f1[i], higher_thing_scaler);
		float right_diff = do_diff_thingy(f1[i], higher_thing_scaler);

		float difficulty = 0.F;
		float squiggly_line = 5.5F;
		if (left_diff > right_diff) {
			difficulty =
			  weighted_average(left_diff, right_diff, squiggly_line, 9.F);
		} else {
			difficulty =
			  weighted_average(right_diff, left_diff, squiggly_line, 9.F);
		}
		soap[BaseNPS][i] = finalscaler * nps;
		soap[BaseMS][i] = finalscaler * difficulty;
		soap[BaseMSD][i] =
		  weighted_average(difficulty, nps, 7.76445F, 10.F) * finalscaler;
	}
	Smooth(soap[BaseNPS], 0.F);
	DifficultyMSSmooth(soap[BaseMS]);
	DifficultyMSSmooth(soap[BaseMSD]);
}

// each skillset should just be a separate calc function [todo]
auto
Calc::Chisel(float player_skill,
			 float resolution,
			 float score_goal,
			 int ss,
			 bool stamina,
			 bool debugoutput) -> float
{

	float gotpoints = 0.F;
	float reqpoints = static_cast<float>(MaxPoints) * score_goal;
	float max_points_lost = static_cast<float>(MaxPoints) - reqpoints;
	float jloss = 0.F;
	for (int iter = 1; iter <= 8; iter++) {
		do {
			if (player_skill > 100.F) {
				return player_skill;
			}
			player_skill += resolution;
			if (ss == Skill_Overall || ss == Skill_Stamina) {
				return 0.F; // not how we set these values
			}

			// reset tallied score, always deduct rather than accumulate now
			gotpoints = static_cast<float>(MaxPoints);
//#define DEBUG_JACK_MODELS
#ifdef DEBUG_JACK_MODELS
				if (ss == Skill_Jumpstream) {
					left_hand.CalcInternal(
					  gotpoints, player_skill, ss, stamina);
					right_hand.CalcInternal(
					  gotpoints, player_skill, ss, stamina);
				}

				if (ss == Skill_JackSpeed)
					gotpoints -=
					  JackLoss(player_skill, 1, max_points_lost, stamina);
				else if (ss == Skill_Chordjack)
					gotpoints -=
					  JackLoss(player_skill, 2, max_points_lost, stamina);
				else if (ss == Skill_Technical)
					gotpoints -=
					  JackLoss(player_skill, 3, max_points_lost, stamina);
				else if (ss == Skill_Stream)
					gotpoints -=
					  JackLoss(player_skill, 0, max_points_lost, stamina) /
					  7.5f;
#else
				// jack sequencer point loss for jack speed and (maybe?)
				// cj
				if (ss == Skill_JackSpeed) {
					// this is slow but gives the best results, do separate
					// passes for different jack types and figure out which
					// is the most prominent of the file. We _don't_ want to
					// do something like take the highest of a given type at
					// multiple points throughout a file, that just results
					// in oversaturation and bad grouping
					jloss = max(
					  JackLoss(player_skill, 1, max_points_lost, stamina),
					  max(JackLoss(player_skill, 2, max_points_lost, stamina),
						  JackLoss(player_skill, 3, max_points_lost, stamina)));
					gotpoints -= jloss;
				} else {
					left_hand.CalcInternal(
					  gotpoints, player_skill, ss, stamina);

					// already can't reach goal, move on
					if (gotpoints > reqpoints) {
						right_hand.CalcInternal(
						  gotpoints, player_skill, ss, stamina);
					}
				}
#endif
		} while (gotpoints < reqpoints);
		player_skill -= resolution;
		resolution /= 2.F;
	}

	// these are the values for msd/stam adjusted msd/pointloss the
	// latter two are dependent on player_skill and so should only
	// be recalculated with the final value already determined
	// getting the jackstam debug output right is lame i know
	if (debugoutput) {
		float jl1 =
		  JackLoss(player_skill, 1, max_points_lost, stamina, debugoutput);
		float jl2 =
		  JackLoss(player_skill, 2, max_points_lost, stamina, debugoutput);
		float jl3 =
		  JackLoss(player_skill, 3, max_points_lost, stamina, debugoutput);
		if (jl1 > jl2 && jl1 > jl3) {
			JackLoss(player_skill, 1, max_points_lost, stamina, debugoutput);
		} else if (jl2 > jl3) {
			JackLoss(player_skill, 2, max_points_lost, stamina, debugoutput);
		} else {
			JackLoss(player_skill, 3, max_points_lost, stamina, debugoutput);
		}

		left_hand.CalcInternal(
		  gotpoints, player_skill, ss, stamina, debugoutput);
		right_hand.CalcInternal(
		  gotpoints, player_skill, ss, stamina, debugoutput);
	}

	return player_skill + 2.F * resolution;
}

void
Hand::InitAdjDiff()
{
	// new plan stop being dumb and doing this over and over again
	// in calc internal because these values never change

	// the new way we wil attempt to diffrentiate skillsets rather
	// than using normalizers is by detecting whether or not we
	// think a file is mostly comprised of a given pattern,
	// producing a downscaler that slightly buffs up those files and
	// produces a downscaler for files not detected of that type.
	// the major potential failing of this system is that it ends up
	// such that the rating is tied directly to whether or not a
	// file can be more or less strongly determined to be of a
	// pattern type, e.g. splithand trills being marked as more "js"
	// than actual js, for the moment these modifiers are still
	// built on proportion of taps in chords / total taps, but
	// there's a lot more give than their used to be. they should be
	// re-done as sequential detection for best effect but i don't
	// know if that will be necessary for basic tuning if we don't
	// do this files may end up misclassing hard and polluting
	// leaderboards, and good scores on overrated files will simply
	// produce high ratings in every category

	static const vector<int> pmods_used[NUM_Skillset] = {
		// overall, nothing, don't handle here
		{},

		// stream
		{
		  Stream,
		  OHTrill,
		  Roll,
		  Chaos,
		  WideRangeRoll,
		  WideRangeJumptrill,
		  FlamJam,
		  OHJumpMod,
		  Balance,
		},

		// js
		{
		  JS,
		  OHJumpMod,
		  Chaos,
		  Balance,
		  TheThing,
		  TheThing2,
		  WideRangeBalance,
		  WideRangeJumptrill,
		  WideRangeRoll,
		  OHTrill,
		},

		// hs
		{
		  HS,
		  OHJumpMod,
		  TheThing,
		  WideRangeAnchor,
		  WideRangeRoll,
		  OHTrill,
		},

		// stam, nothing, don't handle here
		{},

		// jackspeed, ignore for now
		{},

		// chordjack
		{ CJ, CJQuad, WideRangeAnchor },

		// tech, duNNO wat im DOIN
		{
		  OHTrill,
		  Balance,
		  Roll,
		  OHJumpMod,
		  Chaos,
		  WideRangeJumptrill,
		  // WideRangeBalance,
		  WideRangeRoll,
		  FlamJam,
		  RanMan,
		  // WideRangeAnchor,
		  TheThing,
		  TheThing2,
		},

	};

	for (int i = 0; i < NUM_Skillset; ++i) {
		base_adj_diff[i].resize(soap[BaseNPS].size());
		base_diff_for_stam_mod[i].resize(soap[BaseNPS].size());
	}

	// ok this loop is pretty wack i know, for each interval
	for (int i = 0; i < soap[BaseNPS].size(); ++i) {
		float tp_mods[NUM_Skillset] = {
			1.F, 1.F, 1.F, 1.F, 1.F, 1.F, 1.F, 1.F
		};

		// total pattern mods for each skillset, we want this to be
		// calculated before the main skillset loop because we might
		// want access to the total js mod while on stream, or
		// something
		for (int ss = 0; ss < NUM_Skillset; ++ss) {
			// is this even faster than multiplying 1.f by 1.f a
			// billion times?
			if (ss == Skill_Overall || ss == Skill_Stamina) {
				continue;
			}
			for (auto& pmod : pmods_used[ss]) {
				tp_mods[ss] *= doot[pmod][i];
			}
		}

		// main skillset loop, for each skillset that isn't overall
		// or stam
		for (int ss = 0; ss < NUM_Skillset; ++ss) {
			if (ss == Skill_Overall || ss == Skill_Stamina) {
				continue;
			}

			// this should work and not be super slow?
			auto& adj_diff = base_adj_diff[ss][i];
			auto& stam_base = base_diff_for_stam_mod[ss][i];

			// might need optimization, or not since this is not
			// outside of a dumb loop now and is done once instead
			// of a few hundred times
			float funk = soap[BaseNPS][i] * tp_mods[ss] * basescalers[ss];
			adj_diff = funk;
			stam_base = funk;
			switch (ss) {
				// do funky special case stuff here
				case Skill_Stream:
					adj_diff *=
					  CalcClamp(fastsqrt(doot[RanMan][i] - 0.125F), 1.F, 1.05F);
					break;

				// test calculating stam for js/hs on max js/hs diff
				// we want hs to count against js so they are
				// mutually exclusive
				case Skill_Jumpstream:
					adj_diff /= max(doot[HS][i], 1.F);
					adj_diff *= CalcClamp(
					  fastsqrt(doot[RanMan][i] - 0.15F), 0.99F, 1.04F);
					adj_diff /= fastsqrt(doot[OHJumpMod][i] * 0.95F);
					adj_diff /= fastsqrt(doot[WideRangeRoll][i]);
					adj_diff *= fastsqrt(doot[WideRangeAnchor][i]);
					/*adj_diff *=
					  CalcClamp(fastsqrt(doot[RanMan][i] - 0.2f), 1.f, 1.05f);*/
					// maybe we should have 2 loops to avoid doing
					// math twice
					stam_base =
					  max(funk, soap[BaseNPS][i] * tp_mods[Skill_Handstream]);
					break;
				case Skill_Handstream:
					// adj_diff /= fastsqrt(doot[OHJump][i]);
					stam_base =
					  max(funk, soap[BaseNPS][i] * tp_mods[Skill_Jumpstream]);
					break;
				case Skill_Chordjack:
					adj_diff =
					  soap[BaseMS][i] * doot[CJ][i] * tp_mods[Skill_Chordjack] *
					  basescalers[ss] *
					  CalcClamp(fastsqrt(doot[OHJumpMod][i]) + 0.06F, 0.F, 1.F);
					break;
				case Skill_Technical:
					adj_diff =
					  soap[BaseMSD][i] * tp_mods[ss] * basescalers[ss] /
					  max(fastpow(doot[CJ][i], 2.F), 1.F) *
					  max(max(doot[Stream][i], doot[JS][i]), doot[HS][i]) *
					  doot[Chaos][i] * fastsqrt(doot[RanMan][i]);
					break;
				default:
					break;
			}
		}
	}
}

// debug bool here is NOT the one in Calc, it is passed from chisel
// using the final difficulty as the starting point and should only
// be executed once per chisel
void
Hand::CalcInternal(float& gotpoints, float& x, int ss, bool stam, bool debug)
{

	if (stam) {
		StamAdjust(x, ss);
	}

	// final difficulty values to use
	const vector<float>& v = stam ? stam_adj_diff : base_adj_diff[ss];
	float powindromemordniwop = 1.7F;
	if (ss == Skill_Chordjack) {
		powindromemordniwop = 1.7F;
	}

	// i don't like the copypasta either but the boolchecks where
	// they were were too slow
	if (debug) {
		debugValues[2][StamMod].resize(v.size());
		debugValues[2][PtLoss].resize(v.size());
		// final debug output should always be with stam activated
		StamAdjust(x, ss, true);
		debugValues[1][MSD] = stam_adj_diff;

		for (int i = 0; i < v.size(); ++i) {
			if (x < v[i]) {
				auto pts = static_cast<float>(v_itvpoints[i]);
				float lostpoints =
				  (pts - (pts * fastpow(x / v[i], powindromemordniwop)));
				gotpoints -= lostpoints;
				debugValues[2][PtLoss][i] = abs(lostpoints);
			}
		}
	} else {
		for (int i = 0; i < v.size(); ++i) {
			if (x < v[i]) {
				auto pts = static_cast<float>(v_itvpoints[i]);
				gotpoints -=
				  (pts - (pts * fastpow(x / v[i], powindromemordniwop)));
			}
		}
	}
}

inline void
Hand::StamAdjust(float x, int ss, bool debug)
{
	float stam_floor =
	  0.95F;		   // stamina multiplier min (increases as chart advances)
	float mod = 0.95F; // mutliplier

	float avs1 = 0.F;
	float avs2 = 0.F;
	float local_ceil = stam_ceil;
	const float super_stam_ceil = 1.11F;

	// use this to calculate the mod growth
	const auto& base_diff = base_diff_for_stam_mod[ss];
	// but apply the mod growth to these values
	// they might be the same, or not
	const auto& diff = base_adj_diff[ss];

	// i don't like the copypasta either but the boolchecks where
	// they were were too slow
	if (debug) {
		for (int i = 0; i < base_diff.size(); i++) {
			avs1 = avs2;
			avs2 = base_diff[i];
			mod += ((((avs1 + avs2) / 2.F) / (stam_prop * x)) - 1.F) / stam_mag;
			if (mod > 0.95F) {
				stam_floor += (mod - 0.95F) / stam_fscale;
			}
			local_ceil = stam_ceil * stam_floor;

			mod = min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
			stam_adj_diff[i] = diff[i] * mod;
			debugValues[2][StamMod][i] = mod;
		}
	} else {
		for (int i = 0; i < base_diff.size(); i++) {
			avs1 = avs2;
			avs2 = base_diff[i];
			mod += ((((avs1 + avs2) / 2.F) / (stam_prop * x)) - 1.F) / stam_mag;
			if (mod > 0.95F) {
				stam_floor += (mod - 0.95F) / stam_fscale;
			}
			local_ceil = stam_ceil * stam_floor;

			mod = min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
			stam_adj_diff[i] = diff[i] * mod;
		}
	}
}
#pragma endregion

static const float ssr_goal_cap = 0.965F; // goal cap to prevent insane scaling
#pragma region thedoots
// Function to generate SSR rating
auto
MinaSDCalc(const vector<NoteInfo>& NoteInfo, float musicrate, float goal)
  -> vector<float>
{
	if (NoteInfo.size() <= 1) {
		return dimples_the_all_zero_output;
	}
	return std::make_unique<Calc>()->CalcMain(
	  NoteInfo, musicrate, min(goal, ssr_goal_cap));
}

// Wrap difficulty calculation for all standard rates
auto
MinaSDCalc(const vector<NoteInfo>& NoteInfo) -> MinaSD
{
	MinaSD allrates;
	int lower_rate = 7;
	int upper_rate = 21;

	if (NoteInfo.size() > 1) {
		std::unique_ptr<Calc> cacheRun = std::make_unique<Calc>();
		cacheRun->ssr = false;
		for (int i = lower_rate; i < upper_rate; i++) {
			allrates.emplace_back(cacheRun->CalcMain(
			  NoteInfo, static_cast<float>(i) / 10.F, 0.93F));
		}
	} else {
		for (int i = lower_rate; i < upper_rate; i++) {
			allrates.emplace_back(dimples_the_all_zero_output);
		}
	}
	return allrates;
}

// Debug output
void
MinaSDCalcDebug(const vector<NoteInfo>& NoteInfo,
				float musicrate,
				float goal,
				vector<vector<vector<vector<float>>>>& handInfo)
{
	if (NoteInfo.size() <= 1) {
		return;
	}

	std::unique_ptr<Calc> debugRun = std::make_unique<Calc>();
	debugRun->debugmode = true;
	debugRun->CalcMain(NoteInfo, musicrate, min(goal, ssr_goal_cap));

	handInfo.emplace_back(debugRun->left_hand.debugValues);
	handInfo.emplace_back(debugRun->right_hand.debugValues);

	// asdkfhjasdkfhaskdfjhasfd
	if (!DoesFileExist(calc_params_xml)) {
		TheGreatBazoinkazoinkInTheSky ublov;
		ublov.write_params_to_disk();
	}
}
#pragma endregion

int mina_calc_version = 380;
auto
GetCalcVersion() -> int
{
#ifdef USING_NEW_CALC
	return mina_calc_version;
#else
	return 263;
#endif
}
