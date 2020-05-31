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
#include <assert.h>
#include "Etterna/Globals/global.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"

using std::deque;
using std::max;
using std::min;
using std::pair;
using std::pow;
using std::set;
using std::sqrt;
using std::unordered_set;
using std::vector;

#pragma more stuff
static const std::string calc_params_xml = "Save/calc params.xml";
// intervals are _half_ second, no point in wasting time or cpu cycles on 100
// nps joke files
static const int max_nps_for_single_interval = 50;
static const vector<float> dimples_the_all_zero_output{ 0.f, 0.f, 0.f, 0.f,
														0.f, 0.f, 0.f, 0.f };
static const vector<float> gertrude_the_all_max_output{ 100.f, 100.f, 100.f,
														100.f, 100.f, 100.f,
														100.f, 100.f };
static const vector<int> col_ids = { 1, 2, 4, 8 };
static const int zto3[4] = { 0, 1, 2, 3 };
									// THESE ARE ACTUALLY MIRROR'D 
static const char note_map[16][5]{ "0000", "0001", "0010", "0011",
								   "0100", "0101", "0110", "0111",
								   "1000", "1001", "1010", "1011",
								   "1100", "1101", "1110", "1111" };
enum tap_size
{
	single,
	jump,
	hand,
	quad
};

static const float s_init = -5.f;
static const float ms_init = 5000.f;

// neutral pattern mod value.. as opposed to min
static const float neutral = 1.f;
#pragma endregion

// DON'T WANT TO RECOMPILE HALF THE GAME IF I EDIT THE HEADER FILE
// global multiplier to standardize baselines
static const float finalscaler = 3.632f;

// ***note*** if we want max control over stamina we need to have one model for
// affecting the other skillsets to a certain degree, enough to push up longer
// stream ratings into contention with shorter ones, and another for both a more
// granular and influential modifier to calculate the end stamina rating with
// so todo on that

// Stamina Model params
static const float stam_ceil = 1.075234f; // stamina multiplier max
static const float stam_mag = 243.f;	  // multiplier generation scaler
static const float stam_fscale = 500.f; // how fast the floor rises (it's lava)
static const float stam_prop =
  0.69424f; // proportion of player difficulty at which stamina tax begins

// since we are no longer using the normalizer system we need to lower
// the base difficulty for each skillset and then detect pattern types
// to push down OR up, rather than just down and normalizing to a differential
// since chorded patterns have lower enps than streams, streams default to 1
// and chordstreams start lower
// stam is a special case and may use normalizers again
static const float basescalers[NUM_Skillset] = { 0.f,			0.97f, 0.875f,
												 0.89f / 1.02f, 0.94f, 0.7675f,
												 0.84f,			1.075f };
bool debug_lmao = false;

#pragma region stuffs
// Relies on endiannes (significantly inaccurate)
inline float
fastpow(double a, double b)
{
	int u[2];
	std::memcpy(&u, &a, sizeof a);
	u[1] = static_cast<int>(b * (u[1] - 1072632447) + 1072632447);
	u[0] = 0;
	std::memcpy(&a, &u, sizeof a);
	return static_cast<float>(a);
}

// not super accurate, good enough for our purposes
inline float
fastsqrt(float _in)
{
	if (_in == 0.f)
		return 0.f;
	__m128 in = _mm_load_ss(&_in);
	float out;
	_mm_store_ss(&out, _mm_mul_ss(in, _mm_rsqrt_ss(in)));
	return out;
}

inline float
ms_from(const float& now, const float& last)
{
	return (now - last) * 1000.f;
}

inline float
weighted_average(const float& a, const float& b, const float& x, const float& y)
{
	return (x * a + ((y - x) * b)) / y;
}

template<typename T>
inline T
CalcClamp(T x, T l, T h)
{
	return x > h ? h : (x < l ? l : x);
}

// template thingy for generating basic proportion scalers for pattern mods
// potentially super broken
template<typename T>
inline float
pmod_prop(T a,
		  T b,
		  const float& s,
		  const float& min,
		  const float& max,
		  const float& base = 0.f)
{
	return CalcClamp(
	  (static_cast<float>(a) / static_cast<float>(b) * s) + base, min, max);
}

// template thingy for generating basic proportion scalers for pattern mods
// potentially super broken
template<typename T>
inline float
pmod_prop(const float& pool,
		  T a,
		  T b,
		  const float& s,
		  const float& min,
		  const float& max)
{
	return CalcClamp(
	  pool - (static_cast<float>(a) / static_cast<float>(b) * s), min, max);
}

inline float
mean(const vector<float>& v)
{
	return std::accumulate(begin(v), end(v), 0.f) /
		   static_cast<float>(v.size());
}

inline float
mean(const vector<int>& v)
{
	return std::accumulate(begin(v), end(v), 0) / static_cast<float>(v.size());
}

inline float
mean(const unordered_set<int>& v)
{
	return std::accumulate(begin(v), end(v), 0) / static_cast<float>(v.size());
}

// Coefficient of variation
inline float
cv(const vector<float>& input)
{
	float sd = 0.f;
	float average = mean(input);
	for (float i : input)
		sd += (i - average) * (i - average);

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

inline float
cv(const vector<int>& input)
{
	float sd = 0.f;
	float average = mean(input);
	for (int i : input)
		sd +=
		  (static_cast<float>(i) - average) * (static_cast<float>(i) - average);

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

inline float
cv(const unordered_set<int>& input)
{
	float sd = 0.f;
	float average = mean(input);
	for (int i : input)
		sd +=
		  (static_cast<float>(i) - average) * (static_cast<float>(i) - average);

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

// cv of a vector truncated to a set number of values, or if below, filled with
// dummy values to reach the desired num_vals
inline float
cv_trunc_fill(const vector<float>& input,
			  const int& num_vals,
			  const float& ms_dummy)
{
	int moop = static_cast<int>(input.size());
	float welsh_pumpkin = 0.f;
	float average = 0.f;
	if (moop >= num_vals) {
		for (size_t i = 0; i < min(moop, num_vals); ++i)
			average += input[i];
		average /= num_vals;

		for (size_t i = 0; i < min(moop, num_vals); ++i)
			welsh_pumpkin += (input[i] - average) * (input[i] - average);

		// prize winning, even
		return fastsqrt(welsh_pumpkin / static_cast<float>(num_vals)) / average;
	}

	for (size_t i = 0; i < min(moop, num_vals); ++i)
		average += input[i];

	// fill with dummies if input is below desired number of values
	for (size_t i = 0; i < num_vals - moop; ++i)
		average += ms_dummy;
	average /= num_vals;

	for (size_t i = 0; i < min(moop, num_vals); ++i)
		welsh_pumpkin += (input[i] - average) * (input[i] - average);

	for (size_t i = 0; i < num_vals - moop; ++i)
		welsh_pumpkin += (ms_dummy - average) * (ms_dummy - average);

	return fastsqrt(welsh_pumpkin / static_cast<float>(num_vals)) / average;
}

inline float
sum_trunc_fill(const vector<float>& input,
			   const int& num_vals,
			   const float& ms_dummy)
{
	int moop = static_cast<int>(input.size());
	float smarmy_hamster = 0.f;
	// use up to num_vals
	for (size_t i = 0; i < min(moop, num_vals); ++i)
		smarmy_hamster += input[i];

	// got enough
	if (moop >= num_vals)
		return smarmy_hamster;

	// fill with dummies if input is below desired number of values
	for (size_t i = 0; i < num_vals - static_cast<int>(moop); ++i)
		smarmy_hamster += ms_dummy;

	// real piece of work this guy
	return smarmy_hamster;
}

inline float
downscale_low_accuracy_scores(float f, float sg)
{
	return sg >= 0.93f
			 ? f
			 : min(max(f / pow(1.f + (0.93f - sg), 0.75f), 0.f), 100.f);
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
		i = (f1 + f2 + f3) / 3.f;
	}
}

inline void
Smooth(vector<vector<float>>& input, float neutral)
{
	float f1;
	float f2 = neutral;
	for (auto& itv : input)
		for (float& i : itv) {
			f1 = f2;
			f2 = i;
			i = (f1 + f2 * 2.f) / 3.f;
		}
}

inline void
DifficultyMSSmooth(vector<float>& input)
{
	float f1;
	float f2 = 0.f;

	for (float& i : input) {
		f1 = f2;
		f2 = i;
		i = (f1 + f2) / 2.f;
	}
}

inline float
AggregateScores(const vector<float>& skillsets, float rating, float resolution)
{
	float sum;
	for (int iter = 1; iter <= 11; iter++) {
		do {
			rating += resolution;
			sum = 0.0f;
			for (float i : skillsets) {
				sum += 2.f / std::erfc(0.5f * (i - rating)) - 1.f;
			}
		} while (3 < sum);
		rating -= resolution;
		resolution /= 2.f;
	}
	return rating + 2.f * resolution;
}

inline unsigned int
column_count(unsigned int note)
{
	return note % 2 + note / 2 % 2 + note / 4 % 2 + note / 8 % 2;
}

float
chord_proportion(const vector<NoteInfo>& NoteInfo, const int chord_size)
{
	unsigned int taps = 0;
	unsigned int chords = 0;

	for (auto row : NoteInfo) {
		unsigned int notes = column_count(row.notes);
		taps += notes;
		if (notes == chord_size)
			chords += notes;
	}
	assert(taps > 0);
	return static_cast<float>(chords) / static_cast<float>(taps);
}

inline int
max_val(vector<int>& v)
{
	return *std::max_element(v.begin(), v.end());
}

inline float
max_val(vector<float>& v)
{
	return *std::max_element(v.begin(), v.end());
}

inline size_t
max_index(vector<float>& v)
{
	return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

inline int
sum(vector<int>& v)
{
	return std::accumulate(begin(v), end(v), 0);
}

inline int
sum(deque<int>& v)
{
	return std::accumulate(begin(v), end(v), 0);
}

inline float
sum(vector<float>& v)
{
	return std::accumulate(begin(v), end(v), 0.f);
}

void
Calc::TotalMaxPoints()
{
	MaxPoints = 0;
	for (size_t i = 0; i < left_hand.v_itvpoints.size(); i++)
		MaxPoints += left_hand.v_itvpoints[i] + right_hand.v_itvpoints[i];
}

void
Hand::InitPoints(const Finger& f1, const Finger& f2)
{
	v_itvpoints.clear();
	for (size_t ki_is_rising = 0; ki_is_rising < f1.size(); ++ki_is_rising)
		v_itvpoints.emplace_back(f1[ki_is_rising].size() +
								 f2[ki_is_rising].size());
}
#pragma endregion utils are an antipattern

#pragma region CalcBodyFunctions
#pragma region JackModelFunctions
inline void
Calc::JackStamAdjust(float x, int t, int mode, bool debug)
{
	const bool dbg = false && debug;

	float stam_ceil = 1.075234f;
	float stam_mag = 75.f;
	float stam_fscale = 256.f;
	float stam_prop = 0.55424f;
	float stam_floor = 1.f;
	float mod = 1.f;
	float avs1 = 0.f;
	float avs2 = 0.f;
	float local_ceil = stam_ceil;
	float super_stam_ceil = 1.11f;
	if (mode == 4)
		super_stam_ceil = 1.09f;
	const auto& diff = jacks[mode][t];

	if (debug) {
		left_hand.debugValues[2][JackStamMod].resize(numitv);
		right_hand.debugValues[2][JackStamMod].resize(numitv);

		// each interval
		for (size_t i = 0; i < diff.size(); ++i) {
			float mod_sum = 0.f;
			// each jack in the interval
			for (size_t j = 0; j < diff[i].size(); ++j) {
				avs1 = avs2;
				avs2 = diff[i][j];

				if (dbg)
					std::cout << "mod was : " << mod
							  << " at diff : " << diff[i][j] << std::endl;

				mod +=
				  ((((avs1 + avs2) / 2.f) / (stam_prop * x)) - 1.f) / stam_mag;
				if (mod > 0.95f)
					stam_floor += (mod - 0.95f) / stam_fscale;
				local_ceil = stam_ceil * stam_floor;

				mod =
				  min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);

				if (dbg)
					std::cout << "mod now : " << mod << std::endl;

				mod_sum += mod;
				stam_adj_jacks[t][i][j] = diff[i][j] * mod;
			}
			// yes i know it's 1 col per hand atm
			float itv_avg = 1.f;
			if (diff[i].size() > 1)
				itv_avg = mod_sum / static_cast<float>(diff[i].size());

			if (t == 0)
				left_hand.debugValues[2][JackStamMod][i] = itv_avg;
			else if (t == 3)
				right_hand.debugValues[2][JackStamMod][i] = itv_avg;
		}
	} else
		for (size_t i = 0; i < diff.size(); ++i) {
			for (size_t j = 0; j < diff[i].size(); ++j) {
				avs1 = avs2;
				avs2 = diff[i][j];
				mod +=
				  ((((avs1 + avs2) / 2.f) / (stam_prop * x)) - 1.f) / stam_mag;
				if (mod > 0.95f)
					stam_floor += (mod - 0.95f) / stam_fscale;
				local_ceil = stam_ceil * stam_floor;

				mod =
				  min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
				stam_adj_jacks[t][i][j] = diff[i][j] * mod;
			}
		}
}
static const float magic_num = 7.5f;
inline float
hit_the_road(float x, float y, int mode)
{
	return (CalcClamp(
	  magic_num - (magic_num * fastpow(x / y, 2.5f)), 0.f, magic_num));
}

// returns a positive number or 0, output should be subtracted
float
Calc::JackLoss(float x, int mode, float mpl, bool stam, bool debug)
{
	// mpl *= 1.5f;
	const bool dbg = false && debugmode;
	// adjust for stam before main loop, since main loop is interval -> track
	// and not track -> interval, we could also try doing this on the fly with
	// an inline but i cba to mess with that atm
	if (stam)
		for (auto t : { 0, 1, 2, 3 })
			JackStamAdjust(x, t, mode, debug);

	float total_point_loss = 0.f;
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
			float loss = 0.f;
			for (auto& j : seagull) {
				if (x >= j)
					continue;
				loss += hit_the_road(x, j, mode);

				if (dbg)
					std::cout << "loss for diff : " << j
							  << " with pskill: " << x << " : "
							  << hit_the_road(x, j, mode) << std::endl;
			}
			flurbo[t] = loss;
		}

		if (debugmode) {
			left_loss[i] = max(flurbo[0], flurbo[1]);
			right_loss[i] = max(flurbo[2], flurbo[3]);
			// slight optimization i guess, bail if we can no longer reach score
			// goal (but only outside of debug, //////and not for minijacks)
		} else if (total_point_loss > mpl)
			return total_point_loss;

		total_point_loss += max(flurbo[0], flurbo[1]);
		total_point_loss += max(flurbo[2], flurbo[3]);
	}
	if (debugmode) {
		left_hand.debugValues[2][JackPtLoss] = left_loss;
		right_hand.debugValues[2][JackPtLoss] = right_loss;
	}

	total_point_loss = CalcClamp(total_point_loss, 0.f, 10000.f);
	return total_point_loss;
}

inline float
ms_to_bpm(float x)
{
	return 15000.f / x;
}

void
Calc::SequenceJack(const Finger& f, int track, int mode)
{
	const bool dbg = false && debugmode && mode == 1;
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
	for (int i = 0; i < window_size; ++i)
		window_taps.push_back(1250.f);

	vector<float> eff_scalers(window_size);

	// doge adventure etc (maybe don't set this too low yet)
	static const float max_diff = 55.f;

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
	if (dbg)
		std::cout << "sequence jack on track: " << track << std::endl;
	float time = 0.f;
	float eff_ms = 0.f;
	float eff_bpm = 0.f;
	float ms = 0.f;
	const float mode_buffers[4] = { 12.5f, 250.f, 120.f, 225.f };
	static const float jack_global_scaler =
	  finalscaler * basescalers[Skill_JackSpeed] / 15.f;
	static const float mode_scalers[4] = {
		0.9555f, 0.003f * 35.12f / 36.f, 1.28f, 1.5f * 30.5f / 29.5f
	};
	jacks[mode][track].resize(numitv);
	float comp_diff = 0.f;
	for (size_t itv = 0; itv < f.size(); ++itv) {
		jacks[mode][track][itv].resize(f[itv].size());
		// taps in interval
		for (size_t ind = 0; ind < f[itv].size(); ++ind) {
			ms = f[itv][ind];
			time += ms;
			if (dbg) {
				std::cout << "time now: " << time / 1000.f << std::endl;
				std::cout << "ms now: " << ms << std::endl;
			}

			// shift older values back
			for (size_t i = 1; i < window_taps.size(); ++i)
				window_taps[i - 1] = window_taps[i];
			// add new value
			window_taps[window_size - 1] = ms;

			// effective bpm based on a hit window buffer
			eff_ms = sum(window_taps) + mode_buffers[mode];
			eff_bpm = ms_to_bpm(eff_ms / window_taps.size());
			if (mode == 1)
				eff_bpm = pow(ms_to_bpm(eff_ms / window_taps.size()), 2.5f);
			comp_diff = eff_bpm * jack_global_scaler;

			jacks[mode][track][itv][ind] =
			  CalcClamp(comp_diff * mode_scalers[mode], 0.f, max_diff);

			if (dbg) {
				std::cout << "base bpm: "
						  << ms_to_bpm(sum(window_taps) / window_taps.size())
						  << " : eff bpm: " << eff_bpm << std::endl;
				std::cout << "fdiff: " << jacks[mode][track][itv][ind] << "\n"
						  << std::endl;
			}
		}
	}
}
#pragma endregion

Finger
Calc::ProcessFinger(const vector<NoteInfo>& NoteInfo,
					unsigned int t,
					float music_rate,
					float offset,
					bool& joke_file_mon)
{
	// optimization, just allocate memory here once and recycle this vector
	vector<float> temp_queue(max_nps_for_single_interval);
	vector<int> temp_queue_two(max_nps_for_single_interval);
	unsigned int row_counter = 0;
	unsigned int row_counter_two = 0;

	int Interval = 0;
	float last = -5.f;
	Finger AllIntervals(numitv, vector<float>());
	if (t == 0)
		nervIntervals = vector<vector<int>>(numitv, vector<int>());
	unsigned int column = 1u << t;

	for (size_t i = 0; i < NoteInfo.size(); i++) {
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
				for (unsigned int n = 0; n < row_counter; ++n)
					AllIntervals[Interval][n] = temp_queue[n];
			}

			if (row_counter_two > 0) {
				nervIntervals[Interval].resize(row_counter_two);
				for (unsigned int n = 0; n < row_counter_two; ++n)
					nervIntervals[Interval][n] = temp_queue_two[n];
			}

			// reset the counter and iterate interval
			row_counter = 0;
			row_counter_two = 0;
			++Interval;
		}

		if (NoteInfo[i].notes & column) {
			// log all rows for this interval in pre-allocated mem
			// this is clamped to stop 192nd single minijacks from having an
			// outsize influence on anything, they aren't actually that hard in
			// isolation due to hit windows
			temp_queue[row_counter] =
			  CalcClamp(1000.f * (scaledtime - last), 40.f, 5000.f);
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

vector<float>
Calc::CalcMain(const vector<NoteInfo>& NoteInfo,
			   float music_rate,
			   float score_goal)
{
	// actual cancer
	debug_lmao = debugmode;

	// in flux
	float grindscaler =
	  CalcClamp(
		0.9f + (0.1f * ((NoteInfo.back().rowTime / music_rate) - 35.f) / 35.f),
		0.9f,
		1.f) *
	  CalcClamp(
		0.9f + (0.1f * ((NoteInfo.back().rowTime / music_rate) - 15.f) / 15.f),
		0.9f,
		1.f) *
	  CalcClamp(
		0.4f + (0.6f * ((NoteInfo.back().rowTime / music_rate) - 10.f) / 10.f),
		0.4f,
		1.f);

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
		  NoteInfo, music_rate, 0.1f * WHAT_IS_EVEN_HAPPEN_THE_BOMB);

		// if we exceed max_nps_for_single_interval during
		// processing
		if (!continue_calc) {
			std::cout << "skipping junk file" << std::endl;
			return gertrude_the_all_max_output;
		}

		TotalMaxPoints();

		vector<float> mcbloop(NUM_Skillset);
		// overall and stam will be left as 0.f by this loop
		for (int i = 0; i < NUM_Skillset; ++i)
			mcbloop[i] = Chisel(0.1f, 10.24f, score_goal, i, false);

		// stam is based on which calc produced the highest
		// output without it
		size_t highest_base_skillset = max_index(mcbloop);
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
		for (int i = 0; i < NUM_Skillset; ++i)
			if (i == Skill_JackSpeed) {
				if (highest_base_skillset == Skill_JackSpeed ||
					highest_base_skillset == Skill_Technical)
					mcbloop[i] =
					  Chisel(mcbloop[i] * 1.f, 0.32f, score_goal, i, true);
			} else
				mcbloop[i] =
				  Chisel(mcbloop[i] * 0.9f, 0.32f, score_goal, i, true);

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
		if (highest_base_skillset == Skill_JackSpeed)
			poodle_in_a_porta_potty *= 0.9f;

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
		static const float stam_curve_shift = 0.015f;
		// ends up being a multiplier between ~0.8 and ~1
		float mcfroggerbopper =
		  pow((poodle_in_a_porta_potty / base) - stam_curve_shift, 2.5f);

		// we wanted to shift the curve down a lot before
		// pow'ing but it was too much to balance out, so we
		// need to give some back, this is roughly
		// equivalent of multiplying by 1.05 but also not
		// really because math we don't want to push up the
		// high end stuff anymore so just add to let stuff
		// down the curve catch up a little remember we're
		// operating on a multiplier
		mcfroggerbopper = CalcClamp(mcfroggerbopper, 0.8f, 1.08f);
		mcbloop[Skill_Stamina] = poodle_in_a_porta_potty * mcfroggerbopper *
								 basescalers[Skill_Stamina];

		// sets the 'proper' debug output, doesn't
		// (shouldn't) affect actual values this is the only
		// time debugoutput arg should be set to true
		if (debugmode)
			Chisel(mcbloop[highest_base_skillset] - 0.16f,
				   0.32f,
				   score_goal,
				   highest_base_skillset,
				   true,
				   true);

		// the final push down, cap ssrs (score specific
		// ratings) to stop vibro garbage and calc abuse
		// from polluting leaderboards too much, a "true" 38
		// is still unachieved so a cap of 40 [sic] is
		// _extremely_ generous do this for SCORES only, not
		// cached file difficulties
		if (ssr) {
			static const float ssrcap = 40.f;
			for (auto& r : mcbloop) {
				// so 50%s on 60s don't give 35s
				r = downscale_low_accuracy_scores(r, score_goal);
				r = CalcClamp(r, r, ssrcap);
			}
		}

		// finished all modifications to skillset values, set overall
		mcbloop[Skill_Overall] = max_val(mcbloop);

		for (size_t bagles = 0; bagles < mcbloop.size(); ++bagles)
			the_hizzle_dizzles[WHAT_IS_EVEN_HAPPEN_THE_BOMB].push_back(
			  mcbloop[bagles]);
	}
	vector<float> yo_momma(NUM_Skillset);
	for (size_t farts = 0; farts < the_hizzle_dizzles[0].size(); ++farts) {
		vector<float> girls;
		for (size_t nibble = 0; nibble < the_hizzle_dizzles.size(); ++nibble) {
			girls.push_back(the_hizzle_dizzles[nibble][farts]);
		}
		yo_momma[farts] = mean(girls) * grindscaler;
		girls.clear();
	}

	return yo_momma;
}
#pragma endregion

#pragma region sequencing logic definitions
// cross column behavior between 2 notes
enum cc_type
{
	cc_left_right,
	cc_right_left,
	cc_jump_single,
	cc_single_single,
	cc_single_jump,
	cc_jump_jump,
	cc_init,
};

// hand specific meaning the left two or right two columns and only for 4k
enum col_type
{
	col_left,
	col_right,
	col_ohjump,
	col_empty,
	col_init
};

inline bool
is_col_type_single_tap(const col_type& col)
{
	return col == col_left || col == col_right;
}

// there are no empty rows, only empty hands
inline bool
is_empty_hand(const bool& a, const bool& b)
{
	return !a && !b;
}

inline cc_type
determine_cc_type(const col_type& last, const col_type& now)
{
	if (last == col_init)
		return cc_init;

	bool single_tap = is_col_type_single_tap(now);
	if (last == col_ohjump) {
		if (single_tap)
			return cc_jump_single;
		else
			// can't be anything else
			return cc_jump_jump;
	} else if (!single_tap)
		return cc_single_jump;
	// if we are on left col _now_, we are right to left
	else if (now == col_left && last == col_right)
		return cc_right_left;
	else if (now == col_right && last == col_left)
		return cc_left_right;
	else if (now == last)
		// anchor/jack
		return cc_single_single;

	// makes no logical sense
	ASSERT(1 == 0);
	return cc_init;
}

inline col_type
bool_to_col_type(const bool& lcol, const bool& rcol)
{
	if (is_empty_hand(lcol, rcol))
		return col_empty;
	if (lcol - rcol)
		return lcol ? col_left : col_right;
	return col_ohjump;
}

// inverting col state for col_left or col_right only
inline col_type
invert_col(const col_type& col)
{
	ASSERT(col == col_left || col == col_right);
	return col == col_left ? col_right : col_left;
}

// inverting cc state for left_right or right_left only
inline cc_type
invert_cc(const cc_type& cc)
{
	ASSERT(cc == cc_left_right || cc == cc_right_left);
	return cc == cc_left_right ? cc_right_left : cc_left_right;
}

inline void
update_col_time(const col_type& col, float arr[2], const float& val)
{
	// update both
	if (col == col_ohjump) {
		arr[0] = val;
		arr[1] = val;
		return;
	}
	if (col == col_left)
		arr[0] = val;
	else if (col == col_right)
		arr[1] = val;
	return;
}
#pragma endregion

#pragma region noteinfo bitwise operations
// bitwise operations on noteinfo.notes, they must be unsigned ints, and
// shouldn't be called on enums or row counts or anything like that
inline bool
is_single_tap(const unsigned& a)
{
	return (a & (a - 1)) == 0;
}

// between two successive rows usually... but i suppose this could be called
// outside of that limitation
inline bool
is_jack_at_col(const unsigned& id,
			   const unsigned& row_notes,
			   const unsigned& last_row_notes)
{
	return id & row_notes && id & last_row_notes;
}

// doesn't check for jacks
inline bool
is_alternating_chord_single(const unsigned& a, const unsigned& b)
{
	return (a > 1 && b == 1) || (a == 1 && b > 1);
}

// ok lets stop being bad, find 1[n]1 or [n]1[n] with no jacks between first and
// second and second and third elements
inline bool
is_alternating_chord_stream(const unsigned& a,
							const unsigned& b,
							const unsigned& c)
{
	if (is_single_tap(a)) {
		if (is_single_tap(b)) {
			// single single, don't care, bail
			return false;
		} else if (!is_single_tap(c))
			// single, chord, chord, bail
			return false;
	} else {
		if (!is_single_tap(b)) {
			// chord chord, don't care, bail
			return false;
		} else if (is_single_tap(c))
			// chord, single, single, bail
			return false;
	}
	// we have either 1[n]1 or [n]1[n], check for any jacks
	return (a & b && b & c) == 0;
}
#pragma endregion

#pragma region new pattern mod structure
// accumulates info across an interval as it's processed by row
// this should really be moved out of mni maybe probably, since it's generated
// twice and doesn't need to be (this definitely totally needs to happen)
struct itv_info
{
	bool dbg = false && debug_lmao;

	int seriously_not_js = 0;
	int definitely_not_jacks = 0;
	int actual_jacks = 0;
	int actual_jacks_cj = 0;
	int not_js = 0;
	int not_hs = 0;
	int zwop = 0;
	int total_taps = 0;
	int col_taps[4] = { 0, 0, 0, 0};
	int hand_taps[2] = { 0, 0 };
	int chord_taps = 0;
	int taps_by_size[4] = { 0, 0, 0, 0 };
	int shared_chord_jacks = 0;

	// ok new plan instead of a map, keep an array of 3, run a comparison loop
	// that sets 0s to a new value if that value doesn't match any non 0 value,
	// and set a bool flag if we have filled the array with unique values
	unsigned row_variations[3] = { 0, 0, 0 };

	// unique(noteinfos for interval) < 3, or row_variations[2] == 0 by interval
	// end
	bool basically_vibro = true;

	// resets all the stuff that accumulates across intervals
	inline void reset()
	{
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

		// self explanatory
		total_taps = 0;

		// number of non-single taps
		chord_taps = 0;

		// self explanatory and unused
		shared_chord_jacks = 0;

		for (auto& t : col_taps)
			t = 0;
		for (auto& t : hand_taps)
			t = 0;
		// see def
		for (auto& t : taps_by_size)
			t = 0;
		for (auto& t : row_variations)
			t = 0;

		// see def
		basically_vibro = true;
	}

	inline void update_row_variations_and_set_vibro_flag(
	  const unsigned& row_notes)
	{
		// already determined there's enough variation in this interval
		if (!basically_vibro)
			return;

		// trying to fill array with up to 3 unique row_note configurations
		for (auto& t : row_variations) {
			// already a stored value here
			if (t != 0) {
				// already have one of these
				if (t == row_notes) {
					return;
				}
			} else if (t == 0) {
				// nothing stored here and isn't a duplicate, store it
				t = row_notes;

				// check if we filled the array with unique values. since we
				// start by assuming anything is basically vibro, set the flag
				// to false if it is
				if (row_variations[2] != 0)
					basically_vibro = false;
				return;
			}
		}
	}

	// will need last for last.last_row_notes
	// i guess the distinction here that i'm starting to notice is that these
	// are done row by row, whereas the cc sequencing is done hand by hand, so
	// maybe this stuff should be abstracted similarly

	inline void update_tap_counts(const int& row_count)
	{
		total_taps += row_count;

		// ALWAYS COUNT NUMBER OF TAPS IN CHORDS
		if (row_count > 1)
			chord_taps += row_count;

		// ALWAYS COUNT NUMBER OF TAPS IN CHORDS
		taps_by_size[row_count - 1] += row_count;

		// we want mixed hs/js to register as hs, even at relatively sparse hand
		// density
		if (taps_by_size[hand] > 0)
			// this seems kinda extreme? it'll add the number of jumps in the
			// whole interval every hand? maybe it needs to be that extreme?
			taps_by_size[hand] += taps_by_size[jump];
	}

	// this seems messy.. but we want to aggreagte interval info and so we need
	// to transfer the last values to the current object, then update them
	// this should definitely actually be its own struct tbh
	inline void set_interval_data_from_last(const itv_info& last)
	{
		seriously_not_js = last.seriously_not_js;
		definitely_not_jacks = last.definitely_not_jacks;
		actual_jacks = last.actual_jacks;
		actual_jacks_cj = last.actual_jacks_cj;
		not_js = last.not_js;
		not_hs = last.not_hs;

		total_taps = last.total_taps;
		chord_taps = last.chord_taps;
		shared_chord_jacks = last.shared_chord_jacks;

		for (size_t i = 0; i < 4; ++i)
			col_taps[i] = last.col_taps[i];
		for (size_t i = 0; i < 2; ++i)
			hand_taps[i] = last.hand_taps[i];
		for (size_t i = 0; i < 4; ++i)
			taps_by_size[i] = last.taps_by_size[i];
		for (size_t i = 0; i < 3; ++i)
			row_variations[i] = last.row_variations[i];

		basically_vibro = last.basically_vibro;
	}

	inline void operator()(const itv_info& last,
						   const int& row_count,
						   const unsigned& row_notes)
	{
		set_interval_data_from_last(last);
		// could use this to really blow out jumptrill garbage in js/hs, but
		// only used by cj atm
		update_row_variations_and_set_vibro_flag(row_notes);
		update_tap_counts(row_count);
		// hand specific tap counts, multiple different pattern mods need them
		// so let's track them here (lazy i know)
		if (row_notes & col_ids[0]) {
			++col_taps[0];
			++hand_taps[0];
		}
		if (row_notes & col_ids[1]) {
			++col_taps[1];
			++hand_taps[0];
		}
		if (row_notes & col_ids[2]) {
			++col_taps[2];
			++hand_taps[1];
		}
		if (row_notes & col_ids[3]) {
			++col_taps[3];
			++hand_taps[1];
		}
	}
};

// big brain stuff
inline bool
detecc_oht(const cc_type& a, const cc_type& b, const cc_type& c)
{
	// we are flipping b with invert col so make sure it's left_right or
	// right_left single note, if either of the other two aren't this will fail
	// anyway and it's fine
	if (b != cc_left_right && b != cc_right_left)
		return false;

	bool loot = a == invert_cc(b);
	bool doot = a == c;
	// this is kind of big brain so if you don't get it that's ok
	return loot && doot;
}

inline bool
detecc_ccacc(const cc_type& a, const cc_type& b)
{
	if (a != cc_left_right && a != cc_right_left)
		return false;

	// now we know we have cc_left_right or cc_right_left, so, xy, we are
	// looking for xyyx, meaning last_last would be the inverion of now
	if (invert_cc(a) == b)
		return true;

	return false;
}

inline bool
detecc_acca(const cc_type& a, const cc_type& b, const cc_type& c)
{
	// 1122, 2211, etc
	if (a == cc_single_single && (b == cc_left_right || b == cc_right_left) &&
		c == cc_single_single)
		return true;

	return false;
}

enum meta_type
{
	meta_oht,
	meta_ccacc,
	meta_acca,
	meta_init,
	meta_enigma
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
struct metanoteinfo
{
	bool dbg = false && debug_lmao;
	bool dbg_lv2 = false && debug_lmao;

#pragma region row specific data
	// time (s) of the last seen note in each column
	float row_time = s_init;
	float col_time[2] = { s_init, s_init };

	// col
	col_type col = col_init;
	col_type last_non_empty_col = col_init;
	col_type last_col = col_init;
	// type of cross column hit
	cc_type cc = cc_init;
	cc_type last_cc = cc_init;

	meta_type mt = meta_init;
	meta_type last_mt = meta_init;

	// the row notes, yes, this will be redundant, maybe need a metarowinfo that
	// contains 2 metanoteinfos? ... yes... we probably do.. uhh
	unsigned row_notes = 0;
	unsigned last_row_notes = 0;

	// number of notes in the row
	int row_count = 0;

	// last col == col_empty
	bool last_was_offhand_tap = false;

	// ms from last cross column note
	float cc_ms_any = ms_init;

	// ms from last cross column note, excluding jumps (unused atm... might need
	// later)
	float cc_ms_no_jumps = ms_init;

	// ms from last note in this column
	float tc_ms = ms_init;

	// per row bool flags, these must be directly set every row
	bool alternating_chordstream = false;
	bool alternating_chord_single = false;
	bool gluts_maybe = false; // not really used/tested yet
	bool twas_jack = false;
#pragma endregion

	itv_info _itv_info;

	// sets time from last note in the same column, and last note in the
	// opposite column, handling for jumps is not completely fleshed out yet
	// maybe, i think any case specific handling of their timings can be done
	// with the information already given
	inline void set_timings(const float cur[2],
							const float last[2],
							const col_type& last_col)
	{
		switch (cc) {
			case cc_left_right:
			case cc_right_left:
			case cc_jump_single:
			case cc_single_single:
				// either we know the end col so we know the start col, or the
				// start col doesn't matter
				cc_ms_any = ms_from(cur[col], last[invert_col(col)]);

				// technically doesn't matter if we use last_col to index, if
				// it's single -> single we know it's an anchor so it's more
				// intuitive to use col twice
				tc_ms = ms_from(cur[col], last[col]);
				break;
			case cc_single_jump:
				// tracking this for now, use the higher value of the array
				// (lower ms time, i.e. the column closest to this jump)
				if (last[col_left] > last[col_right])
					cc_ms_any = ms_from(cur[col_left], last[col_right]);
				else
					cc_ms_any = ms_from(cur[col_right], last[col_left]);

				// logically the same as cc_ms_any in 1[12] 1 is the anchor
				// timing with 1 and also the cross column timing with 2
				tc_ms = cc_ms_any;
				break;
			case cc_jump_jump:
				// not sure if we should set or leave at init value of 5000.f
				// cc_ms_any = 0.f;

				// indexes don't matter-- except that we can't use col or
				// last_col (because index 2 is outside array size)
				tc_ms = ms_from(cur[0], last[0]);
				break;
			case cc_init:
				break;
			default:
				// cc_type should never be anything but the above, for any
				// reason, if we need to ignore an empty hand on a row, we
				// should be checking col_empty, never the old cc_empty which
				// now doesn't exist because logically it makes no sense
				ASSERT(1 == 0);
				break;
		}
		return;
	}

	// col_type is hand specific, col_left doesn't mean 1000, it means 10
	// cc is also hand specific, and is the nature of how the last 2 notes on
	// this hand interact
	inline void set_col_type(const bool& lcol, const bool& rcol)
	{
		col = bool_to_col_type(lcol, rcol);
	}

	inline void set_cc_type(const col_type& last_col)
	{
		cc = determine_cc_type(last_col, col);
	}

	inline void jack_scan()
	{
		twas_jack = false;

		for (auto& id : col_ids) {
			if (is_jack_at_col(id, row_notes, last_row_notes)) {
				if (dbg_lv2) {
					std::cout
					  << "actual jack with notes: " << note_map[row_notes]
					  << " : " << note_map[last_row_notes] << std::endl;
				}
				// not scaled to the number of jacks anymore
				++_itv_info.actual_jacks;
				twas_jack = true;
				// try to pick up gluts maybe?
				if (row_count > 1 && column_count(last_row_notes) > 1)
					++_itv_info.shared_chord_jacks;
			}
		}

		// if we used the normal actual_jack for CJ too
		// we're saying something like "chordjacks" are
		// harder if they share more columns from chord to
		// chord" which is not true, it is in fact either
		// irrelevant or the inverse depending on the
		// scenario, this is merely to catch stuff like
		// splithand jumptrills registering as chordjacks
		// when they shouldn't be
		if (twas_jack)
			++_itv_info.actual_jacks_cj;
	}

	// will need last for last.last_row_notes
	// i guess the distinction here that i'm starting to notice is that these
	// are done row by row, whereas the cc sequencing is done hand by hand, so
	// maybe this stuff should be abstracted similarly
	inline void basic_pattern_sequencing(const metanoteinfo& last)
	{
		jack_scan();

		// check if we have a bunch of stuff like [123]4[123]
		// [12]3[124] which isn't actually chordjack, its just
		// broken hs/js, and in fact with the level of leniency
		// that is currently being applied to generic
		// proportions, lots of heavy js/hs is being counted as
		// cj for their 2nd rating, and by a close margin too,
		// we can't just look for [123]4, we need to finish the
		// sequence to be sure i _think_ we only want to do this
		// for single notes, we could abstract it to a more
		// generic pattern template, but let's be restrictive
		// for now
		alternating_chordstream = is_alternating_chord_stream(
		  row_notes, last_row_notes, last.last_row_notes);
		if (alternating_chordstream) {
			if (dbg_lv2)
				std::cout << "good hot js/hs !!!!: " << std::endl;
			++_itv_info.definitely_not_jacks;
		}

		// only cares about single vs chord, not jacks
		alternating_chord_single =
		  is_alternating_chord_single(row_count, last.row_count);
		if (alternating_chord_single) {
			if (!twas_jack) {
				if (dbg_lv2)
					std::cout << "good hot js/hs: " << std::endl;
				_itv_info.seriously_not_js -= 3;
			}
		}

		if (last.row_count == 1 && row_count == 1) {
			_itv_info.seriously_not_js = max(_itv_info.seriously_not_js, 0);
			++_itv_info.seriously_not_js;
			if (dbg_lv2)
				std::cout << "consecutive single note: "
						  << _itv_info.seriously_not_js << std::endl;

			// light js really stops at [12]321[23] kind of
			// density, anything below that should be picked up
			// by speed, and this stop rolls between jumps
			// getting floated up too high
			if (_itv_info.seriously_not_js > 3) {
				if (dbg)
					std::cout << "exceeding light js/hs tolerance: "
							  << _itv_info.seriously_not_js << std::endl;
				_itv_info.not_js += _itv_info.seriously_not_js;
				// give light hs the light js treatment
				_itv_info.not_hs += _itv_info.seriously_not_js;
			}
		} else if (last.row_count > 1 && row_count > 1) {
			// suppress jumptrilly garbage a little bit
			if (dbg)
				std::cout << "sequential chords detected: " << std::endl;
			_itv_info.not_hs += row_count;
			_itv_info.not_js += row_count;

			// might be overkill
			if ((row_notes & last_row_notes) == 0) {
				if (dbg)
					std::cout << "bruh they aint even jacks: " << std::endl;
				++_itv_info.not_hs;
				++_itv_info.not_js;
			} else {
				gluts_maybe = true;
			}
		}
	}

	inline meta_type big_brain_sequencing(const metanoteinfo& last)
	{
		if (detecc_oht(cc, last_cc, last.last_cc))
			return meta_oht;
		if (detecc_ccacc(cc, last.last_cc))
			return meta_ccacc;
		if (detecc_acca(cc, last_cc, last.last_cc))
			return meta_acca;
		return meta_enigma;
	}

	inline void operator()(metanoteinfo& last,
						   const float& now,
						   const unsigned& notes,
						   const int& t1,
						   const int& t2,
						   const int& row,
						   bool silence = false)
	{
		// if ulbu is in debug mode it will create an extra mni object every row
		// and spit out the debugoutput twice
		dbg = dbg && !silence;

		set_col_type(t1 & notes, t2 & notes);
		row_count = column_count(notes);
		row_notes = notes;
		row_time = now;

		last_row_notes = last.row_notes;
		last_was_offhand_tap = last.col == col_empty;
		last_col = last.col;
		last_cc = last.cc;

		last_mt = last.mt;

		// need this to determine cc types if they are interrupted by offhand
		// taps
		if (col != col_empty)
			last_non_empty_col = col;
		else
			last_non_empty_col = last.last_non_empty_col;

		// set the interval data from the already accumulated data (we could
		// optimize by not doing this for the first execution of any
		// interval but... it's probably not worth it)
		_itv_info(last._itv_info, row_count, row_notes);

		// run the basic pattern sequencing pass, it will update basic pattern
		// counts in _itv_info and also set row specific flags if applicable
		basic_pattern_sequencing(last);

		// we don't want to set lasttime, lastcol, or re-evaluate cc_type for
		// for empty columns on this hand, carry the cc_type value forward and
		// ignore the timing values, they should never be referenced for col ==
		// empty, but in case they are accidentally, they should make no sense
		if (col == col_empty) {
			cc = last_cc;
			return;
		}

		// set updated cc type only for non-empty columns on this hand
		set_cc_type(last.last_non_empty_col);

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
		update_col_time(col, col_time, now);
		set_timings(col_time, last.col_time, last.col);
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

	// sequencing counters
	// only allow this rm's anchor col to start sequences
	bool in_the_nineties = false;
	// try to allow 1 burst?
	bool is_bursting = false;
	bool had_burst = false;
	float last_anchor_time = s_init;
	float last_off_time = s_init;
	int total_taps = 0;
	int ran_taps = 0;
	col_type anchor_col = col_init;
	cc_type last_cc = cc_init;
	cc_type last_last_cc = cc_init;
	int anchor_len = 0;
	int off_taps_same = 0;
	int oht_taps = 0;
	int oht_len = 0;
	int off_taps = 0;
	int off_len = 0;
	int jack_taps = 0;
	int jack_len = 0;
	float max_ms = ms_init;
	float off_total_ms = 0.f;

	col_type now_col = col_init;
	float now = 0.f;
	float temp_ms = 0.f;

#pragma region functions
	inline void reset()
	{
		// don't reset anchor_col or last_col, we want to preserve the pattern
		// state reset everything else tho

		// now_col and now don't need to be reset either

		in_the_nineties = false;
		is_bursting = false;
		had_burst = false;
		// reset?? don't reset????
		last_anchor_time = ms_init;
		last_off_time = s_init;
		total_taps = 0;
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
		off_total_ms = 0.f;

		// if we are resetting and this column is the anchor col, restart again
		if (anchor_col == now_col)
			handle_anchor_progression();
	}

	inline void handle_off_tap()
	{
		if (!in_the_nineties)
			return;
		last_off_time = now;

		++ran_taps;
		++off_taps;
		++off_len;

		// offnote, reset jack length & oht length
		jack_len = 0;

		// off_total_ms += ms_from(now, last_anchor_time);

		// handle progression for increasing off_len
		handle_off_tap_progression();

		// rolls
		if (off_len == max_off_spacing) {
			// ok do nothing for now i might have a better idea
		}
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
		if (off_len > max_off_spacing)
			is_bursting = true;
		return;
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
			if (temp_ms > max_ms + 5.f)
				reset();
			else
				max_ms = temp_ms;
		} else {
			// set first anchor val
			max_ms = 5000.f;
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
		if (jack_len > max_jack_len)
			reset();
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
			if (oht_len > max_oht_len)
				reset();
		}
	}

	inline void operator()(const metanoteinfo& mni)
	{
		total_taps += mni.row_count;

		now_col = mni.col;
		now = mni.row_time;

		// simple case to handle, can't be a jack (or doesn't really matter) and
		// can't be oht, only reset if we exceed the spacing limit, don't do
		// anything else
		if (mni.col == col_empty) {
			// reset oht len if we hit this (not really robust buuuut)
			oht_len = 0;
			handle_off_tap();
			return;
		}

		// cosmic brain
		if (mni.mt == meta_oht)
			handle_oht_progression();

		switch (mni.cc) {
			case cc_left_right:
			case cc_right_left:
				handle_cross_column_branching();
				break;
			case cc_jump_single:
				if (mni.last_was_offhand_tap) {
					// if we have a jump -> single, and the last
					// note was an offhand tap, and the single
					// is the anchor col, then we have an anchor
					if ((mni.col == col_left && anchor_col == col_left) ||
						(mni.col == col_right && anchor_col == col_right)) {
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
				if (mni.last_was_offhand_tap) {
					// if this wasn't a jack, then it's just
					// a good ol anchor
					handle_anchor_progression();
				} else {
					// a jack, not an anchor, we don't
					// want too many of these but we
					// don't want to allow none of them
					handle_jack_progression();
				}
				break;
			case cc_single_jump:
				// if last note was an offhand tap, this is by
				// definition part of the anchor
				if (mni.last_was_offhand_tap) {
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
				if (now_col == anchor_col)
					handle_anchor_progression();
				break;
			default:
				ASSERT(1 == 0);
				break;
		}
		last_last_cc = last_cc;
		last_cc = mni.cc;
	}
#pragma endregion
};

// since the calc skillset balance now operates on +- rather than
// just - and then normalization, we will use this to depress the
// stream rating for non-stream files.
struct StreamMod
{

	const vector<int> _pmods = { Stream };
	const std::string name = "StreamMod";
	const int _tap_size = single;
	const int _primary = _pmods.front();

#pragma region params
	float min_mod = 0.6f;
	float max_mod = 1.0f;
	float prop_buffer = 1.f;
	float prop_scaler = 1.428; // ~10/7

	float jack_pool = 4.f;
	float jack_comp_min = 0.5f;
	float jack_comp_max = 1.f;

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
	float prop_component = 0.f;
	float jack_component = 0.f;
	float pmod = min_mod;

#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], 1.f);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline bool handle_case_optimizations(const itv_info& itv,
										  vector<float> doot[],
										  const size_t& i)
	{
		// 1 tap is by definition a single tap
		if (itv.total_taps < 2) {
			neutral_set(doot, i);
			return true;
		}

		if (itv.taps_by_size[single] == 0)
		{
			min_set(doot, i);
			return true;
		}
		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i)
	{
		const auto& itv = mni._itv_info;

		if (handle_case_optimizations(itv, doot, i))
			return;

		// we want very light js to register as stream,
		// something like jumps on every other 4th, so 17/19
		// ratio should return full points, but maybe we should
		// allow for some leeway in bad interval slicing this
		// maybe doesn't need to be so severe, on the other
		// hand, maybe it doesn'ting need to be not needing'nt
		// to be so severe

		prop_component =
		  static_cast<float>(itv.taps_by_size[_tap_size] + prop_buffer) /
					 static_cast<float>(itv.total_taps - prop_buffer) * prop_scaler;

		// allow for a mini/triple jack in streams.. but not more than that
		jack_component =
		  CalcClamp(jack_pool - itv.actual_jacks, jack_comp_min, jack_comp_max);
		pmod = fastsqrt(prop_component * jack_component);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		// actual mod
		doot[_primary][i] = pmod;
	}
};
struct JSMod
{

	const vector<int> _pmods = { JS, JSS, JSJ };
	const std::string name = "JSMod";
	const int _tap_size = jump;
	const int _primary = _pmods.front();

#pragma region params
	float min_mod = 0.6f;
	float max_mod = 1.1f;
	float mod_base = 0.f;
	float prop_buffer = 1.f;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 2.714f; // ~19/7

	float split_hand_pool = 1.45f;
	float split_hand_min = 0.85f;
	float split_hand_max = 1.f;
	float split_hand_scaler = 1.f;

	float jack_pool = 1.35f;
	float jack_min = 0.5f;
	float jack_max = 1.f;
	float jack_scaler = 1.f;

	float decay_factor = 0.05f;

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
	float total_prop = 0.f;
	float jumptrill_prop = 0.f;
	float jack_prop = 0.f;
	float last_mod = min_mod;
	float pmod = min_mod;
	float t_taps = 0.f;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], 1.f);
	}

	inline void decay_mod()
	{
		pmod = CalcClamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline bool handle_case_optimizations(const itv_info& itv,
										  vector<float> doot[],
										  const size_t& i)
	{

		// empty interval, don't decay js mod or update last_mod
		if (itv.total_taps == 0) {
			neutral_set(doot, i);
			return true;
		}

		// at least 1 tap but no jumps
		if (itv.taps_by_size[_tap_size] == 0) {
			decay_mod();
			min_set(doot, i);
			doot[_primary][i] = pmod;
			return true;
		}
		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i)
	{
		const auto& itv = mni._itv_info;
		if (handle_case_optimizations(itv, doot, i))
			return;

		t_taps = static_cast<float>(itv.total_taps);

		// creepy banana
		total_prop =
		  static_cast<float>(itv.taps_by_size[_tap_size] + prop_buffer) /
		  (t_taps - prop_buffer) * total_prop_scaler;
		total_prop =
		  CalcClamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// punish lots splithand jumptrills
		// uhh this might also catch oh jumptrills can't remember
		jumptrill_prop =
		  CalcClamp(split_hand_pool - (static_cast<float>(itv.not_js) / t_taps),
					split_hand_min,
					split_hand_max);

		// downscale by jack density rather than upscale like cj
		// theoretically the ohjump downscaler should handle
		// this but handling it here gives us more flexbility
		// with the ohjump mod
		jack_prop =
		  CalcClamp(jack_pool - (static_cast<float>(itv.actual_jacks) / t_taps),
					jack_min,
					jack_max);

		pmod =
		  CalcClamp(total_prop * jumptrill_prop * jack_prop, min_mod, max_mod);

		// actual mod
		doot[_primary][i] = pmod;

		// debug
		doot[JSS][i] = jumptrill_prop;
		doot[JSJ][i] = jack_prop;

		// set last mod, we're using it to create a decaying mod that won't
		// result in extreme spikiness if files alternate between js and
		// hs/stream
		last_mod = pmod;
	}
};
struct HSMod
{
	const vector<int> _pmods = { HS, HSS, HSJ };
	const std::string name = "HSMod";
	const int _tap_size = hand;
	const int _primary = _pmods.front();

#pragma region params
	float min_mod = 0.6f;
	float max_mod = 1.1f;
	float mod_base = 0.4f;
	float prop_buffer = 1.f;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 4.571f; // ~32/7
	float total_prop_base = 0.4f;

	float split_hand_pool = 1.45f;
	float split_hand_min = 0.89f;
	float split_hand_max = 1.f;
	float split_hand_scaler = 1.f;

	float jack_pool = 1.35f;
	float jack_min = 0.5f;
	float jack_max = 1.f;
	float jack_scaler = 1.f;

	float decay_factor = 0.05f;

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
	float total_prop = 0.f;
	float jumptrill_prop = 0.f;
	float jack_prop = 0.f;
	float last_mod = min_mod;
	float pmod = min_mod;
	float t_taps = 0.f;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], 1.f);
	}

	inline void decay_mod()
	{
		pmod = CalcClamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline bool handle_case_optimizations(const itv_info& itv,
										  vector<float> doot[],
										  const size_t& i)
	{
		// empty interval, don't decay mod or update last_mod
		if (itv.total_taps == 0) {
			neutral_set(doot, i);
			return true;
		}

		// look ma no hands
		if (itv.taps_by_size[_tap_size] == 0) {
			decay_mod();
			min_set(doot, i);
			doot[_primary][i] = pmod;
			return true;
		}
		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i)
	{
		const auto& itv = mni._itv_info;
		if (handle_case_optimizations(itv, doot, i))
			return;

		t_taps = static_cast<float>(itv.total_taps);

		// when bark of dog into canyon scream at you
		total_prop =
		  total_prop_base +
		  (static_cast<float>(itv.taps_by_size[_tap_size] + prop_buffer) /
		   (t_taps - prop_buffer) * total_prop_scaler);
		total_prop =
		  CalcClamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// downscale jumptrills for hs as well
		jumptrill_prop =
		  CalcClamp(split_hand_pool - (static_cast<float>(itv.not_hs) / t_taps),
					split_hand_min,
					split_hand_max);

		// downscale by jack density rather than upscale, like cj does
		jack_prop =
		  CalcClamp(jack_pool - (static_cast<float>(itv.actual_jacks) / t_taps),
					jack_min,
					jack_max);
		// clamp the original prop mod first before applying
		// above

		pmod =
		  CalcClamp(total_prop * jumptrill_prop * jack_prop, min_mod, max_mod);

		// actual mod
		doot[_primary][i] = pmod;

		// debug
		doot[HSS][i] = jumptrill_prop;
		doot[HSJ][i] = jack_prop;

		// set last mod, we're using it to create a decaying mod that won't
		// result in extreme spikiness if files alternate between js and
		// hs/stream
		last_mod = pmod;
	}
};
struct CJMod
{
	bool dbg = false;
	const vector<int> _pmods = { CJ, CJS, CJJ, CJQuad };
	const std::string name = "CJMod";
	const int _primary = _pmods.front();

#pragma region params
	float min_mod = 0.6f;
	float max_mod = 1.1f;
	float mod_base = 0.4f;
	float prop_buffer = 1.f;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 5.428f; // ~38/7

	float jack_base = 2.f;
	float jack_min = 0.625f;
	float jack_max = 1.f;
	float jack_scaler = 1.f;

	float not_jack_pool = 1.2f;
	float not_jack_min = 0.4f;
	float not_jack_max = 1.f;
	float not_jack_scaler = 1.f;

	float quad_pool = 1.5f;
	float quad_min = 0.88f;
	float quad_max = 1.f;
	float quad_scaler = 1.f;

	float vibro_flag = 0.85f;

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

		{ "quad_pool", &quad_pool },
		{ "quad_min", &quad_min },
		{ "quad_max", &quad_max },
		{ "quad_scaler", &quad_scaler },

		{ "vibro_flag", &vibro_flag },
	};
#pragma endregion params and param map
	float total_prop = 0.f;
	float jack_prop = 0.f;
	float not_jack_prop = 0.f;
	float quad_prop = 0.f;
	float pmod = min_mod;
	float t_taps = 0.f;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[CJ], 1.f);
		Smooth(doot[CJQuad], 1.f);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline bool handle_case_optimizations(const itv_info& itv,
										  vector<float> doot[],
										  const size_t& i)
	{
		if (itv.total_taps == 0) {
			min_set(doot, i);
			return true;
		}

		// no chords
		if (itv.chord_taps == 0) {
			min_set(doot, i);
			return true;
		}
		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i)
	{
		const auto& itv = mni._itv_info;
		if (handle_case_optimizations(itv, doot, i))
			return;

		t_taps = static_cast<float>(itv.total_taps);

		// we have at least 1 chord we want to give a little leeway for single
		// taps but not too much or sections of [12]4[123] [123]4[23] will be
		// flagged as chordjack when they're really just broken chordstream, and
		// we also want to give enough leeway so that hyperdense chordjacks at
		// lower bpms aren't automatically rated higher than more sparse jacks
		// at higher bpms
		total_prop = static_cast<float>(itv.chord_taps + prop_buffer) /
					 (t_taps - prop_buffer) * total_prop_scaler;
		total_prop =
		  CalcClamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// make sure there's at least a couple of jacks
		jack_prop =
		  CalcClamp(itv.actual_jacks_cj - jack_base, jack_min, jack_max);

		// too many quads is either pure vibro or slow quadmash, downscale a bit
		quad_prop =
		  quad_pool -
		  (static_cast<float>(itv.taps_by_size[quad] * quad_scaler) / t_taps);
		quad_prop = CalcClamp(quad_prop, quad_min, quad_max);

		// explicitly detect broken chordstream type stuff so we can give more
		// leeway to single note jacks brop_two_return_of_brop_electric_bropaloo
		not_jack_prop = CalcClamp(
		  not_jack_pool -
			(static_cast<float>(itv.definitely_not_jacks * not_jack_scaler) /
			 t_taps),
		  not_jack_min,
		  not_jack_max);

		pmod = doot[CJ][i] =
		  CalcClamp(total_prop * jack_prop * not_jack_prop /* * quad_prop*/,
					min_mod,
					max_mod);

		// ITS JUST VIBRO THEN(unique note permutations per interval < 3 ), use
		// this other places ?
		if (itv.basically_vibro)
			pmod *= vibro_flag;

		// actual mod
		doot[_primary][i] = pmod;
		// look another actual mod
		doot[CJQuad][i] = quad_prop;

		// debug
		doot[CJS][i] = not_jack_prop;
		doot[CJJ][i] = jack_prop;
	}
};
struct AnchorMod
{

	const vector<int> _pmods = { Anchor };
	const std::string name = "AnchorMod";
	const int _primary = _pmods.front();

#pragma region params
	float min_mod = 0.9f;
	float max_mod = 1.1f;
	float mod_base = 0.3f;
	float buffer = 1.f;
	float scaler = 1.f;
	float other_scaler = 4.f;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },   { "max_mod", &max_mod },
		{ "mod_base", &mod_base }, { "buffer", &buffer },
		{ "scaler", &scaler },	 { "other_scaler", &other_scaler },
	};
#pragma endregion params and param map
	float l_taps = 0.f;
	float r_taps = 0.f;
	float pmod = min_mod;

#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], 1.f);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline bool handle_case_optimizations(const itv_info& itv,
										  vector<float> doot[],
										  const size_t& i)
	{
		// nothing here
		if (itv.total_taps == 0) {
			neutral_set(doot, i);
			return true;
		}

		// jack
		if (l_taps == 0.f || r_taps == 0.f) {
			neutral_set(doot, i);
			return true;
		}
		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i,
						   const int& hand)
	{
		const auto& itv = mni._itv_info;

		// left
		if (hand == 0) {
			l_taps = static_cast<float>(itv.col_taps[0]);
			r_taps = static_cast<float>(itv.col_taps[1]);
		} else {
			l_taps = static_cast<float>(itv.col_taps[2]);
			r_taps = static_cast<float>(itv.col_taps[3]);
		}

		if (handle_case_optimizations(itv, doot, i))
			return;

		pmod = static_cast<float>(min(l_taps, r_taps)) /
			   static_cast<float>(max(l_taps, r_taps));
		pmod = (mod_base + (buffer + (scaler / pmod)) / other_scaler);

		pmod = CalcClamp(pmod, min_mod, max_mod);

		// actual mod
		doot[_primary][i] = pmod;
	}
};
struct OHJumpMods
{
	bool dbg = true && debug_lmao;
	const vector<int> _pmods = { OHJumpMod,   OHJBaseProp, OHJPropComp,
								 OHJSeqComp,  OHJMaxSeq,   OHJCCTaps,
								 OHJHTaps,	CJOHJump,	CJOHJPropComp,
								 CJOHJSeqComp };
	const std::string name = "OHJumpMods";

#pragma region params
	float ohj_base = 0.15f;
	float ohj_min_mod = 0.5f;
	float ohj_max_mod = 1.f;
	float ohj_pow = 2.f;

	float ohj_max_seq_pool = 1.125f;
	float ohj_max_seq_scaler = 0.65f;
	float ohj_max_seq_jump_scaler = 2.5f;
	float ohj_max_seq_min = 0.f;
	float ohj_max_seq_max = 0.65f;

	float ohj_prop_pool = 1.2f;
	float ohj_prop_scaler = 0.35f;
	float ohj_prop_min = 0.f;
	float ohj_prop_max = 0.65f; // should be 0.35 but it was this before...

	float cj_ohj_base = 0.1f;
	float cj_ohj_min_mod = 0.6f;
	float cj_ohj_max_mod = 1.f;
	float cj_ohj_pow = 2.f;

	float cj_ohj_max_seq_pool = 1.2f;
	float cj_ohj_max_seq_scaler = 0.5f;
	float cj_ohj_max_seq_jump_scaler = 1.f;
	float cj_ohj_max_seq_min = 0.f;
	float cj_ohj_max_seq_max = 0.5f;

	float cj_ohj_prop_pool = 1.2f;
	float cj_ohj_prop_scaler = 0.5f;
	float cj_ohj_prop_min = 0.f;
	float cj_ohj_prop_max = 0.65f; // should be 0.35 but it was this before...

	const vector<pair<std::string, float*>> _params{
		{ "ohj_base", &ohj_base },
		{ "ohj_min_mod", &ohj_min_mod },
		{ "ohj_max_mod", &ohj_max_mod },
		{ "ohj_pow", &ohj_pow },

		{ "ohj_max_seq_pool", &ohj_max_seq_pool },
		{ "ohj_max_seq_scaler", &ohj_max_seq_scaler },
		{ "ohj_max_seq_jump_scaler", &ohj_max_seq_jump_scaler },
		{ "ohj_max_seq_min", &ohj_max_seq_min },
		{ "ohj_max_seq_max", &ohj_max_seq_max },

		{ "ohj_prop_pool", &ohj_prop_pool },
		{ "ohj_prop_scaler", &ohj_prop_scaler },
		{ "ohj_prop_min", &ohj_prop_min },
		{ "ohj_prop_max", &ohj_prop_max },

		{ "cj_ohj_base", &cj_ohj_base },
		{ "cj_ohj_min_mod", &cj_ohj_min_mod },
		{ "cj_ohj_max_mod", &cj_ohj_max_mod },
		{ "cj_ohj_pow", &cj_ohj_pow },

		{ "cj_ohj_max_seq_pool", &cj_ohj_max_seq_pool },
		{ "cj_ohj_max_seq_scaler", &cj_ohj_max_seq_scaler },
		{ "cj_ohj_max_seq_jump_scaler", &cj_ohj_max_seq_jump_scaler },
		{ "cj_ohj_max_seq_min", &cj_ohj_max_seq_min },
		{ "cj_ohj_max_seq_max", &cj_ohj_max_seq_max },

		{ "cj_ohj_prop_pool", &cj_ohj_prop_pool },
		{ "cj_ohj_prop_scaler", &cj_ohj_prop_scaler },
		{ "cj_ohj_prop_min", &cj_ohj_prop_min },
		{ "cj_ohj_prop_max", &cj_ohj_prop_max },
	};
#pragma endregion params and param map
	int cur_ohjump_seq = 0;
	int max_ohjump_seq = 0;
	int window_roll_taps = 0;
	int cc_taps = 0;
	int ohjump_taps = 0.f;
	// we cast the one in itv_info from int
	float hand_taps = 0.f;
	float floatymcfloatface = 0.f;
	float ohj_max_seq_component = 0.f;
	float ohj_prop_component = 0.f;
	float cj_ohj_max_seq_component = 0.f;
	float cj_ohj_prop_component = 0.f;
	float base_prop = 0.f;
	float pmod = ohj_min_mod;

	// non-empty (cc_type is now always non-empty)
	cc_type last_seen_cc = cc_init;
	cc_type last_last_seen_cc = cc_init;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		doot[OHJumpMod][i] = ohj_min_mod;
		doot[CJOHJump][i] = cj_ohj_min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[OHJumpMod], neutral);
		Smooth(doot[CJOHJump], neutral);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void complete_seq()
	{
		// negative values should not be possible
		ASSERT(cur_ohjump_seq >= 0);

		// set the largest ohj sequence
		max_ohjump_seq =
		  cur_ohjump_seq > max_ohjump_seq ? cur_ohjump_seq : max_ohjump_seq;
		// reset
		cur_ohjump_seq = 0;
	}

	inline void advance_sequencing(const metanoteinfo& now)
	{
		// do nothing for offhand taps
		if (now.col == col_empty)
			return;

		if (now.col == col_ohjump)
			ohjump_taps += 2;

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
				// at least 2 jumps in a row
				// boost if this is the beginning, otherwise just iterate
				if (cur_ohjump_seq == 0)
					++cur_ohjump_seq;
				++cur_ohjump_seq;
				break;
			case cc_jump_single:
				// just came out of a jump seq, do nothing... wait to see what
				// happens
				break;
			case cc_left_right:
			case cc_right_left:
				// track actual cc taps in a counter
				++cc_taps;

				// if we have an actual cross column tap now, and if we just
				// came from a jump -> single, then we have something like
				// [12]21, which is much harder than [12]22, so penalize the
				// sequence slightly before completing
				if (now.last_cc == cc_jump_single && cur_ohjump_seq > 1) {
					--cur_ohjump_seq;
					complete_seq();
				}
				break;
			case cc_single_single:
				// we have something like [12]22, complete the sequence
				// without the penalty that the cross column incurs
				if (now.last_cc == cc_jump_single)
					complete_seq();
				break;
			case cc_single_jump:
				// [12]1[12]... we broke a sequence and went right back into
				// one.. reset sequence for now but come back to revsit this, we
				// might want to have different behavior, but we'd need to track
				// the columns of the single notes in the chain
				if (now.last_cc == cc_jump_single)
					complete_seq();
				else
					complete_seq();
				break;
			case cc_init:
				break;
			default:
				break;
		}
	}

	inline bool handle_case_optimizations(vector<float> doot[], const size_t& i)
	{
		if (floatymcfloatface >= hand_taps) {
			min_set(doot, i);
			set_debug_output(doot, i);
			return true;
		}

		// nothing here
		if (hand_taps == 0) {
			neutral_set(doot, i);
			set_debug_output(doot, i);
			return true;
		}

		// nan's
		if (cj_ohj_max_seq_pool <= base_prop * cj_ohj_max_seq_jump_scaler) {
			min_set(doot, i);
			set_debug_output(doot, i);
			return true;
		}

		// no ohjumps
		if (ohjump_taps == 0) {
			neutral_set(doot, i);
			set_debug_output(doot, i);
			return true;
		}

		// no repeated oh jumps, prop scale only
		if (max_ohjump_seq == 0) {
			ohj_prop_component = ohj_prop_pool - (ohjump_taps / hand_taps);
			ohj_prop_component =
			  CalcClamp(ohj_prop_component, ohj_min_mod, ohj_max_mod);

			pmod = fastsqrt(ohj_prop_component);
			pmod = CalcClamp(ohj_base + pmod, ohj_min_mod, ohj_max_mod);
			doot[OHJumpMod][i] = pmod;

			cj_ohj_prop_component =
			  fastsqrt(cj_ohj_prop_pool - (ohjump_taps / hand_taps / 2.f));
			cj_ohj_prop_component =
			  CalcClamp(cj_ohj_prop_component, cj_ohj_min_mod, cj_ohj_max_mod);

			pmod = fastsqrt(cj_ohj_prop_component);
			pmod =
			  CalcClamp(cj_ohj_base + pmod, cj_ohj_min_mod, cj_ohj_max_mod);
			doot[CJOHJump][i] = pmod;

			set_debug_output(doot, i);
			return true;
		}

		// if this is true we have some combination of single notes
		// and jumps where the single notes are all on the same
		// column
		if (cc_taps == 0) {
			// we don't want to treat 2[12][12][12]2222 2222[12][12][12]2
			// differently, so use the max sequence here exclusively
			if (max_ohjump_seq > 0) {
				// yea i thought we might need to tune ohj downscalers for js
				// and cj slightly differently
				ohj_max_seq_component =
				  pow(hand_taps / (floatymcfloatface * ohj_max_seq_jump_scaler),
					  ohj_pow);
				ohj_max_seq_component =
				  CalcClamp(ohj_max_seq_component, ohj_min_mod, ohj_max_mod);
				pmod = ohj_max_seq_component;

				doot[OHJumpMod][i] = pmod;

				// ohjumps in cj can be either easier or harder
				// depending on context.. so we have to pull back a
				// bit so it doesn't swing too far when it shouldn't
				cj_ohj_max_seq_component =
				  pow(hand_taps / (floatymcfloatface *
								   (cj_ohj_max_seq_jump_scaler + 0.4f)),
					  cj_ohj_pow);
				cj_ohj_max_seq_component = CalcClamp(
				  cj_ohj_max_seq_component, cj_ohj_min_mod, cj_ohj_max_mod);
				pmod = cj_ohj_max_seq_component;
				doot[CJOHJump][i] = pmod;

				set_debug_output(doot, i);
				return true;
			} else {
				// single note longjacks, do nothing
				neutral_set(doot, i);
				return true;
			}
		}

		return false;
	}

	inline void interval_reset()
	{
		// reset any interval stuff here
		cc_taps = 0;
		ohjump_taps = 0;
		max_ohjump_seq = 0;
		ohj_max_seq_component = neutral;
		ohj_prop_component = neutral;
		cj_ohj_max_seq_component = neutral;
		cj_ohj_prop_component = neutral;
	}

	inline void set_debug_output(vector<float> doot[], const size_t& i)
	{
		doot[OHJSeqComp][i] = ohj_max_seq_component;
		doot[OHJPropComp][i] = ohj_prop_component;

		doot[CJOHJSeqComp][i] = cj_ohj_max_seq_component;
		doot[CJOHJPropComp][i] = cj_ohj_prop_component;

		doot[OHJBaseProp][i] = base_prop;
		doot[OHJMaxSeq][i] = floatymcfloatface;
		doot[OHJCCTaps][i] = cc_taps;
		doot[OHJHTaps][i] = hand_taps;
	}

	void operator()(const metanoteinfo& mni,
					vector<float> doot[],
					const size_t& i,
					const int& hand)
	{
		const auto& itv = mni._itv_info;

		// if cur_seq > max when we ended the interval, set it, but don't reset
		max_ohjump_seq =
		  cur_ohjump_seq > max_ohjump_seq ? cur_ohjump_seq : max_ohjump_seq;

		floatymcfloatface = static_cast<float>(max_ohjump_seq);
		hand_taps = static_cast<float>(itv.hand_taps[hand]);
		base_prop = floatymcfloatface / hand_taps;

		// handle simple cases first, execute this block if nothing easy is
		// detected, fill out non-component debug info and handle interval
		// resets at end
		if (handle_case_optimizations(doot, i)) {
			interval_reset();
			return;
		}

		// STANDARD OHJ
		// for js we lean into max sequences more, since they're better
		// indicators of inflated difficulty
		ohj_max_seq_component =
		  ohj_max_seq_scaler *
		  (ohj_max_seq_pool - (base_prop * ohj_max_seq_jump_scaler));
		ohj_max_seq_component =
		  CalcClamp(ohj_max_seq_component, ohj_max_seq_min, ohj_max_seq_max);

		ohj_prop_component = ohj_prop_scaler * (ohj_prop_pool - base_prop);
		ohj_prop_component =
		  CalcClamp(ohj_prop_component, ohj_prop_min, ohj_prop_max);

		pmod = fastsqrt(ohj_max_seq_component + ohj_prop_component);
		pmod = CalcClamp(ohj_base + pmod, ohj_min_mod, ohj_max_mod);
		doot[OHJumpMod][i] = pmod;

		// CH OHJ
		// we want both the total number of jumps and the max
		// sequence to count here, with more emphasis on the max
		// sequence, sequence should be multiplied by 2 (or
		// maybe slightly more?)
		cj_ohj_max_seq_component =
		  cj_ohj_max_seq_scaler *
		  fastsqrt(cj_ohj_max_seq_pool -
				   (base_prop * cj_ohj_max_seq_jump_scaler));
		cj_ohj_max_seq_component = CalcClamp(
		  cj_ohj_max_seq_component, cj_ohj_max_seq_min, cj_ohj_max_seq_max);

		cj_ohj_prop_component =
		  cj_ohj_prop_scaler * fastsqrt(cj_ohj_prop_pool - base_prop);
		cj_ohj_prop_component =
		  CalcClamp(cj_ohj_prop_component, cj_ohj_prop_min, cj_ohj_prop_max);

		pmod = fastsqrt(cj_ohj_max_seq_component + cj_ohj_prop_component);
		pmod = CalcClamp(cj_ohj_base + pmod, cj_ohj_min_mod, cj_ohj_max_mod);
		doot[CJOHJump][i] = pmod;

		set_debug_output(doot, i);

		interval_reset();
	}
};
// almost identical to wrr, refer to comments there
struct OHTrillMod
{
	bool dbg = true && debug_lmao;
	const vector<int> _pmods = { OHTrill };
	const std::string name = "OHTrillMod";
	const int _primary = _pmods.front();

	deque<int> window_itv_hand_taps;
	deque<vector<int>> window_itv_trills;

#pragma region params
	float itv_window = 2;

	float min_mod = 0.5f;
	float max_mod = 1.f;
	float mod_pool = 1.25f;

	float moving_cv_init = 0.5f;
	float trill_cv_cutoff = 0.25f;

	const vector<pair<std::string, float*>> _params{
		{ "itv_window", &itv_window },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_pool", &mod_pool },

		{ "moving_cv_init", &moving_cv_init },
		{ "trill_cv_cutoff", &trill_cv_cutoff },
	};
#pragma endregion params and param map
	vector<int> itv_trills;
	int itv_hand_taps = 0;

	bool trilling = false;
	// dunno if we want this for ohts
	// bool is_transition = false;
	int consecutive_trill_counter = 0;

	int window_hand_taps = 0;
	int window_trill_taps = 0;
	float pmod = min_mod;

	vector<float> seq_ms = { 0.f, 0.f, 0.f };
	float moving_cv = moving_cv_init;

	// non-empty (cc_type is now always non-empty)
	cc_type last_seen_cc = cc_init;
	cc_type last_last_seen_cc = cc_init;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], neutral);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void reset_sequence()
	{
		// only need to do this if trilling, otherwise values are false/0 anyway
		if (trilling) {
			itv_trills.push_back(consecutive_trill_counter);
			trilling = false;
			consecutive_trill_counter = 0;
		}

		last_seen_cc = cc_init;
		last_last_seen_cc = cc_init;
		for (auto& v : seq_ms)
			v = 0.f;
	}

	inline bool handle_trill_timing_check()
	{
		// the primary difference from wrr, just check cv on the base ms values,
		// we are looking for values that are all close together with no
		// manipulation
		moving_cv = (moving_cv + cv(seq_ms)) / 2.f;
		return moving_cv < trill_cv_cutoff;
	}

	inline void update_seq_ms(const metanoteinfo& now)
	{
		seq_ms[0] = seq_ms[1]; // last_last
		seq_ms[1] = seq_ms[2]; // last
		seq_ms[2] = now.cc_ms_any;
	}

	inline void advance_sequencing(const metanoteinfo& now)
	{
		// do nothing for offhand taps
		if (now.col == col_empty)
			return;

		// only let these cases through, don't need cc_single_single like wrr
		if (now.cc != cc_left_right && now.cc != cc_right_left) {
			reset_sequence();
			return;
		}

		// update timing stuff
		update_seq_ms(now);

		// check for a complete sequence
		if (last_last_seen_cc != cc_init)
			// check for trills (cc -> inverted(cc) -> cc)
			if (now.mt == meta_oht && handle_trill_timing_check()) {
				++consecutive_trill_counter;
				if (!trilling) {
					// boost slightly because we want to pick up minitrills
					// maybe
					++consecutive_trill_counter;
					trilling = true;
				}
			}

		// update sequence
		last_last_seen_cc = last_seen_cc;
		last_seen_cc = now.cc;
	}

	inline bool handle_case_optimizations(vector<float> doot[], const size_t& i)
	{
		if (window_hand_taps == 0 || window_trill_taps == 0) {
			neutral_set(doot, i);
			return true;
		} else if (window_hand_taps == window_trill_taps) {
			min_set(doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i,
						   const int& hand)
	{
		const auto& itv = mni._itv_info;
		itv_hand_taps = itv.hand_taps[hand];

		// drop the oldest interval values if we have reached full
		// size
		if (window_itv_hand_taps.size() == itv_window) {
			window_itv_hand_taps.pop_front();
			window_itv_trills.pop_front();
		}

		// this is slightly hacky buuut if we have a trill that doesn't complete
		// by the end of the interval, it should count for that interval, but we
		// don't want the value to double up so we will reset the counter on
		// interval end but _not_ reset the trilling bool, so it won't interfere
		// with the detection as the sequencing passes into the next interval,
		// and won't double up values
		if (consecutive_trill_counter > 0) {
			itv_trills.push_back(consecutive_trill_counter);
			consecutive_trill_counter = 0;
		}

		window_itv_hand_taps.push_back(itv_hand_taps);
		window_itv_trills.push_back(itv_trills);

		window_hand_taps = 0;
		for (auto& n : window_itv_hand_taps)
			window_hand_taps += n;

		window_trill_taps = 0;
		// for now just add everything up
		for (auto& n : window_itv_trills)
			for (auto& v : n)
				window_trill_taps += v;

		if (handle_case_optimizations(doot, i)) {
			interval_reset();
			return;
		}

		pmod = max_mod;
		if (window_trill_taps > 0 && window_hand_taps > 0)
			pmod = mod_pool - (static_cast<float>(window_trill_taps - 4) /
							   static_cast<float>(window_hand_taps * 4));

		pmod = CalcClamp(pmod, min_mod, max_mod);
		doot[_primary][i] = pmod;

		interval_reset();
	}

	inline void interval_reset()
	{
		itv_trills.clear();
		itv_hand_taps = 0;
	}
};
struct RunningManMod
{
	const vector<int> _pmods{ RanMan,		 RanLen,	  RanAnchLen,
							  RanAnchLenMod, RanJack,	 RanOHT,
							  RanOffS,		 RanPropAll,  RanPropOff,
							  RanPropOHT,	RanPropOffS, RanPropJack };
	const std::string name = "RunningManMod";
	const int _primary = _pmods.front();

	RM_Sequencing rms[2];
	RM_Sequencing interval_highest;

#pragma region params
	float min_mod = 0.95f;
	float max_mod = 1.35f;
	float mod_base = 0.8f;
	float min_anchor_len = 5.f;
	float min_taps_in_rm = 1.f;
	float min_off_taps_same = 1.f;

	float total_prop_scaler = 1.f;
	float total_prop_min = 0.f;
	float total_prop_max = 1.f;

	float off_tap_prop_scaler = 1.3f;
	float off_tap_prop_min = 0.f;
	float off_tap_prop_max = 1.25f;
	float off_tap_prop_base = 0.05f;

	float off_tap_same_prop_scaler = 1.f;
	float off_tap_same_prop_min = 0.f;
	float off_tap_same_prop_max = 1.25f;
	float off_tap_same_prop_base = 0.25f;

	float anchor_len_divisor = 2.5f;

	float min_jack_taps_for_bonus = 1.f;
	float jack_bonus_base = 0.1f;

	float min_oht_taps_for_bonus = 1.f;
	float oht_bonus_base = 0.1f;

	// params for rm_sequencing, these define conditions for resetting
	// runningmen sequences
	float max_oht_len = 2.f;
	float max_off_spacing = 2.f;
	float max_burst_len = 6.f;
	float max_jack_len = 1.f;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },

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
	float total_prop = 0.f;
	float off_tap_prop = 0.f;
	float off_tap_same_prop = 0.f;
	float anchor_len_comp = 0.f;
	float jack_bonus = 0.f;
	float oht_bonus = 0.f;
	float pmod = min_mod;
	int test = 0;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		// don't try to figure out which column a prospective anchor is on, just
		// run two passes with each assuming a different column
		rms[0].anchor_col = col_left;
		rms[1].anchor_col = col_right;
		rms[0].set_params(
		  max_oht_len, max_off_spacing, max_burst_len, max_jack_len);
		rms[1].set_params(
		  max_oht_len, max_off_spacing, max_burst_len, max_jack_len);

		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], 1.f);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	inline void advance_sequencing(const metanoteinfo& mni)
	{
		for (auto& rm : rms)
			rm(mni);

		// use the biggest anchor that has existed in this interval
		test = rms[0].anchor_len > rms[1].anchor_len ? 0 : 1;

		if (rms[test].anchor_len > interval_highest.anchor_len)
			interval_highest = rms[test];
	}

	inline bool handle_case_optimizations(const RM_Sequencing& rm,
										  vector<float> doot[],
										  const size_t& i)
	{
		// we could mni check for empty intervals like the other mods but it
		// doesn't really matter and this is probably more useful for debug
		// output

		// we could decay in this but it may conflict/be redundant with how
		// runningmen sequences are constructed, if decays are used we would
		// probably generate the mod not from the highest of any interval, but
		// from whatever sequences are still alive by the end
		if (rm.anchor_len < min_anchor_len) {
			min_set(doot, i);
			return true;
		} else if (rm.ran_taps < min_taps_in_rm) {
			min_set(doot, i);
			return true;
		} else if (rm.off_taps_same < min_off_taps_same) {
			min_set(doot, i);
			return true;
		}
		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i)
	{
		const auto& rm = interval_highest;
		if (handle_case_optimizations(rm, doot, i))
			return;

		// the pmod template stuff completely broke the js/hs/cj mods.. so..
		// these might also be broken... investigate later

		// taps in runningman / total taps in interval... i think? can't
		// remember when i reset total taps tbh.. this might be useless
		total_prop = pmod_prop(rm.ran_taps,
							   rm.total_taps,
							   total_prop_scaler,
							   total_prop_min,
							   total_prop_max);

		// number anchor taps / number of non anchor taps
		off_tap_prop = fastpow(pmod_prop(rm.anchor_len,
										 rm.ran_taps,
										 off_tap_prop_scaler,
										 off_tap_prop_min,
										 off_tap_prop_max,
										 off_tap_prop_base),
							   2.f);

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
		  rm.jack_taps >= min_jack_taps_for_bonus ? jack_bonus_base : 0.f;

		// ohts in anchor component, give a small bonus i guess
		// not done
		oht_bonus =
		  rm.oht_taps >= min_oht_taps_for_bonus ? oht_bonus_base : 0.f;

		// we could scale the anchor to speed if we want but meh
		// that's really complicated/messy/error prone
		pmod = anchor_len_comp + jack_bonus + oht_bonus + mod_base;
		pmod = CalcClamp(
		  fastsqrt(pmod * total_prop * off_tap_prop /** off_tap_same_prop*/),
		  min_mod,
		  max_mod);

		// actual used mod
		doot[_primary][i] = pmod;

		// debug
		if (debug_lmao) {
			doot[RanLen][i] =
			  (static_cast<float>(rm.total_taps) / 100.f) + 0.5f;
			doot[RanAnchLen][i] =
			  (static_cast<float>(rm.anchor_len) / 30.f) + 0.5f;
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

		// reset interval highest when we're done
		interval_highest.reset();
	}
};
// probably needs better debugoutput
struct WideRangeJumptrillMod
{
	bool dbg = false;
	const vector<int> _pmods = { WideRangeJumptrill };
	const std::string name = "WideRangeJumptrillMod";
	const int _primary = _pmods.front();

	deque<int> itv_taps;
	deque<int> itv_ccacc;

#pragma region params
	float itv_window = 3;

	float min_mod = 0.25f;
	float max_mod = 1.f;
	float mod_base = 0.4f;

	float moving_cv_init = 0.5f;
	float ccacc_cv_cutoff = 0.5f;

	const vector<pair<std::string, float*>> _params{
		{ "itv_window", &itv_window },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },

		{ "moving_cv_init", &moving_cv_init },
		{ "ccacc_cv_cutoff", &ccacc_cv_cutoff },
	};
#pragma endregion params and param map
	int ccacc_counter = 0;
	int crop_circles = 0;
	float pmod = min_mod;
	int window_taps = 0;
	int window_ccacc = 0;

	vector<float> seq_ms = { 0.f, 0.f, 0.f };
	// uhhh lazy way out of tracking all the floats i think
	float moving_cv = moving_cv_init;

	// non-empty
	cc_type last_seen_cc = cc_init;
	cc_type last_last_seen_cc = cc_init;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], 1.f);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion
	inline void reset_sequence()
	{
		last_seen_cc = cc_init;
		last_last_seen_cc = cc_init;
		for (auto& v : seq_ms)
			v = 0.f;
	}

	// should maybe move this into metanoteinfo and do the counting there, if we
	// could use this anywhere else
	inline bool detecc_ccacc(const metanoteinfo& now)
	{
		// if we're here the following are true, we have a full sequence of 3 cc
		// taps, they are non-empty, and there are no jumps. this means they are
		// all either cc_left_right, cc_right_left, or cc_single_single

		// handle cc_single_single first, for now, lets throw it away
		if (now.cc == cc_single_single)
			return false;

		// now we know we have cc_left_right or cc_right_left, so, xy, we are
		// looking for xyyx, meaning last_last would be the inverion of now
		if (invert_cc(now.cc) == last_last_seen_cc)
			return true;

		return false;
	}

	inline bool handle_ccacc_timing_check()
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

		// doing this would be a problem if we were to try to catch
		// anchor/cc/anchor, since the altered anchor ms value would still exist
		// in the sequence, but that maybe doesn't seem like a great idea
		// anyway, so the value we alter will fall out of the array by the time
		// we get here next

		seq_ms[1] /= 3.f;

		// this may be too fancy even though i'm trying to be not fancy.. update
		// a basic moving window of the cv values and return true if it's below
		// some cutoff, this will have the effect of giving a little lag time
		// before the mod really kicks in
		moving_cv = (moving_cv + cv(seq_ms)) / 2.f;
		return moving_cv < ccacc_cv_cutoff;
	}

	inline void update_seq_ms(const metanoteinfo& now)
	{
		seq_ms[0] = seq_ms[1]; // last_last
		seq_ms[1] = seq_ms[2]; // last

		// update now
		// for anchors, track tc_ms
		if (now.cc == cc_single_single)
			seq_ms[2] = now.tc_ms;
		// for cc_left_right or cc_right_left, track cc_ms
		else
			seq_ms[2] = now.cc_ms_any;
	}

	inline void advance_sequencing(const metanoteinfo& now)
	{
		// do nothing for offhand taps
		if (now.col == col_empty)
			return;

		// reset if we hit a jump
		if (now.col == col_ohjump) {
			reset_sequence();
			return;
		}

		// update timing stuff before checking/updating the sequence...
		// because.. idk why.. jank i guess, this _seems_ to work, don't know if
		// it _actually_ works
		update_seq_ms(now);

		// check for a complete sequence
		if (last_last_seen_cc != cc_init)
			// check for ccacc
			if (detecc_ccacc(now))
				// don't bother adding if the ms values look benign
				if (handle_ccacc_timing_check())
					++ccacc_counter;

		// update sequence
		last_last_seen_cc = last_seen_cc;
		last_seen_cc = now.cc;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i)
	{
		const auto& itv = mni._itv_info;
		// drop the oldest interval values if we have reached full
		// size
		if (itv_taps.size() == itv_window) {
			itv_taps.pop_front();
			itv_ccacc.pop_front();
		}

		itv_taps.push_back(itv.total_taps);
		itv_ccacc.push_back(ccacc_counter);

		if (ccacc_counter > 0)
			++crop_circles;
		else
			--crop_circles;
		if (crop_circles < 0)
			crop_circles = 0;

		for (auto& n : itv_taps)
			window_taps += n;

		for (auto& n : itv_ccacc)
			window_ccacc += n;

		pmod = neutral;
		if (window_ccacc > 0 && crop_circles > 0)
			pmod =
			  static_cast<float>(window_taps) /
			  static_cast<float>(window_ccacc * (1 + max(crop_circles, 5)));

		pmod = CalcClamp(pmod, min_mod, max_mod);
		doot[_primary][i] = pmod;

		// we could count these in metanoteinfo but let's do it here for now,
		// reset every interval when finished
		ccacc_counter = 0;
	}
};
// if ccacc is cross column, anchor, cross column (1221) and we are looking for
// (1212) then we are looking for cccccc where the inner cc is the inverse of
// the outers, so we'll follow the rough model that wrjt setup, conveniently
// the inner timing ratio to the outer timings should also be 3:1 when looking
// for proper rolls, it would be 1:1 for ohts. given the much more efficient
// new setup we can do roll detection and oht detection in separate passes
// this is for PURE ROLLS ONLY, not rolls with maybe some jumps in it, that can
// be done in another pass
// refer to cccccc with an inverted center as "roll" formation, even though
// technically it can either be roll or oht depending on spacing
struct WideRangeRollMod
{
	bool dbg = true && debug_lmao;
	const vector<int> _pmods = { WideRangeRoll, Chaos };
	const std::string name = "WideRangeRollMod";
	const int _primary = _pmods.front();

	// taps for this hand only, we don't want to include offhand taps in
	// determining whether this hand is a roll
	deque<int> window_itv_hand_taps;
	deque<vector<int>> window_itv_rolls;

#pragma region params
	float itv_window = 4;

	float min_mod = 0.25f;
	float max_mod = 1.035f;
	float mod_pool = 1.15f;

	float moving_cv_init = 0.5f;
	float roll_cv_cutoff = 0.5f;

	const vector<pair<std::string, float*>> _params{
		{ "itv_window", &itv_window },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_pool", &mod_pool },

		{ "moving_cv_init", &moving_cv_init },
		{ "roll_cv_cutoff", &roll_cv_cutoff },
	};
#pragma endregion params and param map
	// each element is a discrete roll formation with this many taps
	// (technically it has this many taps + 4 because it requires 1212 or
	// 2121 to start counting, but that's fine, that's what we want and if
	// it seems better to add later we can do that
	vector<int> itv_rolls;
	int itv_hand_taps = 0;

	// unlike ccacc, which has a half baked implementation for chains of
	// 122112211221, we will actually be responsible and sequence both the
	// number of rolls and the notes contained therein
	bool rolling = false;
	bool is_transition = false;
	int consecutive_roll_counter = 0;

	int window_hand_taps = 0;
	// for now we will be lazy and just add up the number of roll taps in any
	// roll, if we leave out the initialization taps (the 4 required to identify
	// the start) we will greatly reduce the effect of short roll bursts, not
	// sure if this is desired behavior
	int window_roll_taps = 0;
	float pmod = min_mod;

	vector<float> seq_ms = { 0.f, 0.f, 0.f };
	// uhhh lazy way out of tracking all the floats i think
	float moving_cv = moving_cv_init;

	// non-empty (cc_type is now always non-empty)
	cc_type last_seen_cc = cc_init;
	cc_type last_last_seen_cc = cc_init;
#pragma region generic functions
	inline void setup(vector<float> doot[], const size_t& size)
	{
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	}

	inline void min_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = min_mod;
	}

	inline void neutral_set(vector<float> doot[], const size_t& i)
	{
		for (auto& mod : _pmods)
			doot[mod][i] = neutral;
	}

	inline void smooth_finish(vector<float> doot[])
	{
		Smooth(doot[_primary], neutral);
	}

	inline XNode* make_param_node() const
	{
		XNode* pmod = new XNode(name);
		for (auto& p : _params)
			pmod->AppendChild(p.first, to_string(*p.second));

		return pmod;
	}

	inline void load_params_from_node(const XNode* node)
	{
		float boat = 0.f;
		auto* pmod = node->GetChild(name);
		if (pmod == NULL)
			return;
		for (auto& p : _params) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == NULL)
				continue;

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#pragma endregion

	// should rename as it resets or completes a sequence... maybe should go
	// look at rm_sequencing again and make roll_sequencing.. idk
	inline void reset_sequence()
	{
		// only need to do this if rolling, otherwise values are false/0 anyway
		if (rolling) {
			itv_rolls.push_back(consecutive_roll_counter);
			rolling = false;
			consecutive_roll_counter = 0;
		}

		last_seen_cc = cc_init;
		last_last_seen_cc = cc_init;
		for (auto& v : seq_ms)
			v = 0.f;
	}

	// copied from wrjt, definitely needs to be tracked in metanoteinfo
	inline bool detecc_ccacc(const metanoteinfo& now)
	{
		if (now.cc == cc_single_single)
			return false;

		if (invert_cc(now.cc) == last_last_seen_cc)
			return true;

		return false;
	}

	// should maybe move this into metanoteinfo and do the counting there, since
	// oht will need this as well, or we could be lazy and do it twice just this
	// once
	inline bool detecc_roll(const metanoteinfo& now)
	{
		// we allow this through up to here due to transition checks
		if (now.cc == cc_single_single)
			return false;

		// if we're here the following are true, we have a full sequence of 3 cc
		// taps, they are non-empty, there are no jumps and no anchors. this
		// means they are all either cc_left_right, cc_right_left

		// now we know we have cc_left_right or cc_right_left, so, xy, we are
		// looking for xyx, meaning last would be the inverion of now
		if (invert_cc(now.cc) == last_seen_cc)
			// now make sure that last_last is the same as now
			if (now.cc == last_last_seen_cc)
				// we now have 1212 or 2121
				return true;
		return false;
	}

	inline bool handle_roll_timing_check()
	{
		// see ccacc timing check in wrjt for explanations, it's basically the
		// same but we have to invert the multiplication depending on which
		// value is higher between seq_ms[0] and seq_ms[1] (easiest to dummy up
		// a roll in an editor to see why)

		// multiply seq_ms[1] by 3 for the cv check, then put it back so it
		// doesn't interfere with the next round
		if (seq_ms[0] > seq_ms[1]) {
			seq_ms[1] *= 3.f;
			moving_cv = (moving_cv + cv(seq_ms)) / 2.f;
			seq_ms[1] /= 3.f;
			return moving_cv < roll_cv_cutoff;
		} else {
			// same thing but divide
			seq_ms[1] /= 3.f;
			moving_cv = (moving_cv + cv(seq_ms)) / 2.f;
			seq_ms[1] *= 3.f;
			return moving_cv < roll_cv_cutoff;
		}
	}

	inline void update_seq_ms(const metanoteinfo& now)
	{
		seq_ms[0] = seq_ms[1]; // last_last
		seq_ms[1] = seq_ms[2]; // last

		// update now, we have no anchors, so always use cc_ms_any (although we
		// want to move this to cc_ms_no_jumps when that gets implemented, since
		// a separate jump inclusive mod should be made to handle those cases
		seq_ms[2] = now.cc_ms_any;
	}

	inline void advance_sequencing(const metanoteinfo& now)
	{
		// do nothing for offhand taps
		if (now.col == col_empty)
			return;

		// only let these cases through, since we use invert_cc, anchors are
		// screened out later, reset otherwise
		if (now.cc != cc_left_right && now.cc != cc_right_left &&
			now.cc != cc_single_single) {
			reset_sequence();
			return;
		}

		// update timing stuff
		update_seq_ms(now);

		is_transition = false;
		// try to catch simple transitions https:i.imgur.com/zhlBio0.png, given
		// the constraints on ccacc we have to check last_cc for the anchor
		if (now.last_cc == cc_single_single)
			if (detecc_ccacc(now)) {
				if (rolling) {
					// don't care about any timing checks for the moment
					is_transition = true;
					++consecutive_roll_counter;
				}
			}

		// check for a complete sequence
		if (last_last_seen_cc != cc_init)
			// check for rolls (cc -> inverted(cc) -> cc)
			if (now.mt == meta_oht && handle_roll_timing_check()) {
				if (rolling) {
					// these should always be mutually exclusive
					ASSERT(is_transition == false);
					++consecutive_roll_counter;
				} else {
					// we could increase the roll counter here, but really
					// all we have now is a minitrill, so lets see if it
					// extends to at least 5 notes before doing anything
					rolling = true;
				}
				// only reset here if this fails and a transition wasn't
				// detected, if we reset here we have to assign seq_ms[2] again,
				// yes this is asofgasfjasofdj messy
			} else if (!is_transition) {
				reset_sequence();
				seq_ms[2] = now.cc_ms_any;
			}

		// update sequence
		last_last_seen_cc = last_seen_cc;
		last_seen_cc = now.cc;
	}

	inline bool handle_case_optimizations(vector<float> doot[], const size_t& i)
	{
		if (window_hand_taps == 0 || window_roll_taps == 0) {
			neutral_set(doot, i);
			return true;
		} else if (window_hand_taps == window_roll_taps) {
			min_set(doot, i);
			return true;
		}

		return false;
	}

	inline void operator()(const metanoteinfo& mni,
						   vector<float> doot[],
						   const size_t& i,
						   const int& hand)
	{
		const auto& itv = mni._itv_info;
		itv_hand_taps = itv.hand_taps[hand];

		// drop the oldest interval values if we have reached full
		// size
		if (window_itv_hand_taps.size() == itv_window) {
			window_itv_hand_taps.pop_front();
			window_itv_rolls.pop_front();
		}

		// this is slightly hacky buuut if we have a roll that doesn't complete
		// by the end of the interval, it should count for that interval, but we
		// don't want the value to double up so we will reset the counter on
		// interval end but _not_ reset the rolling bool, so it won't interfere
		// with the detection as the sequencing passes into the next interval,
		// and won't double up values
		if (consecutive_roll_counter > 0) {
			itv_rolls.push_back(consecutive_roll_counter);
			consecutive_roll_counter = 0;
		}

		window_itv_hand_taps.push_back(itv_hand_taps);
		window_itv_rolls.push_back(itv_rolls);

		window_hand_taps = 0;
		for (auto& n : window_itv_hand_taps)
			window_hand_taps += n;

		window_roll_taps = 0;
		// for now just add everything up
		for (auto& n : window_itv_rolls)
			for (auto& v : n)
				window_roll_taps += v;

		if (handle_case_optimizations(doot, i)) {
			interval_reset();
			return;
		}

		pmod = max_mod;
		if (window_roll_taps > 0 && window_hand_taps > 0)
			pmod = mod_pool - (static_cast<float>(window_roll_taps) /
							   static_cast<float>(window_hand_taps));

		pmod = CalcClamp(pmod, min_mod, max_mod);
		doot[_primary][i] = pmod;

		// temp hack because i didn't want to port chaos mod but didn't want to
		// keep running old wrr
		doot[Chaos][i] = neutral;

		interval_reset();
	}

	// may be unneeded for this function but it's probably good practice to have
	// this and always reset anything that needs to be on handling case
	// optimizations, even if the case optimizations don't require us to reset
	// anything
	inline void interval_reset()
	{
		itv_rolls.clear();
		itv_hand_taps = 0;
	}
};


#pragma endregion
struct TheGreatBazoinkazoinkInTheSky
{
	bool dbg = false;
	// debug stuff, tracks everything that was built
	vector<vector<metanoteinfo>> _mni_dbg_vec1;
	vector<vector<metanoteinfo>> _mni_dbg_vec2;

	// for generic debugging, constructs a string with the pattern formation for
	// a given interval
	vector<std::string> _itv_row_string;

	// basic data we need
	vector<NoteInfo> _ni;
	vector<vector<int>> _itv_rows;
	vector<float>* _doot;
	float _rate = 0.f;
	int hand = 0;
	unsigned _t1 = 0;
	unsigned _t2 = 0;

	// to produce these
	unique_ptr<metanoteinfo> _mni_last;
	unique_ptr<metanoteinfo> _mni_now;
	metanoteinfo _dbg;

	// so we can make pattern mods
	StreamMod _s;
	JSMod _js;
	HSMod _hs;
	CJMod _cj;
	AnchorMod _anch;
	OHJumpMods _ohj;
	OHTrillMod _oht;
	RunningManMod _rm;
	WideRangeJumptrillMod _wrjt;
	WideRangeRollMod _wrr;

	// we only care what last is, not what now is, this should work but it
	// seems almost too clever but seems to work
	inline void set_mni_last() { std::swap(_mni_last, _mni_now); }

	inline void bazoink(const vector<NoteInfo>& ni)
	{
		//load_params_from_disk();

		// ok so the problem atm is the multithreading of songload, if we want
		// to update the file on disk with new values and not just overwrite it
		// we have to write out after loading the values player defined, so the
		// quick hack solution to do that is to only do it during debug output
		// generation, which is fine for the time being, though not ideal
		if (debug_lmao)
			write_params_to_disk();

		_mni_last = std::make_unique<metanoteinfo>();
		_mni_now = std::make_unique<metanoteinfo>();

		// doesn't change with offset or anything
		_ni = ni;
	}

	inline void operator()(const vector<vector<int>>& itv_rows,
						   const float& rate,
						   const unsigned int& t1,
						   const unsigned int& t2,
						   vector<float> doot[])
	{
		// change with offset, if we do multi offset passes we want this to
		// be vars, but we aren't doing it now
		_rate = rate;
		_itv_rows = itv_rows;
		_doot = doot;

		// changes with hand
		_t1 = t1;
		_t2 = t2;

		// askdfjhaskjhfwe
		if (_t1 == col_ids[0])
			hand = 0;
		else
			hand = 1;

		// run any setup functions for pattern mods, generally memory
		// initialization and maybe some other stuff
		run_pattern_mod_setups();

		if (dbg) {
			// asfasdfasjkldf, keep track by hand i guess, since values for
			// hand dependent mods would have been overwritten without using
			// a pushback, and pushing back 2 cycles of metanoteinfo into
			// the same debug vector is kinda like... not good
			if (hand == 0) {
				// left hand stuffies
				_mni_dbg_vec1.resize(_itv_rows.size());
				for (size_t itv = 0; itv < _itv_rows.size(); ++itv)
					_mni_dbg_vec1[itv].reserve(_itv_rows[itv].size());
			} else {
				// right hand stuffies
				_mni_dbg_vec2.resize(_itv_rows.size());
				for (size_t itv = 0; itv < _itv_rows.size(); ++itv)
					_mni_dbg_vec2[itv].reserve(_itv_rows[itv].size());
			}
		}

		// above block is controlled in the struct def, this block is run if we
		// are called from minacalcdebug, allocate the string thing, we can also
		// force it
		if (debug_lmao || dbg)
			_itv_row_string.resize(_itv_rows.size());

		// main interval loop, pattern mods values are produced in this outer
		// loop using the data aggregated/generated in the inner loop
		// all pattern mod functors should use i as an argument, since it needs
		// to update the pattern mod holder at the proper index
		// we end up running the hand independent pattern mods twice, but i'm
		// not sure it matters? they tend to be low cost, this should be split
		// up properly into hand dependent/independent loops if it turns out to
		// be an issue
		for (size_t itv = 0; itv < _itv_rows.size(); ++itv) {
			// reset the last mni interval data, since it gets used to
			// initialize now
			_mni_last->_itv_info.reset();

			// inner loop
			for (auto& row : _itv_rows[itv]) {
				// ok we really should be doing separate loops for both
				// hand/separate hand stuff, and this should be in the former
				if (hand == 0)
					if (debug_lmao || dbg) {
						_itv_row_string[itv].append(note_map[_ni[row].notes]);
						_itv_row_string[itv].append("\n");
					}
				if (debug_lmao)
					std::cout << "\n" << _itv_row_string[itv] << std::endl;

				// generate current metanoteinfo using stuff + last metanoteinfo
				(*_mni_now)(
				  *_mni_last, _ni[row].rowTime, _ni[row].notes, _t1, _t2, row);

				// should be self explanatory
				handle_row_dependent_pattern_advancement();

				if (dbg) {
					_dbg(*_mni_last,
						 _ni[row].rowTime,
						 _ni[row].notes,
						 _t1,
						 _t2,
						 row,
						 true);

					// left hand stuffies
					if (hand == 0)
						_mni_dbg_vec1[itv].push_back(_dbg);
					else
						// right hand stuffies
						_mni_dbg_vec2[itv].push_back(_dbg);
				}

				set_mni_last();
			}
			// pop the last \n for the interval
			if (debug_lmao || dbg)
				if (hand == 0)
					if (!_itv_row_string[itv].empty())
						_itv_row_string[itv].pop_back();

			// set the pattern mod values by calling the mod functors
			call_pattern_mod_functors(itv);
		}
		run_smoothing_pass();
	}

	//// maybe overload for non-hand-specific?
	// inline void operator()(const vector<vector<int>>& itv_rows,
	//					   const float& rate,
	//					   vector<float> doot1[],
	//					   vector<float> doot2[]){

	//};

#pragma region patternmod "loops"
	// some pattern mod detection builds across rows, see rm_sequencing for an
	// example
	void handle_row_dependent_pattern_advancement()
	{
		_ohj.advance_sequencing(*_mni_now);
		_oht.advance_sequencing(*_mni_now);
		_rm.advance_sequencing(*_mni_now);
		_wrjt.advance_sequencing(*_mni_now);
		_wrr.advance_sequencing(*_mni_now);
	}

	inline void run_pattern_mod_setups()
	{
		_s.setup(_doot, _itv_rows.size());
		_js.setup(_doot, _itv_rows.size());
		_hs.setup(_doot, _itv_rows.size());
		_cj.setup(_doot, _itv_rows.size());
		_anch.setup(_doot, _itv_rows.size());
		_ohj.setup(_doot, _itv_rows.size());
		_oht.setup(_doot, _itv_rows.size());
		_rm.setup(_doot, _itv_rows.size());
		_wrr.setup(_doot, _itv_rows.size());
		_wrjt.setup(_doot, _itv_rows.size());
	}

	inline void run_smoothing_pass()
	{
		_s.smooth_finish(_doot);
		_js.smooth_finish(_doot);
		_hs.smooth_finish(_doot);
		_cj.smooth_finish(_doot);
		_anch.smooth_finish(_doot);
		_ohj.smooth_finish(_doot);
		_oht.smooth_finish(_doot);
		_rm.smooth_finish(_doot);
		_wrr.smooth_finish(_doot);
		_wrjt.smooth_finish(_doot);
	}

	inline void call_pattern_mod_functors(const int& itv)
	{
		_s(*_mni_now, _doot, itv);
		_js(*_mni_now, _doot, itv);
		_hs(*_mni_now, _doot, itv);
		_cj(*_mni_now, _doot, itv);
		_anch(*_mni_now, _doot, itv, hand);
		_ohj(*_mni_now, _doot, itv, hand);
		_oht(*_mni_now, _doot, itv, hand);
		_rm(*_mni_now, _doot, itv);
		_wrr(*_mni_now, _doot, itv, hand);
		_wrjt(*_mni_now, _doot, itv);
	}

	inline void load_params_from_disk()
	{
		std::string fn = calc_params_xml;
		int iError;
		std::unique_ptr<RageFileBasic> pFile(
		  FILEMAN->Open(fn, RageFile::READ, iError));
		if (pFile.get() == NULL)
			return;

		XNode params;
		if (!XmlFileUtil::LoadFromFileShowErrors(params, *pFile.get()))
			return;

		_s.load_params_from_node(&params);
		_js.load_params_from_node(&params);
		_hs.load_params_from_node(&params);
		_cj.load_params_from_node(&params);
		_anch.load_params_from_node(&params);
		_ohj.load_params_from_node(&params);
		_oht.load_params_from_node(&params);
		_rm.load_params_from_node(&params);
		_wrr.load_params_from_node(&params);
		_wrjt.load_params_from_node(&params);
	}

	inline XNode* make_param_node() const
	{
		XNode* calcparams = new XNode("CalcParams");

		calcparams->AppendChild(_s.make_param_node());
		calcparams->AppendChild(_js.make_param_node());
		calcparams->AppendChild(_hs.make_param_node());
		calcparams->AppendChild(_cj.make_param_node());
		calcparams->AppendChild(_anch.make_param_node());
		calcparams->AppendChild(_ohj.make_param_node());
		calcparams->AppendChild(_oht.make_param_node());
		calcparams->AppendChild(_rm.make_param_node());
		calcparams->AppendChild(_wrr.make_param_node());
		calcparams->AppendChild(_wrjt.make_param_node());

		return calcparams;
	}
#pragma endregion

	void write_params_to_disk()
	{
		std::string fn = calc_params_xml;
		std::unique_ptr<XNode> xml(make_param_node());

		std::string err;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE))
			return;
		XmlFileUtil::SaveToFile(xml.get(), f, "", false);
	}
};

bool
Calc::InitializeHands(const vector<NoteInfo>& NoteInfo,
					  float music_rate,
					  float offset)
{
	numitv = static_cast<int>(
	  std::ceil(NoteInfo.back().rowTime / (music_rate * IntervalSpan)));

	bool junk_file_mon = false;
	ProcessedFingers fingers;
	for (auto t : zto3) {
		fingers.emplace_back(
		  ProcessFinger(NoteInfo, t, music_rate, offset, junk_file_mon));

		// don't bother with this file
		if (junk_file_mon)
			return false;
	}

	// sequence jack immediately so we can ref pass & sort in calc msestimate
	// without things going be wackying
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
			for (size_t i = 0; i < fingers[t].size(); ++i)
				stam_adj_jacks[t][i].resize(fingers[t][i].size());
		}
	}

	pair<Hand&, vector<int>> spoopy[2] = { { left_hand, { 1, 2 } },
										   { right_hand, { 4, 8 } } };

	TheGreatBazoinkazoinkInTheSky ulbo;
	ulbo.bazoink(NoteInfo);

	// loop to help with hand specific stuff, we could do this stuff
	// in the class but that's more structural work and this is
	// simple
	for (auto& hp : spoopy) {
		auto& hand = hp.first;
		const auto& fv = hp.second;

		// these definitely do change with every chisel test
		hand.stam_adj_diff.resize(numitv);
		SetSequentialDownscalers(NoteInfo, fv[0], fv[1], music_rate, hand.doot);
		ulbo(nervIntervals, music_rate, fv[0], fv[1], hand.doot);
	}

	// these are evaluated on all columns so right and left are the
	// same these also may be redundant with updated stuff
	
	SetFlamJamMod(NoteInfo, left_hand.doot, music_rate);
	TheThingLookerFinderThing(NoteInfo, music_rate, left_hand.doot);
	WideRangeBalanceScaler(NoteInfo, music_rate, left_hand.doot);
	WideRangeAnchorScaler(NoteInfo, music_rate, left_hand.doot);

	vector<int> bruh_they_the_same = { Stream,		 Chaos,
									   FlamJam,			 TheThing,
									   WideRangeBalance, WideRangeAnchor };
	// hand agnostic mods are the same
	for (auto pmod : bruh_they_the_same)
		right_hand.doot[pmod] = left_hand.doot[pmod];

	// do these last since calcmsestimate modifies the interval ms values of
	// fingers with sort, anything that is derivative of those values that
	// requires them to be in sequential order should be done before this point
	left_hand.InitBaseDiff(fingers[0], fingers[1]);
	left_hand.InitPoints(fingers[0], fingers[1]);
	left_hand.InitAdjDiff();
	right_hand.InitBaseDiff(fingers[2], fingers[3]);
	right_hand.InitPoints(fingers[2], fingers[3]);
	right_hand.InitAdjDiff();

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

			for (size_t i = 0; i < ModCount; ++i)
				hand.debugValues[0][i] = hand.doot[i];

			// set everything but final adjusted output here
			for (size_t i = 0; i < NUM_CalcDiffValue - 1; ++i)
				hand.debugValues[1][i] = hand.soap[i];
		}
	}
	return true;
}

float
Hand::CalcMSEstimate(vector<float>& input, const int& burp)
{
	static const bool dbg = false;

	// how many ms values we use from here, if there are fewer than this
	// number we'll mock up some values to water down intervals with a
	// single extremely fast minijack, if there are more, we will truncate
	unsigned int num_used = burp;

	if (input.empty())
		return 0.f;

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
	static const float ms_dummy = 360.f;

	// mostly try to push down stuff like jumpjacks, not necessarily to push
	// up "complex" stuff (this will push up intervals with few fast ms values
	// kinda hard but it shouldn't matter as their base ms diff should be
	// extremely low
	float cv_yo = cv_trunc_fill(input, burp, ms_dummy) + 0.5f;
	cv_yo = CalcClamp(cv_yo, 0.5f, 1.25f);

	if (dbg && debug_lmao) {
		std::string moop = "";
		for (auto& v : input) {
			moop.append(std::to_string(v));
			moop.append(", ");
		}

		std::cout << "notes: " << moop << std::endl;
	}

	if (dbg && debug_lmao)
		std::cout << "cv : " << cv_yo << std::endl;

	// basically doing a jank average, bigger m = lower difficulty
	float m = sum_trunc_fill(input, burp, ms_dummy);

	if (dbg && debug_lmao)
		std::cout << "m : " << m << std::endl;

	// add 1 to num_used because some meme about sampling
	// same thing as jack stuff, convert to bpm and then nps
	float bpm_est = ms_to_bpm(m / (num_used + 1));
	float nps_est = bpm_est / 15.f;
	float fdiff = nps_est * cv_yo;
	if (dbg && debug_lmao)
		std::cout << "diff : " << fdiff << std::endl;
	return fdiff;
}

void
Hand::InitBaseDiff(Finger& f1, Finger& f2)
{
	static const bool dbg = false;

	for (size_t i = 0; i < NUM_CalcDiffValue - 1; ++i)
		soap[i].resize(f1.size());

	for (size_t i = 0; i < f1.size(); i++) {

		if (dbg && debug_lmao)
			std::cout << "\ninterval : " << i << std::endl;

		// scaler for things with higher things
		static const float higher_thing_scaler = 1.175f;
		float nps = 1.6f * static_cast<float>(f1[i].size() + f2[i].size());

		auto do_diff_thingy = [this](vector<float>& input,
									 const float& scaler) {
			float mwerp = CalcMSEstimate(input, 3);
			if (input.size() > 3)
				mwerp = max(mwerp, CalcMSEstimate(input, 4) * scaler);
			if (input.size() > 4)
				mwerp = max(mwerp, CalcMSEstimate(input, 5) * scaler * scaler);
			return mwerp;
		};

		float left_diff = do_diff_thingy(f1[i], higher_thing_scaler);
		float right_diff = do_diff_thingy(f1[i], higher_thing_scaler);

		float difficulty = 0.f;
		float squiggly_line = 5.5f;
		if (left_diff > right_diff)
			difficulty =
			  weighted_average(left_diff, right_diff, squiggly_line, 9.f);
		else
			difficulty =
			  weighted_average(right_diff, left_diff, squiggly_line, 9.f);
		soap[BaseNPS][i] = finalscaler * nps;
		soap[BaseMS][i] = finalscaler * difficulty;
		soap[BaseMSD][i] =
		  weighted_average(difficulty, nps, 7.76445f, 10.f) * finalscaler;
	}
	Smooth(soap[BaseNPS], 0.f);
	DifficultyMSSmooth(soap[BaseMS]);
	DifficultyMSSmooth(soap[BaseMSD]);
}

// each skillset should just be a separate calc function [todo]
float
Calc::Chisel(float player_skill,
			 float resolution,
			 float score_goal,
			 int ss,
			 bool stamina,
			 bool debugoutput)
{

	float gotpoints = 0.f;
	float reqpoints = static_cast<float>(MaxPoints) * score_goal;
	float max_points_lost = static_cast<float>(MaxPoints) - reqpoints;
	float jloss = 0.f;
	for (int iter = 1; iter <= 8; iter++) {
		do {
			if (player_skill > 100.f)
				return player_skill;
			player_skill += resolution;
			if (ss == Skill_Overall || ss == Skill_Stamina)
				return 0.f; // not how we set these values

			// reset tallied score, always deduct rather than accumulate now
			gotpoints = static_cast<float>(MaxPoints);
			if (true) {
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
					/*	static const float literal_black_magic = 0.85f;
						if (ss == Skill_Technical) {
							float bzz = 0.f;
							float bz0 =
							  JackLoss((player_skill * literal_black_magic
					   ), 0, max_points_lost, stamina) * 3.f; float bz1 =
							  JackLoss((player_skill * literal_black_magic),
									   1,
									   max_points_lost,
									   stamina) +
							  bz0;
							float bz2 =
							  JackLoss((player_skill * literal_black_magic),
									   2,
									   max_points_lost,
									   stamina) +
							  bz0;
							float bz3 =
							  JackLoss((player_skill * literal_black_magic),
									   3,
									   max_points_lost,
									   stamina) +
							  bz0;

							bzz = mean(vector<float>{ bz1, bz2, bz3 });
							gotpoints += bzz / 3.f;
						}*/

					left_hand.CalcInternal(
					  gotpoints, player_skill, ss, stamina);

					// already can't reach goal, move on
					if (gotpoints > reqpoints)
						right_hand.CalcInternal(
						  gotpoints, player_skill, ss, stamina);
				}
#endif
			}
		} while (gotpoints < reqpoints);
		player_skill -= resolution;
		resolution /= 2.f;
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
		if (jl1 > jl2 && jl1 > jl3)
			JackLoss(player_skill, 1, max_points_lost, stamina, debugoutput);
		else if (jl2 > jl3)
			JackLoss(player_skill, 2, max_points_lost, stamina, debugoutput);
		else
			JackLoss(player_skill, 3, max_points_lost, stamina, debugoutput);

		left_hand.CalcInternal(
		  gotpoints, player_skill, ss, stamina, debugoutput);
		right_hand.CalcInternal(
		  gotpoints, player_skill, ss, stamina, debugoutput);
	}

	return player_skill + 2.f * resolution;
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
		  Anchor,
		  WideRangeBalance,
		},

		// js
		{
		  JS,
		  Chaos,
		  OHJumpMod,
		  TheThing,
		  Anchor,
		  WideRangeBalance,
		},

		// hs
		{
		  HS,
		  Chaos,
		  OHJumpMod,
		  TheThing,
		  Anchor,
		  WideRangeBalance,
		},

		// stam, nothing, don't handle here
		{},

		// jackspeed, ignore for now
		{
		  Chaos,
		  Roll,
		  WideRangeJumptrill,
		  WideRangeRoll,
		  FlamJam,
		  OHJumpMod,
		  CJOHJump,
		  CJQuad,
		  WideRangeBalance,
		},

		// chordjack
		{
		  CJ,
		  CJQuad,
		  CJOHJump,
		  Anchor,
		  WideRangeBalance,
		},

		// tech, duNNO wat im DOIN
		{
		  OHTrill,
		  Anchor,
		  Roll,
		  OHJumpMod,
		  Chaos,
		  WideRangeJumptrill,
		  WideRangeBalance,
		  WideRangeRoll,
		  FlamJam,
		  RanMan,
		},

	};

	vector<float> scoring_justice_warrior_agenda(NUM_Skillset - 1);

	// why can't i do this in the function that calls this?
	for (int i = 0; i < NUM_Skillset; ++i) {
		base_adj_diff[i].resize(soap[BaseNPS].size());
		base_diff_for_stam_mod[i].resize(soap[BaseNPS].size());
	}

	// ok this loop is pretty wack i know, for each interval
	for (size_t i = 0; i < soap[BaseNPS].size(); ++i) {
		float tp_mods[NUM_Skillset] = {
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f
		};

		// total pattern mods for each skillset, we want this to be
		// calculated before the main skillset loop because we might
		// want access to the total js mod while on stream, or
		// something
		for (int ss = 0; ss < NUM_Skillset; ++ss) {
			// is this even faster than multiplying 1.f by 1.f a
			// billion times?
			if (ss == Skill_Overall || ss == Skill_Stamina)
				continue;
			for (auto& pmod : pmods_used[ss])
				tp_mods[ss] *= doot[pmod][i];
		}

		// main skillset loop, for each skillset that isn't overall
		// or stam
		for (int ss = 0; ss < NUM_Skillset; ++ss) {
			if (ss == Skill_Overall || ss == Skill_Stamina)
				continue;

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
					adj_diff *= CalcClamp(fastsqrt(doot[RanMan][i] - 0.1f), 1.f, 1.075f);
					break;

				// test calculating stam for js/hs on max js/hs diff
				// we want hs to count against js so they are
				// mutually exclusive
				case Skill_Jumpstream:
					adj_diff /=
					  max(doot[HS][i], 1.f) * fastsqrt(doot[OHJumpMod][i]);
					adj_diff *=
					  CalcClamp(fastsqrt(doot[RanMan][i]), 1.f, 1.05f);
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
				case Skill_Technical:
					// AHAHAHHAAH DRUNK WITH POWER AHAHAHAHAHAAHAHAH
					{
						// for (int j = 0; j < NUM_Skillset - 1; ++j)
						//	if (j == Skill_Stamina || j == Skill_Overall)
						//		scoring_justice_warrior_agenda[j] = 0.f;
						//	else
						//		scoring_justice_warrior_agenda[j] =
						// tp_mods[j];
						// float muzzle = *std::max_element(
						//  scoring_justice_warrior_agenda.begin(),
						//  scoring_justice_warrior_agenda.end());
						adj_diff = soap[BaseMSD][i] * tp_mods[ss] *
								   basescalers[ss] /
								   fastsqrt(doot[WideRangeBalance][i]) /
								   max(doot[CJ][i], 1.f);
					}
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

	if (stam)
		StamAdjust(x, ss);

	// final difficulty values to use
	const vector<float>& v = stam ? stam_adj_diff : base_adj_diff[ss];
	float powindromemordniwop = 1.9f;
	if (ss == Skill_Chordjack)
		powindromemordniwop = 2.f;
	// i don't like the copypasta either but the boolchecks where
	// they were were too slow
	if (debug) {
		debugValues[2][StamMod].resize(v.size());
		debugValues[2][PtLoss].resize(v.size());
		// final debug output should always be with stam activated
		StamAdjust(x, ss, true);
		debugValues[1][MSD] = stam_adj_diff;

		for (size_t i = 0; i < v.size(); ++i) {
			if (x < v[i]) {
				float pts = static_cast<float>(v_itvpoints[i]);
				float lostpoints = (pts - (pts * fastpow(x / v[i], 1.7f)));
				gotpoints -= lostpoints;
				debugValues[2][PtLoss][i] = abs(lostpoints);
			}
		}
	} else
		for (size_t i = 0; i < v.size(); ++i)
			if (x < v[i]) {
				float pts = static_cast<float>(v_itvpoints[i]);
				gotpoints -= (pts - (pts * fastpow(x / v[i], 1.7f)));
			}
}

inline void
Hand::StamAdjust(float x, int ss, bool debug)
{
	float stam_floor =
	  0.95f;		   // stamina multiplier min (increases as chart advances)
	float mod = 0.95f; // mutliplier

	float avs1 = 0.f;
	float avs2 = 0.f;
	float local_ceil = stam_ceil;
	const float super_stam_ceil = 1.11f;

	// use this to calculate the mod growth
	const auto& base_diff = base_diff_for_stam_mod[ss];
	// but apply the mod growth to these values
	// they might be the same, or not
	const auto& diff = base_adj_diff[ss];

	// i don't like the copypasta either but the boolchecks where
	// they were were too slow
	if (debug)
		for (size_t i = 0; i < base_diff.size(); i++) {
			avs1 = avs2;
			avs2 = base_diff[i];
			mod += ((((avs1 + avs2) / 2.f) / (stam_prop * x)) - 1.f) / stam_mag;
			if (mod > 0.95f)
				stam_floor += (mod - 0.95f) / stam_fscale;
			local_ceil = stam_ceil * stam_floor;

			mod = min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
			stam_adj_diff[i] = diff[i] * mod;
			debugValues[2][StamMod][i] = mod;
		}
	else
		for (size_t i = 0; i < base_diff.size(); i++) {
			avs1 = avs2;
			avs2 = base_diff[i];
			mod += ((((avs1 + avs2) / 2.f) / (stam_prop * x)) - 1.f) / stam_mag;
			if (mod > 0.95f)
				stam_floor += (mod - 0.95f) / stam_fscale;
			local_ceil = stam_ceil * stam_floor;

			mod = min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
			stam_adj_diff[i] = diff[i] * mod;
		}
}
#pragma endregion

#pragma region PatternMods




// downscales full rolls or rolly js, it looks explicitly for
// consistent cross column timings on both hands; consecutive notes
// on the same column will reduce the penalty 0.5-1 multiplier also
// now downscales ohj because we don't want to run this loop too
// often even if it makes code slightly clearer, i think, new ohj
// scaler is the same as the last one but gives higher weight to
// sequences of ohjumps 0.5-1 multipier
void
Calc::SetSequentialDownscalers(const vector<NoteInfo>& NoteInfo,
							   unsigned int t1,
							   unsigned int t2,
							   float music_rate,
							   vector<float> doot[])
{
	doot[Roll].resize(nervIntervals.size());
	doot[OHJumpMod].resize(nervIntervals.size());
	doot[CJOHJump].resize(nervIntervals.size());
	doot[OHTrill].resize(nervIntervals.size());
	doot[Chaos].resize(nervIntervals.size());

	// not sure if these should persist between intervals or not
	// not doing so makes the pattern detection slightly more strict
	// doing so will give the calc some context from the previous
	// interval but might have strange practical consequences
	// another major benefit of retaining last col from the previous
	// interval is we don't have to keep resetting it and i don't
	// like how that case is handled atm

	vector<float> lr;
	vector<float> rl;
	float lasttime = 0.f;
	float dswip = 0.f;
	int lastcol = -1;
	int lastsinglecol = -1;
	static const float water_it_for_me = 0.05f;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// roll downscaler stuff
		// this appears not to be picking up certain patterns in
		// certain test files, reminder to investigate
		int totaltaps = 0;

		lr.clear();
		rl.clear();

		int ltaps = 0;
		int rtaps = 0;
		int dswap = 0;

		// ohj downscaler stuff
		int jumptaps = 0;	  // more intuitive to count taps in jumps
		int max_jumps_seq = 0; // basically the biggest sequence of ohj
		int cur_jumps_seq = 0;
		bool newrow = true;
		for (int row : nervIntervals[i]) {
			//	if (debugmode && newrow)
			//		std::cout << "new interval: " << i << " time: "
			//				  << NoteInfo[row].rowTime / music_rate
			//				  << " hand: " << t1 << std::endl;
			newrow = false;
			// if (debugmode)
			//	std::cout << "new row" << std::endl;
			bool lcol = NoteInfo[row].notes & t1;
			bool rcol = NoteInfo[row].notes & t2;
			totaltaps += (static_cast<int>(lcol) + static_cast<int>(rcol));
			float curtime = NoteInfo[row].rowTime / music_rate;

			// as variation approaches 0 the effect of variation
			// diminishes, e.g. given 140, 140, 120 ms and 40, 40,
			// 20 ms the variation in 40, 40, 20 is meaningless
			// since they're all psuedo jumps anyway, but it will
			// prevent the roll downscaler from being applied to the
			// degree it should, so add a flat value to water down
			// the effect
			float bloaaap = water_it_for_me + curtime - lasttime;

			// ignore jumps/no tapsals
			if (!(lcol ^ rcol)) {
				// fully skip empty rows, set nothing
				if (!(lcol || rcol)) {
					// if (debugmode)
					//	std::cout << "empty row" << std::endl;
					continue;
				}

				// if (debugmode)
				//	std::cout << "jump" << std::endl;

				// add jumptaps when hitting jumps for ohj
				// turns out in order to catch rolls with periodic
				// [12] jumps we need to actually count them as
				// taps-inside-rolls rather than just ignoring them,
				// and we can try kicking back an extra value into
				// the lr or rl vectors since 1->[12] is technically
				// a 1->2 and the difference in motion isn't
				// appreciably different under the circumstances we
				// are interested in
				if (lcol && rcol) {
					jumptaps += 2;
					lastsinglecol = lastcol;
					// on ohjumps treat the next note as always
					// cross column
					lastcol = -1;
				}

				// yes we want to set this for jumps
				lasttime = curtime;

				// iterate recent jumpsequence
				++cur_jumps_seq;
				// set the largest ohj sequence
				max_jumps_seq =
				  cur_jumps_seq > max_jumps_seq ? cur_jumps_seq : max_jumps_seq;
				continue;
			}
			// reset cur_jumps_seq on hitting a single note
			cur_jumps_seq = 0;

			// if lcol is true and we are here we have 1 single tap
			// and if lcol is true we are on column 0; don't try to
			// be clever, even if lcol < rcol to convert bools into
			// ints into a bool into an int worked it was needlessly
			// confusing
			int thiscol = lcol ? 0 : 1;

			// ignore consecutive notes, if we encountered a one
			// hand jump treat it as always being a column swap if
			// (debugmode) std::cout << "lastcol is " << lastcol <<
			// std::endl; if (debugmode) std::cout << "thiscol is "
			// << thiscol << std::endl;
			if (thiscol != lastcol || lastcol == -1) {
				// treat 1[12]2 as different from 1[12]1, count the
				// latter as an anchor and the former as a roll with
				// 4 notes ok actually lets treat them the mostly
				// same for the time being
				if (lastcol == -1)
					if (rcol) {
						lr.push_back(bloaaap);
						++ltaps;
						++rtaps;
					} else if (lcol) {
						rl.push_back(bloaaap);
						++ltaps;
						++rtaps;
					}

				// this is the col we END on, so if we end on right,
				// we are left to right, not right to left
				if (rcol) {
					lr.push_back(bloaaap);
					++ltaps;
					// if (debugmode)
					// std::cout << "left right " << curtime -
					// lasttime
					//		  << std::endl;
				} else if (lcol) {
					rl.push_back(bloaaap);
					++rtaps;
					// if (debugmode)
					// std::cout << "right to left " << curtime -
					// lasttime
					//		  << std::endl;
				} else {
					// if (debugmode)
					// std::cout << "THIS CANT HAPPEN AAAAAAAAAAAAA"
					// << std::endl;
				}
				// only log cross column lasttimes on single notes
				lasttime = curtime;
			} else {
				// if (debugmode)
				// std::cout << "anchor" << std::endl;
				// consecutive notes should "poison" the current
				// cross column vector but without shifting the
				// proportional scaling too much this is to avoid
				// treating 121212212121 too much like 121212121212

				// if we wanted to be _super explicit_ we could just
				// reset the lr/rl vectors when hitting a
				// consecutive note (and/or jump), there are
				// advantages to being hyper explicit but at the
				// moment this does sort of pick up rolly js quite
				// well, though it would probably be more
				// responsible longterm to have an explicit roll
				// detector an explicit trill detector, and an
				// explicit rolly js detector thing is debugging all
				// 3 and making sure they work as intended and in
				// exclusion is just as hard as making a couple of
				// more generic mods and accepting they will overlap
				// in certain scenarios though again on the other
				// hand explicit modifiers are easier to tune you
				// just have to do a lot more of it
				if (rcol)
					lr.push_back(bloaaap);
				else if (lcol)
					rl.push_back(bloaaap);

				// we have an anchor and we either have moderately
				// complex patterning now or we have simply changed
				// direction of the roll
				++dswap;
			}

			// ohj downscaler stuff
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

			// edit, breaks in ohj sequences are implicitly
			// penalized by a base nps loss, we don't need to do it
			// again here as it basically shreds the modifiers for
			// quadjacks with some hands sprinkled in
			//	jumptaps -=
			//	 1; // we're already on single notes, so just
			// decrement
			// a lil

			// if we have a consecutive single tap, and the column
			// swaps, NOW, shred the ohjump modifier
			if (thiscol != lastcol && lastcol != -1) {
				jumptaps -= 1;
				//		if (debugmode)
				//			std::cout << "removed jumptap, now: " <<
				// jumptaps
				//					  << std::endl;
				//		if (debugmode)
				//			std::cout << "last col is: " << lastcol
				//					  << std::endl;
				//		if (debugmode)
				//			std::cout << "this col is: " << thiscol
				//<<
				// std::endl;
			}
			lastcol = thiscol;
		}

		/*
		if (debugmode) {
			std::string rarp = "left to right: ";

			for (auto& a : lr) {
				rarp.append(std::to_string(a - water_it_for_me));
				rarp.append(", ");
			}
			rarp.append("\nright to left: ");

			for (auto& b : rl) {
				rarp.append(std::to_string(b - water_it_for_me));
				rarp.append(", ");
			}

			std::cout << "" << rarp << std::endl;
		}
		*/

		// I DONT KNOW OK
		dswip = (dswip + dswap) / 2.f;
		int cvtaps = ltaps + rtaps;

		// if this is true we have some combination of single notes
		// and jumps where the single notes are all on the same
		// column
		if (cvtaps == 0) {

			//	if (debugmode)
			//		std::cout << "cvtaps0: " << max_jumps_seq <<
			// std::endl;

			// we don't want to treat 2[12][12][12]2222
			// 2222[12][12][12]2 differently, so use the
			// max sequence here exclusively
			if (max_jumps_seq > 0) {

				// yea i thought we might need to tune ohj
				// downscalers for js and cj slightly differently
				doot[OHJumpMod][i] =
				  CalcClamp(pow(static_cast<float>(totaltaps) /
								  (static_cast<float>(max_jumps_seq) * 2.5f),
								2.f),
							0.5f,
							1.f);

				// ohjumps in cj can be either easier or harder
				// depending on context.. so we have to pull back a
				// bit so it doesn't swing too far when it shouldn't
				doot[CJOHJump][i] =
				  CalcClamp(pow(static_cast<float>(totaltaps) /
								  (static_cast<float>(max_jumps_seq) * 2.4f),
								2.f),
							0.6f,
							1.f);
				//	if (debugmode)
				//		std::cout << "chohj: " << doot[CJOHJump][i]
				//<<
				// std::endl;
			}

			else { // single note longjacks, do nothing
				   //	if (debugmode)
				   //		std::cout << "zemod would be but wasn't:
				   //" <<
				   // zemod
				   //				  << std::endl;
				doot[OHJumpMod][i] = 1.f;
				doot[CJOHJump][i] = 1.f;
			}

			// no rolls here by definition
			doot[Roll][i] = 0.9f;
			doot[OHTrill][i] = 1.f;
			continue;
		}

		float cvlr = 0.2f;
		float cvrl = 0.2f;
		if (lr.size() > 1)
			cvlr = cv(lr);
		if (rl.size() > 1)
			cvrl = cv(rl);

		// if (debugmode)
		//	std::cout << "cv lr " << cvlr << std::endl;
		// if (debugmode)
		//	std::cout << "cv rl " << cvrl << std::endl;

		// weighted average, but if one is empty we want it to skew
		// high not low due to * 0
		float Cv = ((cvlr * (ltaps + 1)) + (cvrl * (rtaps + 1))) /
				   static_cast<float>(cvtaps + 2);

		// if (debugmode)
		//	std::cout << "cv " << Cv << std::endl;
		float yes_trills = 1.f;

		// check for oh trills
		if (true) {
			if (!lr.empty() && !rl.empty()) {

				// ok this is SUPER jank but we added a flat amount
				// to the ms values to water down the effects of
				// variation, but that will negate the differential
				// between the means of the two, so now we have to
				// again subtract that amount from the ms values in
				// the vectors
				for (auto& v : lr)
					v -= water_it_for_me;
				for (auto& v : rl)
					v -= water_it_for_me;

				float no_trills = 1.f;
				float mlr = mean(lr);
				float mrl = mean(rl);
				bool rl_is_higher = mlr < mrl;

				// if the mean of one isn't much higher than the
				// other, it's oh trills, so leave it alone, if it
				// is, scale down the roll modifier by the oh
				// trillyness, we don't want to affect that here
				float div = rl_is_higher ? mrl / mlr : mlr / mrl;
				div = CalcClamp(div, 1.f, 3.f);
				// if (debugmode)
				//	std::cout << "div " << div << std::endl;
				no_trills = CalcClamp(1.75f - div, 0.f, 1.f);

				// store high oh trill detection in case
				// we want to do stuff with it later
				yes_trills = CalcClamp(1.1f - div, 0.f, 1.f);
				Cv += no_trills * 1.f; // just straight up add to cv
			}
		}

		// cv = rl_is_higher ? (2.f * cv + cvrl) / 3.f : (2.f * cv +
		// cvlr) / 3.f; if (debugmode) 	std::cout << "cv2 " << cv <<
		// std::endl;
		/*

		// then scaled against how many taps we ignored

		float barf = (-0.1f + (dswap * 0.1f));
		barf += (barf2 - 1.f);
		if (debugmode)
			std::cout << "barf " << barf << std::endl;
		cv += barf;
		cv *= barf2;
		cv = CalcClamp(cv, 0.f, 1.f);
		if (debugmode)
			std::cout << "cv3 " << cv << std::endl;
		yes_trills *= barf;
		*/

		// we just want a minimum amount of variation to escape
		// getting downscaled cap to 1 (it's not an inherently bad
		// idea to upscale sets of patterns with high variation but
		// we shouldn't do that here, probably)

		float barf2 =
		  static_cast<float>(totaltaps) / static_cast<float>(cvtaps);
		float barf =
		  0.25f +
		  0.4f * (static_cast<float>(totaltaps) / static_cast<float>(cvtaps)) +
		  dswip * 0.25f;

		// some weird anchor problems can cause sqrt(-f) here so...
		if (Cv > 0.000000000000000000000000000000001f) {
			Cv = fastsqrt(Cv) - 0.1f;
			Cv += barf;
			Cv *= barf2;
			doot[Roll][i] = CalcClamp(Cv, 0.5f, 1.f);
		} else
			doot[Roll][i] = 1.f;

		doot[OHTrill][i] = CalcClamp(0.5f + fastsqrt(yes_trills), 0.8f, 1.f);
		// if (debugmode)
		//	std::cout << "final mod " << doot[Roll][i] << "\n" <<
		// std::endl;
		// ohj stuff, wip
		if (jumptaps < 1 && max_jumps_seq < 1) {
			//		if (debugmode)
			//			std::cout << "down to end but eze: " <<
			// max_jumps_seq
			//					  << std::endl;
			doot[OHJumpMod][i] = 1.f;
			doot[CJOHJump][i] = 1.f;
		} else {
			// STANDARD OHJ
			// for js we lean into max sequences more, since they're
			// better indicators of inflated difficulty
			float max_seq_component =
			  0.65f * (1.125f - static_cast<float>(max_jumps_seq * 2.5) /
								  static_cast<float>(totaltaps));
			max_seq_component = CalcClamp(max_seq_component, 0.f, 0.65f);

			float prop_component =
			  0.35f * (1.2f - static_cast<float>(jumptaps) /
								static_cast<float>(totaltaps));
			prop_component = CalcClamp(prop_component, 0.f, 0.65f);

			float base_ohj = max_seq_component + prop_component;
			float ohj = fastsqrt(base_ohj);

			doot[OHJumpMod][i] = CalcClamp(0.1f + ohj, 0.5f, 1.f);

			// CH OHJ
			// we want both the total number of jumps and the max
			// sequence to count here, with more emphasis on the max
			// sequence, sequence should be multiplied by 2 (or
			// maybe slightly more?)
			max_seq_component =
			  0.5f * fastsqrt(1.2f - static_cast<float>(max_jumps_seq * 2) /
									   static_cast<float>(totaltaps));
			max_seq_component =
			  max_seq_component > 0.5f ? 0.5f : max_seq_component;

			prop_component =
			  0.5f * fastsqrt(1.2f - static_cast<float>(jumptaps) /
									   static_cast<float>(totaltaps));
			prop_component = prop_component > 0.5f ? 0.5f : prop_component;

			float base_cjohj = 0.3f + max_seq_component + prop_component;
			float cjohj = fastsqrt(base_cjohj);

			//	if (debugmode)
			//		std::cout << "jumptaps: "
			//					  << jumptaps << std::endl;
			//			if (debugmode)
			//				std::cout << "maxseq: " << max_jumps_seq
			//<<
			// std::endl; 		if (debugmode) 			std::cout <<
			// "total taps:
			// "
			// << totaltaps
			//<< std::endl; 	if (debugmode) 		std::cout << "seq
			// comp: " <<
			// max_seq_component<< std::endl; if (debugmode)
			// std::cout
			// << "prop comp: " <<prop_component << std::endl; if
			// (debugmode) std::cout << "actual prop: " << ohj
			//					  << std::endl;
			doot[CJOHJump][i] = CalcClamp(cjohj, 0.5f, 1.f);
			// if (debugmode)
			//	std::cout << "final mod: " << doot[OHJump][i] <<
			//"\n" <<
			// std::endl;
		}
	}

		Smooth(doot[Roll], 1.f);
		Smooth(doot[Roll], 1.f);
		Smooth(doot[OHTrill], 1.f);
		Smooth(doot[OHJumpMod], 1.f);
		Smooth(doot[CJOHJump], 1.f);
	// hack because i was sqrt'ing in calcinternal for js and hs
	// for (auto& v : doot[OHJump])
	//	v = fastsqrt(v);

	// this is fugly but basically we want to negate any _bonus_
	// from chaos if the polys are arranged in a giant ass roll
	// formation for (size_t i = 0; i < doot[Chaos].size(); ++i)
	//	doot[Chaos][i] = CalcClamp(doot[Chaos][i] * doot[Roll][i],
	//							   doot[Chaos][i],
	//							   max(doot[Chaos][i] *
	// doot[Roll][i], 1.f));

	// for (size_t i = 0; i < doot[Roll].size(); ++i)
	//	doot[Roll][i] = CalcClamp(doot[Roll][i] * doot[OHTrill][i],
	//							  0.4f,
	//							  1.f);

	return;
}

// this should probably almost assuredly be hand specific???
inline float
wras_internal(const vector<NoteInfo>& NoteInfo,
			  float music_rate,
			  const vector<int>& rows,
			  deque<int>& itv_taps,
			  deque<vector<int>>& itv_col_taps,
			  vector<int>& col_taps,
			  bool dbg)
{
	static const float min_mod = 1.0f;
	static const float max_mod = 1.1f;
	int interval_taps = 0;

	bool newint1 = true;
	for (int row : rows) {
		if (dbg && newint1)
			std::cout << "interval start time: "
					  << NoteInfo[row].rowTime / music_rate << std::endl;
		newint1 = false;

		interval_taps += column_count(NoteInfo[row].notes);

		// iterate taps per col.. yes we've done this already in
		// process finger but w.e just redo it for now
		for (size_t c = 0; c < col_ids.size(); c++)
			if (NoteInfo[row].notes & col_ids[c])
				++col_taps[c];
	}

	itv_taps.push_back(interval_taps);
	itv_col_taps.push_back(col_taps);

	int window_taps = sum(itv_taps);
	vector<int> window_col_taps(4);
	for (auto& n : itv_col_taps)
		for (size_t c = 0; c < col_ids.size(); c++)
			window_col_taps[c] += n[c];

	// for this we really want to highlight the differential between
	// the highest value and the lowest, and for each hand,
	// basically the same concept as the original anchor mod, but we
	// won't discriminate by hand (yet?)
	int window_max_anch = max_val(window_col_taps);
	int window_2nd_anch = 0;

	// we actually do care here if we have 2 equivalent max values,
	// we want the next value below that, technically we should only
	// care if the max value is the same on both hands, since that's
	// significantly harder than them being on the same hand,
	// probably, actually that's only true if they're ohjumps, but
	// we don't know that here and this is supposed to be a simple
	// approach for the moment
	for (auto& n : window_col_taps)
		if (n > window_2nd_anch && n < window_max_anch)
			window_2nd_anch = n;

	if (dbg) {
		std::cout << "window taps: " << window_taps << std::endl;
		std::cout << "window col 1: " << window_col_taps[0] << std::endl;
		std::cout << "window col 2: " << window_col_taps[1] << std::endl;
		std::cout << "window col 3: " << window_col_taps[2] << std::endl;
		std::cout << "window col 4: " << window_col_taps[3] << std::endl;
		std::cout << "max anchor: " << window_max_anch << std::endl;
		std::cout << "2nd anchor: " << window_2nd_anch << std::endl;
	}

	// nothing here or the differential is irrelevant because the
	// number of notes is too small
	if (window_max_anch < 3)
		return 1.f;
	// if we don't return max mod
	if (window_2nd_anch == 0)
		return 1.f;

	// i don't like subtraction very much but it shouldn't be so
	// volatile over this large a window

	float bort =
	  static_cast<float>(window_max_anch) - static_cast<float>(window_2nd_anch);
	bort /= 10.f;
	float pmod = bort + 0.65f;

	if (dbg) {
		std::cout << "bort: " << bort << std::endl;
	}
	return CalcClamp(pmod, min_mod, max_mod);
}

// track anchors over a wide range
void
Calc::WideRangeAnchorScaler(const vector<NoteInfo>& NoteInfo,
							float music_rate,
							vector<float> doot[])
{
	const bool dbg = false && debugmode;
	doot[WideRangeAnchor].resize(nervIntervals.size());

	unsigned int itv_window = 3;
	deque<int> itv_taps;
	deque<vector<int>> itv_col_taps;

	// updated every interval but recycle the memory
	vector<int> col_taps(col_ids.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {

		if (dbg) {
			for (auto row : nervIntervals[i])
				std::cout << NoteInfo[row].notes << std::endl;
			std::cout << "\n" << std::endl;
		}

		// drop the oldest interval values if we have reached full
		// size
		if (itv_taps.size() == itv_window) {
			itv_taps.pop_front();
			itv_col_taps.pop_front();
		}

		doot[WideRangeAnchor][i] = wras_internal(NoteInfo,
												 music_rate,
												 nervIntervals[i],
												 itv_taps,
												 itv_col_taps,
												 col_taps,
												 dbg);
		// reset col taps for this interval
		for (auto& zz : col_taps)
			zz = 0;
		if (dbg)
			std::cout << "final wra mod " << doot[WideRangeAnchor][i] << "\n"
					  << std::endl;
	}

		Smooth(doot[WideRangeAnchor], 1.f);
	return;
}

inline float
wrbs_internal(const vector<NoteInfo>& NoteInfo,
			  float music_rate,
			  const vector<int>& rows,
			  deque<int>& itv_taps,
			  deque<vector<int>>& itv_col_taps,
			  vector<int>& col_taps,
			  bool dbg)
{
	static const float min_mod = 1.f;
	static const float max_mod = 1.04f;
	int interval_taps = 0;

	bool newint1 = true;
	for (int row : rows) {
		if (dbg && newint1)
			std::cout << "interval start time: "
					  << NoteInfo[row].rowTime / music_rate << std::endl;
		newint1 = false;

		interval_taps += column_count(NoteInfo[row].notes);

		// iterate taps per col.. yes we've done this already in
		// process finger but w.e just redo it for now
		for (size_t c = 0; c < col_ids.size(); c++)
			if (NoteInfo[row].notes & col_ids[c])
				++col_taps[c];
	}

	itv_taps.push_back(interval_taps);
	itv_col_taps.push_back(col_taps);

	int window_taps = sum(itv_taps);
	vector<int> window_col_taps(4);
	for (auto& n : itv_col_taps)
		for (size_t c = 0; c < col_ids.size(); c++)
			window_col_taps[c] += n[c];

	int window_max_anch = max_val(window_col_taps);
	// shouldn't matter if the two highest are even, we just want
	// stuff like 7/2/2/3 to pop, we could also try using the second
	// highest value but that might be too volatile

	// this mod will go down if you take a runningman pattern and
	// add more notes to it outside of the anchor, note that this
	// doesn't mean the calc thinks it's now "easier", this is just
	// one component of evaluation, extra notes will increase the
	// base difficulty meaning the need to upvalue based on max
	// anchor length is decreased
	int window_taps_non_anchor = window_taps - window_max_anch;

	if (dbg) {
		std::cout << "window taps: " << window_taps << std::endl;
		std::cout << "window col 1: " << window_col_taps[0] << std::endl;
		std::cout << "window col 2: " << window_col_taps[1] << std::endl;
		std::cout << "window col 3: " << window_col_taps[2] << std::endl;
		std::cout << "window col 4: " << window_col_taps[3] << std::endl;
		std::cout << "max anchor: " << window_max_anch << std::endl;
		std::cout << "non anchor taps: " << window_taps_non_anchor << std::endl;
	}

	// nothing here or the differential is irrelevant because the
	// number of notes is too small
	if (window_max_anch < 3)
		return 1.f;
	// send out max mod i guess
	if (window_taps_non_anchor == 0)
		return max_mod;

	float pmod = static_cast<float>(window_max_anch) /
				 static_cast<float>(window_taps_non_anchor) / 2.f;
	pmod = 0.55f + fastsqrt(pmod);
	return CalcClamp(pmod, min_mod, max_mod);
}

// track general balance over a wide range
void
Calc::WideRangeBalanceScaler(const vector<NoteInfo>& NoteInfo,
							 float music_rate,
							 vector<float> doot[])
{
	const bool dbg = false && debugmode;
	doot[WideRangeBalance].resize(nervIntervals.size());

	unsigned int itv_window = 2;
	deque<int> itv_taps;
	deque<vector<int>> itv_col_taps;

	// updated every interval but recycle the memory
	vector<int> col_taps(col_ids.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {

		if (dbg) {
			for (auto row : nervIntervals[i])
				std::cout << NoteInfo[row].notes << std::endl;
			std::cout << "\n" << std::endl;
		}

		// drop the oldest interval values if we have reached full
		// size
		if (itv_taps.size() == itv_window) {
			itv_taps.pop_front();
			itv_col_taps.pop_front();
		}

		doot[WideRangeBalance][i] = wrbs_internal(NoteInfo,
												  music_rate,
												  nervIntervals[i],
												  itv_taps,
												  itv_col_taps,
												  col_taps,
												  dbg);

		// reset col taps for this interval
		for (auto& zz : col_taps)
			zz = 0;
		if (dbg)
			std::cout << "final wrb mod " << doot[WideRangeBalance][i] << "\n"
					  << std::endl;
	}

		Smooth(doot[WideRangeBalance], 1.f);
	return;
}

// look for a thing
// a thing is [aa]x[23]x[cc] where aa and cc are either [12] or [34]
// or hands that contain those jumps and where aa != cc and x's do
// not form jacks, this pattern is one staple of extremely
// jumptrillable js and even if you hit it as legit as possible,
// it's still a joke because of the way the patternage flows. i have
// tentatively proposed naming this pattern and its variants "the
// slip" after the worst aram fizz player i ever seened

// look into consistent spacing checks
void
Calc::TheThingLookerFinderThing(const vector<NoteInfo>& NoteInfo,
								float music_rate,
								vector<float> doot[])
{
	doot[TheThing].resize(nervIntervals.size());

	static const float min_mod = 0.85513412f;
	static const float max_mod = 1.f;
	unsigned int itv_window = 3;

	deque<int> itv_taps;
	deque<int> itv_THINGS;

	int lastcols = -1;
	int col_ids[4] = { 1, 2, 4, 8 };
	int the_slip = -1;
	bool malcom = false;
	int last_notes = 0;
	bool the_last_warblers_call = false;
	bool was23 = false;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		//	if (debugmode)
		//		std::cout << "new interval " << i << std::endl;

		int interval_taps = 0;
		int the_things_found = 0;

		if (itv_taps.size() == itv_window) {
			itv_taps.pop_front();
			itv_THINGS.pop_front();
		}

		bool newrow = true;
		for (int row : nervIntervals[i]) {
			// if (debugmode && newrow)
			//	std::cout << "new interval: " << i
			//			  << " time: " << NoteInfo[row].rowTime /
			// music_rate
			//			  << std::endl;
			newrow = false;
			int notes = column_count(NoteInfo[row].notes);
			int boot = NoteInfo[row].notes;
			interval_taps += notes;

			/*if (debugmode) {
				std::string moop = "";
				for (int i = 0; i < 4; ++i)
					if (boot & col_ids[i])
						moop.append(std::to_string(i + 1));
				std::cout << "notes: " << moop << std::endl;
			}*/
			// try allowing hand formations... should be ok
			if (notes == 2 || notes == 3) {
				bool is12 = boot & col_ids[0] && boot & col_ids[1];
				bool is23 = boot & col_ids[1] && boot & col_ids[2];
				bool is34 = boot & col_ids[2] && boot & col_ids[3];

				//	if (debugmode)
				//		std::cout << "the slip: " << std::endl;
				if (the_slip == -1) {
					//		if (debugmode)
					//			std::cout << "the slip: " <<
					// std::endl;
					if (is12 || is34) {
						the_slip = boot;
						//	if (debugmode)
						//		std::cout << "the slip is the boot: "
						//<<
						// std::endl;
					}
				} else {
					// if (debugmode)
					//	std::cout << "two but not the new: " <<
					// std::endl;
					if (is23) {
						if (was23) {
							//		if (debugmode)
							//			std::cout << "you knew to new to
							// yew
							// two
							// ewes: "
							//					  << std::endl;
							malcom = false;
							the_slip = -1;
							the_last_warblers_call = false;
							was23 = false;
						} else {
							//		if (debugmode)
							//			std::cout << "the malcom: "
							//<<
							// std::endl;
							malcom = true;
							the_last_warblers_call = false;
							was23 = true;
						}
					} else if (the_slip != boot && malcom &&
							   the_last_warblers_call && (is12 || is34)) {
						bool das_same = false;
						for (auto& id : col_ids)
							if (boot & id && lastcols & id) {
								// if (debugmode)
								//	std::cout << "wtf boot:" << id
								//<<
								// std::endl;
								// if (debugmode)
								//	std::cout << "wtf id: " << id <<
								// std::endl;
								das_same = true;
								break;
							}

						if (!das_same) {
							++the_things_found;
						}
						// maybe we want to reset to -1 here and
						// only retain if thing found?
						the_slip = boot;
						malcom = false;
						the_last_warblers_call = false;
						was23 = false;
					} else {
						if (is12 || is34) {
							the_slip = boot;
							//		if (debugmode)
							//			std::cout << "three four out
							// the
							// door: "
							//<<
							// std::endl;
						} else
							the_slip = -1;
						malcom = false;
						the_last_warblers_call = false;
						was23 = false;
						//		if (debugmode)
						//			std::cout << "buckle my shoe reset:
						//"
						//<<
						// std::endl;
					}
				}
			}
			if (notes == 1) {
				// if (debugmode)
				//		std::cout << "A SINGLE THING O NO: " <<
				// std::endl;
				if (the_last_warblers_call) {
					the_last_warblers_call = false;
					malcom = false;
					the_slip = -1;
					was23 = false;
					// if (debugmode)
					//	std::cout << "RESET, 2 singles: " <<
					// std::endl;
				} else {
					if (the_slip != -1) {
						bool das_same = false;
						for (auto& id : col_ids)
							if (boot & id && lastcols & id) {
								// if (debugmode)
								//	std::cout << "wtf boot:" << id
								//<<
								// std::endl;
								// if (debugmode)
								//	std::cout << "wtf id: " << id <<
								// std::endl;
								das_same = true;
								break;
							}
						if (!das_same) {
							//	if (debugmode)
							//		std::cout
							//		  << "SLIP ON SLIP UNTIL UR SLIP
							// COME
							// TRUE:"
							//		  << std::endl;
							the_last_warblers_call = true;
						} else {
							//	if (debugmode)
							//		std::cout
							//		  << "HOL UP DOE (feamale
							// hamtseer):"
							//		  << std::endl;
							the_slip = -1;
							the_last_warblers_call = false;
							was23 = false;
							malcom = false;
						}

					} else {
						the_last_warblers_call = false;
						malcom = false;
						was23 = false;
						//			if (debugmode)
						//				std::cout << "CABBAGE: " <<
						// std::endl;
					}
				}
			}

			if (notes == 4) {
				the_last_warblers_call = false;
				malcom = false;
				the_slip = -1;
				was23 = false;
				//		if (debugmode)
				//			std::cout << "RESERT, 2 SLIDE 2 FURY: "
				//<<
				// std::endl;
			}
			lastcols = boot;
		}

		itv_taps.push_back(interval_taps);
		itv_THINGS.push_back(max(the_things_found, 0));

		unsigned int window_taps = 0;
		for (auto& n : itv_taps)
			window_taps += n;

		unsigned int window_things = 0;
		for (auto& n : itv_THINGS)
			window_things += n;

		// if (debugmode)
		//	std::cout << "window taps: " << window_taps <<
		// std::endl;
		// if (debugmode)
		//	std::cout << "things: " << window_things << std::endl;

		float pmod = 1.f;
		if (window_things > 0)
			pmod = static_cast<float>(window_taps) /
				   static_cast<float>(window_things * 55);

		doot[TheThing][i] = CalcClamp((pmod), min_mod, max_mod);
		// if (debugmode)
		//	std::cout << "final mod " << doot[TheThing][i] << "\n"
		//<<
		// std::endl;
	}

		Smooth(doot[TheThing], 1.f);
		Smooth(doot[TheThing], 1.f);
	return;
}

// try to sniff out chords that are built as flams. BADLY NEEDS
// REFACTOR
void
Calc::SetFlamJamMod(const vector<NoteInfo>& NoteInfo,
					vector<float> doot[],
					float& music_rate)
{
	doot[FlamJam].resize(nervIntervals.size());
	// scan for flam chords in this window
	float grouping_tolerance = 11.f;
	// tracks which columns were seen in the current flam chord
	// this is essentially the same as if NoteInfo[row].notes
	// was tracked over multiple rows
	int cols = 0;
	// all permutations of these values are unique identifiers
	int col_id[4] = { 1, 2, 4, 8 };
	// unused atm but we might want this information, allocate once
	vector<int> flam_rows(4);
	// timing points of the elements of the flam chord, allocate
	// once
	vector<float> flamjam(4);
	// we don't actually need this counter since we can derive it
	// from cols but it might just be faster to track it locally
	// since we will be recycling the flamjam vector memory
	int flam_row_counter = 0;
	bool flamjamslamwham = false;

	// in each interval
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// build up flam detection for this interval
		vector<float> temp_mod;

		// row loop to pick up flams within the interval
		for (int row : nervIntervals[i]) {
			// perhaps we should start tracking this instead of
			// tracking it over and over....
			float scaled_time = NoteInfo[row].rowTime / music_rate * 1000.f;

			// this can be optimized a lot by properly mapping out
			// the notes value to arrow combinations (as it is
			// constructed from them) and deterministic

			// we are traversing intervals->rows->columns
			for (auto& id : col_id) {
				// check if there's a note here
				bool isnoteatcol = NoteInfo[row].notes & id;
				if (isnoteatcol) {
					// we're past the tolerance range, break if we
					// have grouped more than 1 note, or if we have
					// filled an entire quad. with this behavior if
					// we fill a quad of 192nd flams with order 1234
					// and there's still another note on 1 within
					// the tolerance range we'll flag this as a flam
					// chord and downscale appropriately, not sure
					// if we want this as it could be the case that
					// there is a second flamchord immediately
					// after, and it's just vibro, or it could be
					// the case that there are complex reasonable
					// patterns following, perhaps a different
					// behavior would be better

					// we cannot exceed tolerance without at least 1
					// note
					bool tol_exceed =
					  flam_row_counter > 0 &&
					  (scaled_time - flamjam[0]) > grouping_tolerance;

					if (tol_exceed && flam_row_counter == 1) {
						// single note, don't flag a detect
						flamjamslamwham = false;

						// reset
						flam_row_counter = 0;
						cols = 0;
					}
					if ((tol_exceed && flam_row_counter > 1) ||
						flam_row_counter == 4)
						// at least a flam jump has been detected,
						// flag it
						flamjamslamwham = true;

					// if we have identified a flam chord in some
					// way; handle and reset, we don't want to skip
					// the notes in this iteration yes this should
					// be done in the column loop since a flam can
					// start and end on any columns in any order

					// conditions to be here are at least 2
					// different columns have been logged as part of
					// a flam chord and we have exceeded the
					// tolerance for flam duration, or we have a
					// full quad flam detected, though not
					// necessarily exceeding the tolerance window.
					// we do want to reset if it doesn't, because we
					// want to pick up vibro flams and nerf them
					// into oblivion too, i think
					if (flamjamslamwham) {
						// we'll construct the final pattern mod
						// value from the flammyness and number of
						// detected flam chords
						float mod_part = 0.f;

						// lower means more cheesable means nerf
						// harder
						float fc_dur =
						  flamjam[flam_row_counter - 1] - flamjam[0];

						// we don't want to affect explicit chords,
						// but we have to be sure that the entire
						// flam we've picked up is an actual chord
						// and only an actual chord, if the first
						// and last elements detected were on the
						// same row, ignore it, trying to do fc_dur
						// == 0.f didn't work because of float
						// precision
						if (flam_rows[0] != flam_rows[flam_row_counter - 1]) {
							// basic linear scale for testing
							// purposes, scaled to the window length
							// and also flam size
							mod_part =
							  fc_dur / grouping_tolerance / flam_row_counter;
							temp_mod.push_back(mod_part);
						}

						// reset
						flam_row_counter = 0;
						cols = 0;
						flamjamslamwham = false;
					}

					// we know chord flams can't contain multiple
					// notes of the same column (those are just
					// gluts), reset if detected even within the
					// tolerance range (we can't be outside of it
					// here by definition)
					if (cols & id) {
						flamjamslamwham = false;

						// reset
						flam_row_counter = 0;
						cols = 0;
					}

					// conditions to reach here are that a note in
					// this column has not been logged yet and we
					// are still within the grouping tolerance. we
					// don't need cur/last times here, the time of
					// the first element will be used to determine
					// the size of the total group

					// track the time point of this note
					flamjam[flam_row_counter] = scaled_time;
					// track which row its on
					flam_rows[flam_row_counter] = row;

					// update unique column identifier
					cols += id;
					++flam_row_counter;
				}
			}
		}

		// finishing the row loop leaves us with instances of
		// flamjams forgive a single instance of a chord flam for
		// now; handle none
		if (temp_mod.size() < 2)
			doot[FlamJam][i] = 1.f;

		float wee = 0.f;
		for (auto& v : temp_mod)
			wee += v;

		// we can do this for now without worring about /0 since
		// size is at least 2 to get here
		wee /= static_cast<float>(temp_mod.size() - 1);

		wee = CalcClamp(1.f - wee, 0.5f, 1.f);
		doot[FlamJam][i] = wee;

		// reset the stuffs, _theoretically_ since we are sequencing
		// we don't even need at all to clear the flam detection
		// however then we have to handle cases like a single note
		// in an interval and i don't feel like doing that, a small
		// number of flams that happen to straddle the interval
		// splice points shouldn't make a huge difference, but if
		// they do then we should deal with it
		temp_mod.clear();
		flam_row_counter = 0;
		cols = 0;
	}
		Smooth(doot[FlamJam], 1.f);
}

#pragma endregion

static const float ssr_goal_cap = 0.965f; // goal cap to prevent insane scaling
#pragma region thedoots
// Function to generate SSR rating
vector<float>
MinaSDCalc(const vector<NoteInfo>& NoteInfo, float musicrate, float goal)
{
	if (NoteInfo.size() <= 1)
		return dimples_the_all_zero_output;
	return std::make_unique<Calc>()->CalcMain(
	  NoteInfo, musicrate, min(goal, ssr_goal_cap));
}

// Wrap difficulty calculation for all standard rates
MinaSD
MinaSDCalc(const vector<NoteInfo>& NoteInfo)
{
	MinaSD allrates;
	int lower_rate = 7;
	int upper_rate = 21;

	if (NoteInfo.size() > 1) {
		std::unique_ptr<Calc> cacheRun = std::make_unique<Calc>();
		cacheRun->ssr = false;
		for (int i = lower_rate; i < upper_rate; i++) {
			allrates.emplace_back(cacheRun->CalcMain(
			  NoteInfo, static_cast<float>(i) / 10.f, 0.93f));
		}
	} else
		for (int i = lower_rate; i < upper_rate; i++)
			allrates.emplace_back(dimples_the_all_zero_output);
	return allrates;
}

// Debug output
void
MinaSDCalcDebug(const vector<NoteInfo>& NoteInfo,
				float musicrate,
				float goal,
				vector<vector<vector<vector<float>>>>& handInfo)
{
	if (NoteInfo.size() <= 1)
		return;

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

int mina_calc_version = 350;
int
GetCalcVersion()
{
#ifdef USING_NEW_CALC
	return mina_calc_version;
#else
	return 263;
#endif
}
