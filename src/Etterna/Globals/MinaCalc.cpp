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

// intervals are _half_ second, no pointing time or cpu cycles on 100 nps joke
// files
static const int max_nps_for_single_interval = 50;
static const vector<float> dimples_the_all_zero_output{ 0.f, 0.f, 0.f, 0.f,
														0.f, 0.f, 0.f, 0.f };
static const vector<float> gertrude_the_all_max_output{ 100.f, 100.f, 100.f,
														100.f, 100.f, 100.f,
														100.f, 100.f };
static const vector<int> col_ids = { 1, 2, 4, 8 };
static const int zto3[4] = { 0, 1, 2, 3 };
struct JumpHandChordData
{
	vector<int> num_row_variations;
	// man these names are cryptic
	vector<int> definitely_not_jacks;
	vector<int> actual_jacks;
	vector<int> actual_jacks_cj;
	vector<int> not_js;
	vector<int> not_hs;
	vector<unsigned int> taps;
	vector<unsigned int> jumptaps;
	vector<unsigned int> handtaps;
	vector<unsigned int> chordtaps;
	vector<unsigned int> quads;
};
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

inline void
truncate_or_fill_to_size(vector<float>& v, unsigned int n, float dummy_value)
{
	unsigned int old = v.size();
	// we could do this outside but w.e
	if (old == n)
		return;

	// truncate if over
	if (old > n)
		v.resize(n);
	else
		// fill if under
		for (size_t i = 0; i < n - old; ++i)
			v.push_back(dummy_value);
}

template<typename T>
inline T
CalcClamp(T x, T l, T h)
{
	return x > h ? h : (x < l ? l : x);
}

// template thingy for generating basic proportion scalers for pattern mods
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
};

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

vector<float>
skillset_vector(const DifficultyRating& difficulty)
{
	return vector<float>{ difficulty.overall,	difficulty.stream,
						  difficulty.jumpstream, difficulty.handstream,
						  difficulty.stamina,	difficulty.jack,
						  difficulty.chordjack,  difficulty.technical };
}

inline float
highest_difficulty(const DifficultyRating& difficulty)
{
	auto v = { difficulty.stream,	 difficulty.jumpstream,
			   difficulty.handstream, difficulty.stamina,
			   difficulty.jack,		  difficulty.chordjack,
			   difficulty.technical };
	return *std::max_element(v.begin(), v.end());
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
#pragma endregion utils

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
static const float basescalers[NUM_Skillset] = { 0.f,   0.97f,   0.875f, 0.89f,
												 0.94f, 0.7675f, 0.84f,  0.7f };
bool debug_lmao = false;

#pragma region patternmodparamstuff
static const std::string calc_params_xml = "calc params.xml";

#pragma endregion

#pragma region CalcBodyFunctions
#pragma region JackModelFunctions
// SOMEHOW MAKES JAKES EASIER SO DISABLED FOR NOW (it's also sort of redundant
// with the entire system as it is so this may not be needed
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
		static const float
		  actual_literal_black_magic_number_random_HAHAHAHA____ = 1.f;
		// yes i know how dumb this looks
		DifficultyRating difficulty = {
			mcbloop[0],
			mcbloop[1],
			mcbloop[2],
			mcbloop[3],
			mcbloop[4],
			mcbloop[5],
			mcbloop[6],
			mcbloop[7] * actual_literal_black_magic_number_random_HAHAHAHA____
		};
		vector<float> pumpkin = skillset_vector(difficulty);
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

		difficulty.overall = highest_difficulty(difficulty);

		// the final push down, cap ssrs (score specific
		// ratings) to stop vibro garbage and calc abuse
		// from polluting leaderboards too much, a "true" 38
		// is still unachieved so a cap of 40 [sic] is
		// _extremely_ generous do this for SCORES only, not
		// cached file difficulties
		auto bye_vibro_maybe_yes_this_should_be_refactored_lul =
		  skillset_vector(difficulty);
		if (ssr) {
			static const float ssrcap = 40.f;

			for (auto& r : bye_vibro_maybe_yes_this_should_be_refactored_lul) {
				// so 50%s on 60s don't give 35s
				r = downscale_low_accuracy_scores(r, score_goal);
				r = CalcClamp(r, r, ssrcap);
			}
		}
		for (size_t bagles = 0;
			 bagles < bye_vibro_maybe_yes_this_should_be_refactored_lul.size();
			 ++bagles)
			the_hizzle_dizzles[WHAT_IS_EVEN_HAPPEN_THE_BOMB].push_back(
			  bye_vibro_maybe_yes_this_should_be_refactored_lul[bagles]);
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

#pragma region sequencelogicstuffmaybe
// cross column behavior between 2 notes
enum cc_type
{
	cc_left_right,
	cc_right_left,
	cc_jump_single,
	cc_single_single,
	cc_single_jump,
	cc_jump_jump,
	cc_empty,
	cc_init,
	cc_undefined
};

// for either hand, 4k
enum col_type
{
	col_left,
	col_right,
	col_ohjump,
	col_empty,
	col_init
};

inline bool
is_single_tap(const bool& a, const bool& b)
{
	return a ^ b;
}

inline bool
is_single_tap(const cc_type& cc)
{
	return cc == cc_left_right || cc == cc_right_left || cc == cc_jump_single;
}

inline bool
is_single_tap(const col_type& col)
{
	return col == col_left || col == col_right;
}

inline bool
is_empty_row(const bool& a, const bool& b)
{
	return !a && !b;
}
// this can be inferred from !empty and !single in most cases
inline bool
is_jump(const bool& a, const bool& b)
{
	return a && b;
}

inline bool
is_jump(const cc_type& cc)
{
	return cc == cc_jump_jump || cc == cc_single_jump;
}

inline cc_type
determine_cc_type(const col_type& last, const col_type& now)
{
	if (now == col_empty)
		return cc_empty;
	else if (last == col_init)
		return cc_init;

	bool single_tap = is_single_tap(now);
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
	else
		// anchor/jack
		return cc_single_single;

	// shouldn't ever happen
	return cc_undefined;
}
static const float s_init = -5.f;
static const float ms_init = 5000.f;

// pattern info and stuff idk this needed to be done a bazillion years ago
struct metanoteinfo
{
	int id = 0;
	float time = ms_init;

	col_type col = col_init;

	// type of cross column hit
	cc_type cc = cc_init;

	unsigned int count = 0;

	bool last_was_offhand_tap = false;

	// ms from last cross column note
	float cc_ms_any = ms_init;

	// ms from last cross column note, excluding jumps
	float cc_ms_no_jumps = ms_init;

	// ms from last note in this column
	float tc_ms = ms_init;
};

//	unused but example
// bool
// should_log_cross_columns_for_ms_base(cc_type cc)
//{
//	switch (cc) {
//		case cc_left_right:
//		case cc_right_left:
//		case cc_jump_single:
//		case cc_single_jump:
//			return true;
//		case cc_single_single:
//		case cc_jump_jump:
//		case cc_empty:
//		case cc_was_init:
//		case cc_undefined:
//		default:
//			return false;
//	}
//}

inline col_type
bool_to_col_type(const bool& lcol, const bool& rcol)
{
	if (is_empty_row(lcol, rcol))
		return col_empty;
	if (lcol - rcol)
		return lcol ? col_left : col_right;
	return col_ohjump;
};

inline void
set_col_and_cc_types(metanoteinfo& mni,
					 const bool& lcol,
					 const bool& rcol,
					 const col_type& last_col)
{
	mni.col = bool_to_col_type(lcol, rcol);
	mni.cc = determine_cc_type(last_col, mni.col);
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
};

// inverting col state for col_left or col_right only
inline col_type
invert_col(const col_type& col)
{
	// this should crash or something idk.. this is just for flipping
	// left->right and vice versa to be used in indexing... dangerous if misused
	// due to unpredictable/undefined behavior
	if (col != col_left && col != col_right)
		return col_init;
	return col == col_left ? col_right : col_left;
};

// inverting cc state for left_right or right_left only
inline cc_type
invert_cc(const cc_type& cc)
{
	// this should also crash, but it's not as dangerous as above
	if (cc != cc_left_right && cc != cc_right_left)
		return cc_init;
	return cc == cc_left_right ? cc_right_left : cc_left_right;
};

inline void
set_metanoteinfo_timings(metanoteinfo& mni,
						 const float cur[2],
						 const float last[2],
						 const col_type& last_col)
{
	switch (mni.cc) {
		case cc_left_right:
		case cc_right_left:
		case cc_jump_single:
		case cc_single_single:
			// either we know the end col so we know the start col, or the start
			// col doesn't matter
			mni.cc_ms_any = ms_from(cur[mni.col], last[invert_col(mni.col)]);

			// technically doesn't matter if we use last_col to index, if it's
			// single -> single we know it's an anchor so it's more intuitive to
			// use mni.col twice
			mni.tc_ms = ms_from(cur[mni.col], last[mni.col]);
			break;
		case cc_single_jump:
			// tracking this for now, we want to track from last col to last col
			// inverted
			mni.cc_ms_any = ms_from(cur[invert_col(last_col)], last[last_col]);

			// can't use mni.col twice, use last_col, see below
			mni.tc_ms = ms_from(cur[0], last[last_col]);
			break;
		case cc_jump_jump:
			// not sure if we should set or leave at init value of 5000.f
			// mni.cc_ms_any = 0.f;

			// indexes don't matter-- except that we can't use mni.col or
			// last_col (because index 2 is outside array size)
			mni.tc_ms = ms_from(cur[0], last[0]);
			break;
		case cc_empty:
			break;
		case cc_init:
			break;
		case cc_undefined:
			break;
		default:
			break;
	}
	return;
}

// ranmen staff
static const int max_oht_len = 2;
static const int max_off_spacing = 2;
static const int max_burst_len = 6;
static const int max_jack_len = 1;

inline bool
is_oht(const cc_type& a, const cc_type& b, const cc_type& c)
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

struct RM_Sequencing
{
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

	float temp_ms = 0.f;

	void reset();
	void handle_off_tap(const float& now);
	void handle_off_tap_completion();
	void handle_off_tap_progression(const bool& completing);
	void handle_anchor_progression(const float& now);
	void handle_jack_progression();
	void handle_cross_column_branching(const cc_type& cc, const float& now);
};

inline void
RM_Sequencing::reset()
{
	// don't reset anchor_col or last_col
	// we want to preserve the pattern state
	// reset everything else tho

	is_bursting = false;
	had_burst = false;
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
}

inline void
RM_Sequencing::handle_off_tap(const float& now)
{
	last_off_time = now;

	++ran_taps;
	++off_taps;
	++off_len;

	// offnote, reset jack length & oht length
	jack_len = 0;

	off_total_ms += ms_from(now, last_anchor_time);

	// handle progression for increasing off_len
	handle_off_tap_progression(false);

	// rolls
	if (off_len == max_off_spacing) {
		// ok do nothing for now i might have a better idea
	}
}

inline void
RM_Sequencing::handle_off_tap_completion()
{
	// if we end while bursting due to hitting an anchor, complete it
	if (is_bursting) {
		is_bursting = false;
		had_burst = true;
	}
	// reset off_len counter
	off_len = 0;
}

inline void
RM_Sequencing::handle_off_tap_progression(const bool& completing)
{
	// handle ending off tap progression due to jacks or anchors
	if (completing) {
		handle_off_tap_completion();
		// below is for increasing off tap logic, skip
		return;
	}

	// resume off tap progression caused by another consecutive off tap
	// normal behavior if we have already allowed for 1 burst, reset if the
	// offtap sequence exceeds the spacing limit; this will also catch bursts
	// that exceed the max burst length
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

inline void
RM_Sequencing::handle_anchor_progression(const float& now)
{
	temp_ms = ms_from(now, last_anchor_time);
	// account for float precision error and small bpm flux
	if (temp_ms > max_ms + 5.f)
		reset();
	else
		max_ms = temp_ms;

	last_anchor_time = now;
	++ran_taps;
	++anchor_len;

	// handle completion of off tap progression
	handle_off_tap_progression(true);
}

inline void
RM_Sequencing::handle_jack_progression()
{
	++ran_taps;
	++anchor_len; // do this for jacks?
	++jack_len;
	++jack_taps;

	// handle completion of off tap progression
	handle_off_tap_progression(true);

	// make sure to set the anchor col when resetting if we exceed max jack len
	if (jack_len > max_jack_len)
		reset();
}

inline void
RM_Sequencing::handle_cross_column_branching(const cc_type& cc,
											 const float& now)
{
	// we are comparing 2 different enum types here, but this is what we want.
	// cc_left_right is 0, col_left is 0. if we are cc_left_right then we have
	// landed on the right column, so if we have cc (0) == anchor_col (0), we
	// are entering the off column (right) of the anchor (left).
	// perhaps left_right and right_left should be flipped in the
	// cc_type enum to make this more intuitive (but probably not)

	// NOT an anchor
	if (anchor_col == cc) {
		handle_off_tap(now);
		// same hand offtap
		++off_taps_same;
		return;
	}
	handle_anchor_progression(now);
};

struct RunningMen
{
	const vector<int> _pmods{ RanMan,		 RanLen,	  RanAnchLen,
							  RanAnchLenMod, RanJack,	 RanOHT,
							  RanOffS,		 RanPropAll,  RanPropOff,
							  RanPropOHT,	RanPropOffS, RanPropJack };

	const std::string name = "RunningMen";

	// params
	float min_mod = 0.95;
	float max_mod = 1.5f;
	float mod_base = 1.f;
	float min_anchor_len = 4.f;
	float min_taps_in_rm = 1.f;
	float min_off_taps_same = 1.f;

	float total_prop_scaler = 1.f;
	float total_prop_min = 0.f;
	float total_prop_max = 1.f;

	float off_tap_prop_scaler = 1.f;
	float off_tap_prop_min = 0.f;
	float off_tap_prop_max = 1.f;

	float off_tap_same_base = 0.5f;
	float off_tap_same_prop_scaler = 1.f;
	float off_tap_same_prop_min = 0.f;
	float off_tap_same_prop_max = 1.25f;

	float anchor_len_divisor = 2.5f;

	float min_jack_taps_for_bonus = 1.f;
	float jack_bonus_base = 0.1f;

	float min_oht_taps_for_bonus = 1.f;
	float oht_bonus_base = 0.1f;

	std::map<std::string, float*> param_map{
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
		{ "off_tap_same_base", &off_tap_same_base },

		{ "off_tap_same_prop_scaler", &off_tap_same_prop_scaler },
		{ "off_tap_same_prop_min", &off_tap_same_prop_min },
		{ "off_tap_same_prop_max", &off_tap_same_prop_max },

		{ "anchor_len_divisor", &anchor_len_divisor },

		{ "min_jack_taps_for_bonus", &min_jack_taps_for_bonus },
		{ "jack_bonus_base", &jack_bonus_base },

		{ "min_oht_taps_for_bonus", &min_oht_taps_for_bonus },
		{ "oht_bonus_base", &oht_bonus_base }
	};

	// stuff for making mod
	float total_prop = 0.f;
	float off_tap_prop = 0.f;
	float off_tap_same_prop = 0.f;
	float anchor_len_comp = 0.f;
	float jack_bonus = 0.f;
	float oht_bonus = 0.f;
	float pmod = min_mod;
	void resize_own_pmods(vector<float> doot[ModCount], const int& size)
	{
		floop();
		for (auto& mod : _pmods)
			doot[mod].resize(size);
	};
	void operator()(const RM_Sequencing& rm,
					vector<float> doot[ModCount],
					const int& i);

	XNode* CreateParamNode() const;

	void floop();
};

// uhh reminder to self to make this not load values every time thing is done
// and thing and stuff, probably
void
RunningMen::floop()
{
	std::string fn = "Save/" + calc_params_xml;
	int iError;
	std::unique_ptr<RageFileBasic> pFile(
	  FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL)
		return;

	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return;

	CHECKPOINT_M("Loading the Param node.");

	// auto* pmod = xml.GetChild(name + "ModParams");

	for (auto& p : param_map) {
		auto* ch = xml.GetChild(p.first);
		float boat = 0.f;
		ch->GetTextValue(boat);
		*p.second = boat;
	}
};

XNode*
RunningMen::CreateParamNode() const
{
	XNode* pmod = new XNode(name + "ModParams");
	for (auto& p : param_map)
		pmod->AppendChild(p.first, to_string(*p.second));

	return pmod;
}

void
SavePatternModParamXmlToDir()
{
	RunningMen zoop;

	string fn = "Save/" + calc_params_xml;
	unique_ptr<XNode> xml(zoop.CreateParamNode());
	string err;
	RageFile f;
	if (!f.Open(fn, RageFile::WRITE))
		return;
	XmlFileUtil::SaveToFile(xml.get(), f, "", false);
}

//
// XNode*
// Calc::SaveParamNode() const
//{
//	CHECKPOINT_M("Saving calc param nodes.");
//
//	XNode* calctestlists = new XNode("ParamNodes");
//	FOREACHM_CONST(Skillset, CalcTestList,
// testChartList, i)
//	calctestlists->AppendChild(i->second.CreateNode());
//	return calctestlists;
//}

// we either want this setup this way or not...
void
RunningMen::operator()(const RM_Sequencing& rm,
					   vector<float> doot[ModCount],
					   const int& i)
{
	if (rm.anchor_len < min_anchor_len) {
		doot[RanMan][i] = min_mod;
		return;
	} else if (rm.ran_taps < min_taps_in_rm) {
		doot[RanMan][i] = min_mod;
		return;
	} else if (rm.off_taps_same < min_off_taps_same) {
		doot[RanMan][i] = min_mod;
		return;
	}

	// taps in runningman / total taps in interval... i think? can't remember
	// when i reset total taps tbh.. this might be useless
	total_prop = pmod_prop(rm.ran_taps,
						   rm.total_taps,
						   total_prop_scaler,
						   total_prop_min,
						   total_prop_max);

	// number anchor taps / number of non anchor taps
	off_tap_prop = pmod_prop(rm.anchor_len,
							 rm.ran_taps,
							 off_tap_prop_scaler,
							 off_tap_prop_min,
							 off_tap_prop_max);

	// number of same hand off anchor taps / anchor taps, basically stuff is
	// really hard when this is high (a value of 0.5 is a triplet every other
	// anchor)
	off_tap_same_prop = pmod_prop(rm.off_taps_same,
								  rm.anchor_len,
								  off_tap_same_prop_scaler,
								  off_tap_same_prop_min,
								  off_tap_same_prop_max,
								  off_tap_same_base);

	// anchor length component
	anchor_len_comp = static_cast<float>(rm.anchor_len) / anchor_len_divisor;

	// jacks in anchor component, give a small bonus i guess
	jack_bonus =
	  rm.jack_taps >= min_jack_taps_for_bonus ? jack_bonus_base : 0.f;

	// ohts in anchor component, give a small bonus i guess
	// not done
	oht_bonus = rm.oht_taps >= min_oht_taps_for_bonus ? oht_bonus_base : 0.f;

	// we could scale the anchor to speed if we want but meh
	// that's really complicated/messy/error prone
	pmod = anchor_len_comp + jack_bonus + oht_bonus + mod_base;
	pmod = CalcClamp(
	  pmod * total_prop * off_tap_prop * off_tap_same_prop, min_mod, max_mod);

	// actual used mod
	doot[RanMan][i] = pmod;

	// debug
	doot[RanLen][i] = (static_cast<float>(rm.total_taps) / 100.f) + 0.5f;
	doot[RanAnchLen][i] = (static_cast<float>(rm.anchor_len) / 30.f) + 0.5f;
	doot[RanAnchLenMod][i] = anchor_len_comp;
	doot[RanOHT][i] = static_cast<float>(rm.oht_taps);
	doot[RanOffS][i] = static_cast<float>(rm.off_taps_same);
	doot[RanJack][i] = static_cast<float>(rm.jack_taps);
	doot[RanPropAll][i] = total_prop;
	doot[RanPropOff][i] = off_tap_prop;
	doot[RanPropOffS][i] = off_tap_same_prop;
	doot[RanPropOHT][i] = oht_bonus;
	doot[RanPropJack][i] = jack_bonus;
};

#pragma endregion
vector<vector<metanoteinfo>>
gen_metanoteinfo(const vector<vector<int>>& itv_rows,
				 const vector<NoteInfo>& NoteInfo,
				 float music_rate,
				 int t1,
				 int t2,
				 vector<float> doot[ModCount])
{
	RunningMen zorp;
	zorp.resize_own_pmods(doot, itv_rows.size());
	const bool dbg = false && debug_lmao;
	vector<vector<metanoteinfo>> o;
	vector<metanoteinfo> p;

	float lasttime[2] = { s_init, s_init };
	float curtime[2] = { 0.f, 0.f };
	float this_ms = ms_init;

	col_type last_col = col_init;
	cc_type last_cc = cc_init;

	// test stuff
	RM_Sequencing rms[2];
	// maybe the easy way to do this is just run both columns at once
	rms[0].anchor_col = col_left;
	rms[1].anchor_col = col_right;
	int cols = 0;

	col_type ran_last = col_init;
	bool offhand_tap = false;
	bool was_last_offhand_tap = false;
	for (size_t i = 0; i < itv_rows.size(); ++i) {
		RM_Sequencing rm_to_use_for_mods;
		p.clear();
		auto& itv = itv_rows[i];
		for (auto& row : itv) {
			metanoteinfo mni;
			set_col_and_cc_types(mni,
								 NoteInfo[row].notes & t1,
								 NoteInfo[row].notes & t2,
								 last_col);

			// test stuff
			// define anchor as spaced jacks, either by 1 note or 2, cancel on 3
			// allow some tolerance for jacks and pure ohts, something something
			// cancel if the anchor'd column exceeds a time boundary
			{
				// nerv always has at least 1 tap, if this hand has nothing
				// on it, then we are by definition seeing an offhand tap
				offhand_tap = false;
				if (mni.col == col_empty)
					offhand_tap = true;

			mni.count = column_count(NoteInfo[row].notes);
			mni.time = NoteInfo[row].rowTime;
			float bort = mni.time = NoteInfo[row].rowTime;
				for (auto& rm : rms) {
					rm.total_taps += mni.count;

					// oht
					if (is_oht(rm.last_last_cc, rm.last_cc, mni.cc)) {
						++rm.oht_len;
						++rm.oht_taps;
						if (rm.oht_len > max_oht_len)
							rm.reset();
					}

					// if (offhand_tap) {
					//	// moved to the cc_empty case in the swtich cause i
					//	// think these are identical conditions
					//} else {
					switch (mni.cc) {
						case cc_left_right:
						case cc_right_left:
							rm.handle_cross_column_branching(mni.cc, bort);
							break;
						case cc_jump_single:
							if (was_last_offhand_tap) {
								// if we have a jump -> single, and the last
								// note was an offhand tap, and the single
								// is the anchor col, then we have an anchor
								if ((mni.col == col_left &&
									 rm.anchor_col == col_left) ||
									(mni.col == col_right &&
									 rm.anchor_col == col_right)) {
									rm.handle_anchor_progression(bort);
								} else {
									// otherwise we have an off anchor tap
									rm.handle_off_tap(bort);
									// same hand offtap
									++rm.off_taps_same;
								}
							} else {
								// if we are jump -> single and the last
								// note was not an offhand hand tap, we have
								// a jack
								rm.handle_jack_progression();
							}
							break;
						case cc_single_single:
							if (was_last_offhand_tap) {
								// if this wasn't a jack, then it's just
								// a good ol anchor
								rm.handle_anchor_progression(bort);
							} else {
								// a jack, not an anchor, we don't
								// want too many of these but we
								// don't want to allow none of them
								rm.handle_jack_progression();
							}
							break;
						case cc_single_jump:
							// if last note was an offhand tap, this is by
							// definition part of the anchor
							if (was_last_offhand_tap) {
								rm.handle_anchor_progression(bort);
							} else {
								// if not, a jack
								rm.handle_jack_progression();
							}
							break;
						case cc_jump_jump:
							// this is kind of a grey area, given that
							// the difficulty of runningmen comes from
							// the tight turns on the same hand... we
							// will treat this as a jack even though
							// technically it's an "anchor" when the
							// last tap was an offhand tap
							rm.handle_jack_progression();
							break;
						case cc_empty:
							// simple case to handle, can't be a jack (or
							// doesn't
							// really matter) and can't be oht, only reset
							// if we exceed the spacing limit
							rm.handle_off_tap(bort);
							break;
						case cc_init:
							// uhh we could do something here but i'm lazy
							break;
						case cc_undefined:
							break;
						default:
							break;
					}
					rm.last_last_cc = rm.last_cc;
					rm.last_cc = mni.cc;
				}

				// use the biggest anchor that has existed in this interval
				int test = rms[0].anchor_len > rms[1].anchor_len ? 0 : 1;

				if (rms[test].anchor_len > rm_to_use_for_mods.anchor_len)
					rm_to_use_for_mods = rms[test];

				ran_last = mni.col;
				was_last_offhand_tap = offhand_tap;
			}

			mni.last_was_offhand_tap = offhand_tap;
			// we don't want to set lasttime or lastcol for empty rows
			if (mni.col == col_empty)
				continue;

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
			update_col_time(mni.col, curtime, NoteInfo[row].rowTime);
			set_metanoteinfo_timings(mni, curtime, lasttime, last_col);

			// update last notes/time
			last_col = mni.col;
			last_cc = mni.cc;
			lasttime[0] = curtime[0];
			lasttime[1] = curtime[1];
			p.push_back(mni);
		}
		o.push_back(p);

		// continue test stuff
		static const float min_mod = 0.95f;
		static const float max_mod = 1.5f;
		doot[RanMan][i] = min_mod;
		doot[RanLen][i] = 0.f;
		doot[RanAnchLen][i] = 0.f;
		doot[RanOffS][i] = 0.f;
		doot[RanJack][i] = 0.f;
		doot[RanPropAll][i] = 0.f;
		doot[RanPropOff][i] = 0.f;
		doot[RanPropOffS][i] = 0.f;
		doot[RanPropOHT][i] = 0.f;
		doot[RanPropJack][i] = 0.f;
		
		zorp(rm_to_use_for_mods, doot, i);
	}
	// Smooth(doot[RanMan], 1.f);
	// Smooth(doot[RanMan], 1.f);
	return o;
}

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

	pair<Hand&, vector<int>> spoopy[2] = { { left_hand, { 1, 2 } },
										   { right_hand, { 4, 8 } } };

	vector<vector<vector<metanoteinfo>>> bruh;

	// loop to help with hand specific stuff, we could do this stuff
	// in the class but that's more structural work and this is
	// simple
	for (auto& hp : spoopy) {
		auto& hand = hp.first;
		const auto& fv = hp.second;

		// these definitely do change with every chisel test
		hand.stam_adj_diff.resize(numitv);

		// initialize base difficulty and point values
		// ok i know this is messy and the loop doesn't solve
		// anything here and almost defeats the purpose but
		// whatever, we need to do this before the pmods
		if (fv[0] == 1) {
			bruh.emplace_back(gen_metanoteinfo(nervIntervals,
											   NoteInfo,
											   music_rate,
											   col_ids[0],
											   col_ids[1],
											   hand.doot));
			hand.InitBaseDiff(fingers[0], fingers[1]);
			hand.InitPoints(fingers[0], fingers[1]);
		} else {
			bruh.emplace_back(gen_metanoteinfo(nervIntervals,
											   NoteInfo,
											   music_rate,
											   col_ids[2],
											   col_ids[3],
											   hand.doot));
			hand.InitBaseDiff(fingers[2], fingers[3]);
			hand.InitPoints(fingers[2], fingers[3]);
		}
		SetAnchorMod(NoteInfo, fv[0], fv[1], hand.doot);
		SetSequentialDownscalers(NoteInfo, fv[0], fv[1], music_rate, hand.doot);
		WideRangeRollScaler(NoteInfo, fv[0], fv[1], music_rate, hand.doot);
		WideRangeJumptrillScaler(NoteInfo, fv[0], fv[1], music_rate, hand.doot);
	}

	auto jhc_data = gen_jump_hand_chord_data(NoteInfo);
	// these are evaluated on all columns so right and left are the
	// same these also may be redundant with updated stuff
	SetHSMod(jhc_data, left_hand.doot);
	SetJumpMod(jhc_data, left_hand.doot);
	SetCJMod(jhc_data, left_hand.doot);
	SetStreamMod(NoteInfo, left_hand.doot, music_rate);
	SetFlamJamMod(NoteInfo, left_hand.doot, music_rate);
	TheThingLookerFinderThing(NoteInfo, music_rate, left_hand.doot);
	WideRangeBalanceScaler(NoteInfo, music_rate, left_hand.doot);
	WideRangeAnchorScaler(NoteInfo, music_rate, left_hand.doot);

	vector<int> bruh_they_the_same = { HS,
									   HSS,
									   HSJ,
									   JS,
									   JSS,
									   JSJ,
									   CJ,
									   CJS,
									   CJJ,
									   CJQuad,
									   StreamMod,
									   Chaos,
									   FlamJam,
									   TheThing,
									   WideRangeBalance,
									   WideRangeAnchor };
	// hand agnostic mods are the same
	for (auto pmod : bruh_they_the_same)
		right_hand.doot[pmod] = left_hand.doot[pmod];

	// loop to help with hand specific stuff
	for (auto& hp : spoopy) {
		auto& hand = hp.first;
		const auto& fv = hp.second;

		// needs to be done after pattern mods are calculated
		hand.InitAdjDiff();

		// pattern mods and base msd never change, set degbug output
		// for them now
		if (debugmode) {
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

	// werwerwer
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
	return true;
}

// DON'T refpass, since we manipulate the vector and this is done before
// jackseq, if we shuffle stuff around so this is done after jackseq and
// we're sure we don't need to use this for anything else we can probably
// refpass again but cba to test atm
float
Hand::CalcMSEstimate(vector<float> input, int burp)
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
	static const float ms_dummy = 360.f;
	truncate_or_fill_to_size(input, num_used, ms_dummy);

	// mostly try to push down stuff like jumpjacks, not necessarily to push
	// up "complex" stuff
	float cv_yo = cv(input) + 0.5f;
	cv_yo = CalcClamp(cv_yo, 0.5f, 1.25f);

	sort(input.begin(), input.end());
	float comb_cc = cv(input) + 1.f;

	if (dbg && debug_lmao)
		std::cout << "cv in: " << cv_yo << " : cv comb: " << comb_cc
				  << std::endl;

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
	float m = 0.f;
	for (auto& ms : input)
		m += ms;

	if (dbg && debug_lmao)
		std::cout << "m : " << m << std::endl;

	// add 1 to num_used because some meme about sampling
	// same thing as jack stuff, convert to bpm and then nps
	float bpm_est = ms_to_bpm(m / (num_used + 1));
	float nps_est = bpm_est / 15.f;
	float fdiff = nps_est * comb_cc;
	if (dbg && debug_lmao)
		std::cout << "diff : " << fdiff << std::endl;
	return fdiff;
}

inline float
a_thing(float a, float b, float x, float y)
{
	return (x * a + ((y - x) * b)) / y;
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
		float left_difficulty =
		  max(CalcMSEstimate(f1[i], 3),
			  CalcMSEstimate(f1[i], 4) * higher_thing_scaler);
		left_difficulty = max(left_difficulty,
							  CalcMSEstimate(f1[i], 5) * higher_thing_scaler *
								higher_thing_scaler);
		float right_difficulty =
		  max(CalcMSEstimate(f2[i], 3),
			  CalcMSEstimate(f2[i], 4) * higher_thing_scaler);
		right_difficulty = max(right_difficulty,
							   CalcMSEstimate(f2[i], 5) * higher_thing_scaler *
								 higher_thing_scaler);

		float difficulty = 0.f;
		float squiggly_line = 6.5f;
		if (left_difficulty > right_difficulty)
			difficulty =
			  a_thing(left_difficulty, right_difficulty, squiggly_line, 9.f);
		else
			difficulty =
			  a_thing(right_difficulty, left_difficulty, squiggly_line, 9.f);
		soap[BaseNPS][i] = finalscaler * nps;
		soap[BaseMS][i] = finalscaler * difficulty;
		soap[BaseMSD][i] = a_thing(difficulty, nps, 3.5f, 10.f) * finalscaler;
	}
	Smooth(soap[BaseNPS], 0.f);
	Smooth(soap[BaseMSD], 0.f);
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
		  StreamMod,
		  Roll,
		  Chaos,
		  WideRangeRoll,
		  WideRangeJumptrill,
		  FlamJam,
		  OHJump,
		  Anchor,
		  WideRangeBalance,
		},

		// js
		{
		  JS,
		  Chaos,
		  OHJump,
		  TheThing,
		  Anchor,
		  WideRangeBalance,
		},

		// hs
		{
		  HS,
		  Chaos,
		  OHJump,
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
		  OHJump,
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
		  Anchor,
		  Roll,
		  OHJump,
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
					// jank application of ranman
					adj_diff *= CalcClamp(doot[RanMan][i], 1.f, 1.1f);
					break;

				// test calculating stam for js/hs on max js/hs diff
				// we want hs to count against js so they are
				// mutually exclusive
				case Skill_Jumpstream:
					adj_diff /=
					  max(doot[HS][i], 1.f) * fastsqrt(doot[OHJump][i]);
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
						adj_diff = soap[BaseMS][i] * tp_mods[ss] *
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

JumpHandChordData
Calc::gen_jump_hand_chord_data(const vector<NoteInfo>& NoteInfo)
{
	const bool dbg = false && debugmode;
	const bool dbg_lv2 = false && debugmode;
	JumpHandChordData data;
	// these reserve statements should match the struct fields
	data.num_row_variations.reserve(nervIntervals.size());
	data.definitely_not_jacks.reserve(nervIntervals.size());
	data.actual_jacks.reserve(nervIntervals.size());
	data.actual_jacks_cj.reserve(nervIntervals.size());
	data.not_js.reserve(nervIntervals.size());
	data.not_hs.reserve(nervIntervals.size());
	data.taps.reserve(nervIntervals.size());
	data.jumptaps.reserve(nervIntervals.size());
	data.handtaps.reserve(nervIntervals.size());
	data.chordtaps.reserve(nervIntervals.size());
	data.quads.reserve(nervIntervals.size());

	int seriously_not_js = 0;
	set<unsigned int> row_variations;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		row_variations.clear();
		// sequencing stuff
		bool last_was_definitely_not_jacks_maybe = false;
		int definitely_not_jacks = 0;
		int actual_jacks = 0;
		int actual_jacks_cj = 0;
		int not_js = 0;
		int not_hs = 0;
		int last_cols = 0;

		unsigned int taps = 0;
		unsigned int jumptaps = 0;
		unsigned int handtaps = 0;
		unsigned int chordtaps = 0;
		unsigned int quads = 0;
		unsigned int last_notes = 0;

		bool newinterval = true;
		if (dbg)
			for (int row : nervIntervals[i]) {
				if (dbg && newinterval)
					std::cout << "new interval: " << i
							  << " time: " << NoteInfo[row].rowTime
							  << std::endl;

				std::cout << NoteInfo[row].notes << std::endl;
				newinterval = false;
			}

		auto find_ranmen = [&data]() {

		};

		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes > 1) {
				chordtaps += notes;
			}
			if (notes == 2) {
				jumptaps += 2;
			} else if (notes == 3) {
				handtaps += 3;
			} else if (notes == 4) {
				++quads;
			}

			// we want mixed hs/js to register as hs, even at
			// relatively sparse hand density
			if (handtaps > 0)
				handtaps += jumptaps;

			// sequencing stuff
			unsigned int cols = NoteInfo[row].notes;
			row_variations.emplace(cols);

			if (dbg) {
				std::cout << "cols: " << cols << std::endl;
				std::cout << "last cols: " << last_cols << std::endl;
			}

			bool twas_jack = false;
			for (auto& id : col_ids) {
				if (dbg_lv2)
					std::cout << "cur id: " << id << std::endl;
				if (cols & id && last_cols & id) {
					if (dbg_lv2) {
						std::cout << "actual jack at: " << id << std::endl;
						std::cout << "with cols: " << cols
								  << " last cols: " << last_cols << std::endl;
					}
					++actual_jacks;
					twas_jack = true;
				}
			}
			if (twas_jack) {
				// [Comment moved from SetCJMod]
				// if we used the normal actual_jack for CJ too
				// we're saying something like "chordjacks" are
				// harder if they share more columns from chord to
				// chord" which is not true, it is in fact either
				// irrelevant or the inverse depending on the
				// scenario, this is merely to catch stuff like
				// splithand jumptrills registering as chordjacks
				// when they shouldn't be
				++actual_jacks_cj;
			}

			// [Comment moved from SetCJMod]
			// probably should be refactored/simplified, we want to
			// know if we have a bunch of stuff like [123]4[123]
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

			if (last_was_definitely_not_jacks_maybe) {
				// if there is no (mini)jack
				if (!(last_cols & cols)) {
					++definitely_not_jacks;
					if (dbg)
						std::cout << "definitely not jack: " << std::endl;
					// don't reset
					// last_was_definitely_not_jacks_maybe
				}
			}

			// only set for single notes
			if (notes == 1) {
				if (!(last_cols & cols)) {
					if (dbg)
						std::cout << "maybe not jack: " << std::endl;
					last_was_definitely_not_jacks_maybe = true;
				} else {
					last_was_definitely_not_jacks_maybe = false;
				}
			}

			// if prev row was chord and now it's a single tap, or
			// vice-versa
			if ((last_notes > 1 && notes == 1) ||
				(notes > 1 && last_notes == 1)) {
				if (!twas_jack) {
					if (dbg_lv2)
						std::cout << "good hot js/hs: " << std::endl;
					seriously_not_js -= 3;
				}
			}

			if (last_notes == 1 && notes == 1) {
				seriously_not_js = max(seriously_not_js, 0);
				++seriously_not_js;
				if (dbg_lv2)
					std::cout << "consecutive single note: " << seriously_not_js
							  << std::endl;

				// light js really stops at [12]321[23] kind of
				// density, anything below that should be picked up
				// by speed, and this stop rolls between jumps
				// getting floated up too high
				if (seriously_not_js > 3) {
					if (dbg)
						std::cout << "exceeding light js/hs tolerance: "
								  << seriously_not_js << std::endl;
					not_js += seriously_not_js;
					// give light hs the light js treatment
					not_hs += seriously_not_js;
				}
			} else if (last_notes > 1 && notes > 1) {
				// suppress jumptrilly garbage a little bit
				if (dbg)
					std::cout << "sequential chords detected: " << std::endl;
				not_hs += notes;
				not_js += notes;
			}

			last_notes = notes;
			last_cols = cols;
		}

		// keep care to match these to the struct fields
		data.num_row_variations.push_back(row_variations.size());
		data.definitely_not_jacks.push_back(definitely_not_jacks);
		data.actual_jacks.push_back(actual_jacks);
		data.actual_jacks_cj.push_back(actual_jacks_cj);
		data.not_js.push_back(not_js);
		data.not_hs.push_back(not_hs);
		data.taps.push_back(taps);
		data.jumptaps.push_back(jumptaps);
		data.handtaps.push_back(handtaps);
		data.chordtaps.push_back(chordtaps);
		data.quads.push_back(quads);
	}

	return data;
}

void
Calc::SetJumpMod(const JumpHandChordData& data, vector<float> doot[ModCount])
{
	const bool dbg = false && debugmode;
	doot[JS].resize(nervIntervals.size());
	doot[JSS].resize(nervIntervals.size());
	doot[JSJ].resize(nervIntervals.size());

	static const float min_mod = 0.6f;
	static const float max_mod = 1.1f;
	float last_mod = min_mod;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// nothing here
		if (data.taps[i] == 0) {
			doot[JS][i] = 1.f;
			doot[JSS][i] = 1.f;
			doot[JSJ][i] = 1.f;
		}
		// at least 1 tap but no jumps
		else if (data.jumptaps[i] == 0) {
			// use a decay in case this is something like js,
			// stream, js, stream js
			doot[JS][i] = CalcClamp(last_mod - 0.05f, min_mod, max_mod);
			doot[JSS][i] = 1.f;
			doot[JSJ][i] = 1.f;
		} else { // at least 1 jump
			// creepy banana
			float prop = static_cast<float>(data.jumptaps[i] + 1) /
						 static_cast<float>(data.taps[i] - 1) * 19.f / 7.f;

			// maybe the better solution would instead of
			// downscaling not js and jacks, just upscale js ??

			// punish lots splithand jumptrills
			float bromide =
			  CalcClamp(1.45f - (static_cast<float>(data.not_js[i]) /
								 static_cast<float>(data.taps[i])),
						0.85f,
						1.f);
			// downscale by jack density rather than upscale like cj
			// ok we can't be lazy about this or ippon manzone is js
			// (it's not)
			// theoretically the ohjump downscaler should handle
			// this but handling it here gives us more flixbility
			// with the ohjump mod
			float brop =
			  CalcClamp(1.35f - (static_cast<float>(data.actual_jacks[i]) /
								 static_cast<float>(data.taps[i])),
						0.5f,
						1.f);
			// clamp the original prop mod first before applying
			// above
			float zoot = CalcClamp(fastsqrt(prop), min_mod, max_mod);
			doot[JS][i] = CalcClamp(zoot * bromide * brop, min_mod, max_mod);
			doot[JSS][i] = bromide;
			doot[JSJ][i] = brop;
		}
		last_mod = doot[JS][i];
	}
	Smooth(doot[JS], 1.f);
}

void
Calc::SetHSMod(const JumpHandChordData& data, vector<float> doot[ModCount])
{
	const bool dbg = false && debugmode;
	doot[HS].resize(nervIntervals.size());
	doot[HSS].resize(nervIntervals.size());
	doot[HSJ].resize(nervIntervals.size());

	static const float min_mod = 0.6f;
	static const float max_mod = 1.1f;
	float last_mod = min_mod;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// nothing here
		if (data.taps[i] == 0) {
			doot[HS][i] = 1.f;
			doot[HSS][i] = 1.f;
			doot[HSJ][i] = 1.f;
		}
		// look ma no hands
		else if (data.handtaps[i] == 0) {
			// use a decay in case this is something like hs, js,
			// hs, js
			doot[HS][i] = CalcClamp(last_mod - 0.05f, min_mod, max_mod);
			doot[HSS][i] = 1.f;
			doot[HSJ][i] = 1.f;
		} else { // at least 1 hand
			// when bark of dog into canyon scream at you
			float prop =
			  0.4f + (static_cast<float>(data.handtaps[i] + 1) /
					  static_cast<float>(data.taps[i] - 1) * 32.f / 7.f);

			float bromide =
			  CalcClamp(1.45f - (static_cast<float>(data.not_hs[i]) /
								 static_cast<float>(data.taps[i])),
						0.89f,
						1.f);
			// downscale by jack density rather than upscale, like
			// cj
			float brop =
			  CalcClamp(1.35f - (static_cast<float>(data.actual_jacks[i]) /
								 static_cast<float>(data.taps[i])),
						0.5f,
						1.f);
			// clamp the original prop mod first before applying
			// above
			float zoot = CalcClamp(sqrt(prop), min_mod, max_mod);
			doot[HS][i] = CalcClamp(zoot * bromide * brop, min_mod, max_mod);
			doot[HSS][i] = bromide;
			doot[HSJ][i] = brop;
		}
		last_mod = doot[HS][i];
	}
	Smooth(doot[HS], 1.f);
}

// depress cj rating for non-cj stuff and boost cj rating for cj
// stuff
void
Calc::SetCJMod(const JumpHandChordData& data, vector<float> doot[ModCount])
{
	const bool dbg = false && debugmode;
	doot[CJ].resize(nervIntervals.size());
	doot[CJS].resize(nervIntervals.size());
	doot[CJJ].resize(nervIntervals.size());
	doot[CJQuad].resize(nervIntervals.size());

	static const float min_mod = 0.7f;
	static const float max_mod = 1.1f;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// nothing here
		if (data.taps[i] == 0) {
			doot[CJ][i] = 1.f;
			doot[CJS][i] = 1.f;
			doot[CJJ][i] = 1.f;
			doot[CJQuad][i] = 1.f;
		} else if (data.chordtaps[i] == 0) { // there are taps, but no chords
			doot[CJ][i] = min_mod;
			doot[CJS][i] = 1.f;
			doot[CJJ][i] = 1.f;
			doot[CJQuad][i] = 1.f;
		} else { // we have at least 1 chord
			// we want to give a little leeway for single taps but
			// not too much or sections of [12]4[123]   [123]4[23]
			// will be flagged as chordjack when they're really just
			// broken chordstream, and we also want to give enough
			// leeway so that hyperdense chordjacks at lower bpms
			// aren't automatically rated higher than more sparse
			// jacks at higher bpms
			float prop = static_cast<float>(data.chordtaps[i] + 1) /
						 static_cast<float>(data.taps[i] - 1) * 38.f / 7.f;
			float brop = CalcClamp(data.actual_jacks_cj[i] - 2.f, 0.625f, 1.f);

			float bruh_too_many_quads =
			  1.5f - (static_cast<float>(data.quads[i] * 4) /
					  static_cast<float>(data.taps[i]));
			bruh_too_many_quads = CalcClamp(bruh_too_many_quads, 0.88f, 1.f);

			// explicitly detect broken chordstream type stuff so we
			// can give more leeway to single note jacks
			float brop_two_return_of_brop_electric_bropaloo = CalcClamp(
			  1.2f - (static_cast<float>(data.definitely_not_jacks[i] * 2) /
					  static_cast<float>(data.taps[i])),
			  0.4f,
			  1.f);

			doot[CJ][i] = CalcClamp(
			  brop * brop_two_return_of_brop_electric_bropaloo * sqrt(prop),
			  min_mod,
			  max_mod);
			doot[CJS][i] = brop_two_return_of_brop_electric_bropaloo;
			doot[CJJ][i] = brop;
			doot[CJQuad][i] = bruh_too_many_quads;

			// ITS JUST VIBRO THEN
			if (data.num_row_variations[i] == 0 ||
				data.num_row_variations[i] < 3)
				doot[CJ][i] *= 0.85f;
			if (dbg) {
				std::cout << "quads: " << data.quads[i] << std::endl;
				std::cout << "taps: " << data.taps[i] << std::endl;
				std::cout << "bruh quads: " << bruh_too_many_quads << std::endl;
				std::cout << "actual jacks: " << data.actual_jacks_cj[i]
						  << std::endl;
				std::cout << "not jacks: " << data.definitely_not_jacks[i]
						  << std::endl;
				std::cout << "prop: " << prop << std::endl;
				std::cout << "brop: " << brop << std::endl;
				std::cout << "final mod: " << doot[CJ][i] << "\n" << std::endl;
				std::cout << "brop2: "
						  << brop_two_return_of_brop_electric_bropaloo
						  << std::endl;
			}
		}
	}
	Smooth(doot[CJ], 1.f);
	Smooth(doot[CJQuad], 1.f);
}

// since the calc skillset balance now operates on +- rather than
// just - and then normalization, we will use this to depress the
// stream rating for non-stream files. edit: ok technically this
// should be done in the sequential pass however that's getting so
// bloated and efficiency has been optimized enough we can just loop
// through noteinfo sequentially a few times and it's whatever

// the chaos mod is also determined here for the moment, which
// pushes up polys and stuff... idk how it even works myself tbh its
// a pretty hackjobjob
void
Calc::SetStreamMod(const vector<NoteInfo>& NoteInfo,
				   vector<float> doot[ModCount],
				   float music_rate)
{
	doot[StreamMod].resize(nervIntervals.size());
	int last_col = -1;

	float lasttime = -1.f;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		int actual_jacks = 0;
		unsigned int taps = 0;
		unsigned int singletaps = 0;
		set<float> whatwhat;
		vector<float> whatwhat2;
		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;

			if (notes == 1 && NoteInfo[row].notes == last_col)
				++actual_jacks;
			if (notes == 1) {
				++singletaps;
				last_col = NoteInfo[row].notes;
			}

			float curtime = NoteInfo[row].rowTime / music_rate;

			float giraffeasaurus = curtime - lasttime;
			// screen out large hits, it should be ok if this is a
			// discrete cutoff, but i don't like it if
			// (giraffeasaurus < 0.25f)
			//	whatwhat.emplace(giraffeasaurus);

			// instead of making another new mod, calculate the
			// original and most basic chaos mod and apply it along
			// with the new one for (size_t i = 0; i < notes; ++i)
			//	whatwhat2.push_back(giraffeasaurus);
			lasttime = curtime;
		}

		auto HE = [](float x) {
			static const int HE = 9;
			int this_is_a_counter = 0;
			vector<float> o(2 * (HE - 2) + 1);
			for (int i = 2; i < HE; ++i) {
				o[this_is_a_counter] = (1000.f / i * static_cast<float>(x));
				++this_is_a_counter;
			}
			o[this_is_a_counter] = 1000.f * static_cast<float>(x);
			++this_is_a_counter;
			for (int i = 2; i < HE; ++i) {
				o[this_is_a_counter] = (1000.f * i * static_cast<float>(x));
				++this_is_a_counter;
			}
			return o;
		};
		vector<vector<float>> hmmk;
		// for (auto& e : whatwhat)
		//	hmmk.emplace_back(HE(e));

		// I'M SURE THERE'S AN EASIER/FASTER WAY TO DO THIS
		float stub = 0.f;
		// using something else for chaos for now
		// compare each expanded sequence with every other
		if (false) {
			vector<float> mmbop;
			set<int> uniqshare;
			vector<float> biffs;
			vector<float> awwoo;
			for (size_t i = 0; i < hmmk.size() - 1; ++i) {
				float zop = 0.f;
				auto& a = hmmk[i];
				// compare element i against all others
				for (size_t j = i + 1; j < hmmk.size(); ++j) {
					auto& b = hmmk[j]; // i + 1 - last
					biffs.clear();
					for (size_t pP = 0; pP < a.size(); ++pP) {
						for (size_t vi = 0; vi < a.size(); ++vi) {
							float hi = 0.f;
							float lo = 0.f;
							if (a[pP] > b[vi]) {
								hi = a[pP];
								lo = b[vi];
							} else {
								lo = a[pP];
								hi = b[vi];
							}
							biffs.emplace_back(fastsqrt(hi / lo));
						}
					}

					// not exactly correct naming but basically if
					// hi/lo is close enough to 1 we can consider
					// the two points an intersection between the
					// respective quantization waves, the more
					// intersections we pick up and the closer they
					// are to 1 the more confident we are that what
					// we have are duplicate quantizations, and the
					// lower the final mod is
					int under1 = 0;
					float hair_scrunchy = 0.f;
					for (auto& lul : biffs) {
						if (lul < 1.05f) {
							++under1;
							// inverting; 1.05 values should produce
							// a lower mod than 1.0s and since we
							// are using this value as a divisor we
							// need to flip it around
							hair_scrunchy += 2.f - lul;
						}
					}
					awwoo.clear();
					for (auto& lul : biffs)
						awwoo.emplace_back(
						  1.f / static_cast<float>(hair_scrunchy + 1.f));
					uniqshare.insert(under1);
					// std::cout << "shared: " << under1 <<
					// std::endl;
				}
				zop = mean(awwoo);
				mmbop.push_back(zop);
				// std::cout << "zope: " << zop << std::endl;
			}
			stub = mean(mmbop);
			stub *= fastsqrt(static_cast<float>(uniqshare.size()));
			// std::cout << "mmbop: " << stub << std::endl;

			stub += 0.9f;
			float test_chaos_merge_stuff = sqrt(0.9f + cv(whatwhat2));
			test_chaos_merge_stuff =
			  CalcClamp(test_chaos_merge_stuff, 0.975f, 1.025f);
			stub =
			  CalcClamp(fastsqrt(stub) * test_chaos_merge_stuff, 0.955f, 1.04f);
			// std::cout << "uniq " << uniqshare.size() <<
			// std::endl;
		} else {
			// can't compare if there's only 1 ms value
			stub = 1.f;
		}

		// 1 tap is by definition a single tap
		if (taps < 2) {
			doot[StreamMod][i] = 1.f;
			// doot[Chaos][i] = stub;
		} else if (singletaps == 0) {
			doot[StreamMod][i] = 0.8f;
			// doot[Chaos][i] = stub;
		} else {
			// we're going to use this to downscale the stream
			// skillset of anything that isn't stream, just a simple
			// tap proportion for the moment but maybe if we need to
			// do fancier sequential stuff we can, the only real
			// concern are jack files registering as stream and that
			// shouldn't be an issue because the amount of single
			// taps required to do that to any effectual level would
			// be unplayable

			// we could also use this to push up stream files if we
			// wanted to but i don't think that's advisable or
			// necessary

			// we want very light js to register as stream,
			// something like jumps on every other 4th, so 17/19
			// ratio should return full points, but maybe we should
			// allow for some leeway in bad interval slicing this
			// maybe doesn't need to be so severe, on the other
			// hand, maybe it doesn'ting need to be not needing'nt
			// to be so severe
			float prop = static_cast<float>(singletaps + 1) /
						 static_cast<float>(taps - 1) * 10.f / 7.f;
			float creepy_pasta = CalcClamp(3.f - actual_jacks, 0.5f, 1.f);
			doot[StreamMod][i] =
			  CalcClamp(fastsqrt(prop * creepy_pasta), 0.8f, 1.0f);
			// doot[Chaos][i] = stub;
		}
	}
	for (auto& v : doot[Chaos])
		if (debugmode) {
			// std::cout << "butts: final " << v << std::endl;
		}
	if (SmoothPatterns) {
		Smooth(doot[StreamMod], 1.f);
		// Smooth(doot[Chaos], 1.f);
		// Smooth(doot[Chaos], 1.f);
	}
}

void
Calc::SetAnchorMod(const vector<NoteInfo>& NoteInfo,
				   unsigned int firstNote,
				   unsigned int secondNote,
				   vector<float> doot[ModCount])
{
	doot[Anchor].resize(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		int lcol = 0;
		int rcol = 0;
		for (int row : nervIntervals[i]) {
			if (NoteInfo[row].notes & firstNote)
				++lcol;
			if (NoteInfo[row].notes & secondNote)
				++rcol;
		}
		bool anyzero = lcol == 0 || rcol == 0;
		float bort = static_cast<float>(min(lcol, rcol)) /
					 static_cast<float>(max(lcol, rcol));
		bort = (0.3f + (1.f + (1.f / bort)) / 4.f);

		//
		bort = CalcClamp(bort, 0.9f, 1.1f);

		doot[Anchor][i] = anyzero ? 1.f : bort;

		fingerbias += (static_cast<float>(max(lcol, rcol)) + 2.f) /
					  (static_cast<float>(min(lcol, rcol)) + 1.f);
	}

	if (SmoothPatterns)
		Smooth(doot[Anchor], 1.f);
}

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
							   vector<float> doot[ModCount])
{
	doot[Roll].resize(nervIntervals.size());
	doot[OHJump].resize(nervIntervals.size());
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
				doot[OHJump][i] =
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
				doot[OHJump][i] = 1.f;
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
			doot[OHJump][i] = 1.f;
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

			doot[OHJump][i] = CalcClamp(0.1f + ohj, 0.5f, 1.f);

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

	if (SmoothPatterns) {
		Smooth(doot[Roll], 1.f);
		Smooth(doot[Roll], 1.f);
		Smooth(doot[OHTrill], 1.f);
		Smooth(doot[OHJump], 1.f);
		Smooth(doot[CJOHJump], 1.f);
	}
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

// ok new plan this takes a bunch of the concepts i tried with the
// old didn't-work-so-well-roll-downscaler and utilizes them across
// a wide range of intervals, making it more suited to picking up
// and hammering long stretches of jumptrillable roll patterns, this
// also apparently thinks every js and hs pattern in existence is
// mashable too, probably because they are
void
Calc::WideRangeRollScaler(const vector<NoteInfo>& NoteInfo,
						  unsigned int t1,
						  unsigned int t2,
						  float music_rate,
						  vector<float> doot[])
{
	doot[WideRangeRoll].resize(nervIntervals.size());

	static const float min_mod = 0.5f;
	static const float max_mod = 1.035f;
	unsigned int itv_window = 4;
	deque<vector<int>> itv_array;
	deque<vector<int>> itv_arrayTWO;
	deque<int> itv_taps;
	deque<int> itv_cv_taps;
	deque<int> itv_single_taps;
	vector<int> cur_vals;
	vector<int> window_vals;
	unordered_set<int> unique_vals;
	vector<int> filtered_vals;
	vector<int> lr;
	vector<int> rl;

	float lasttime = 0.f;
	int lastcol = -1;
	int lastsinglecol = -1;
	int single_taps = 0;

	// we could implement this like the roll scaler above, however
	// this is to prevent /0 errors exclusively at the moment
	// (because we are truncating extremely high bpm 192nd flams can
	// produce 0 as ms), causing potenial nan's in the sd function
	// we could look into using this for balance purposes as well
	// but things look ok for the moment and it would likely be a
	// large time sink without proper tests setup
	static const int ms_add = 1;

	// miss window seems like a reasonable cutoff, we don't want
	// 1500 ms hits after long breaks to poison the pool
	static const int max_ms_value = 180;
	static const float mean_cutoff_factor = 1.7f;

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		int interval_taps = 0;
		int ltaps = 0;
		int rtaps = 0;
		int single_taps = 0;

		// drop the oldest interval values if we have reached full
		// size
		if (itv_array.size() == itv_window) {
			itv_array.pop_front();
			itv_arrayTWO.pop_front();
			itv_cv_taps.pop_front();
			itv_taps.pop_front();
			itv_single_taps.pop_front();
		}

		// clear the current interval value vectors
		cur_vals.clear();
		lr.clear();
		rl.clear();

		// if (debugmode)
		//	std::cout << "new interval: " << i << std::endl;

		for (int row : nervIntervals[i]) {
			float curtime = NoteInfo[row].rowTime / music_rate;

			// we don't want slight bpm variations to pollute too
			// much stuff, especially if we are going to use unique
			// values at some point (we don't currently), if we do
			// and even single digit rounding becomes an issue we
			// can truncate further up
			int trunc_ms =
			  static_cast<int>((curtime - lasttime) * 1000.f) + ms_add;

			bool lcol = NoteInfo[row].notes & t1;
			bool rcol = NoteInfo[row].notes & t2;
			interval_taps += (static_cast<int>(lcol) + static_cast<int>(rcol));
			if (lcol ^ rcol)
				++single_taps;

			// if (debugmode)
			//	std::cout << "truncated ms value: " << trunc_ms <<
			// std::endl;

			// for expurrrmental thing for chaos thing
			if (trunc_ms < max_ms_value)
				cur_vals.push_back(trunc_ms - ms_add);

			if (!(lcol ^ rcol)) {
				if (!(lcol || rcol)) {
					//	if (debugmode)
					//		std::cout << "empty row" << std::endl;
					continue;
				}

				// if (debugmode)
				//	std::cout << "jump" << std::endl;

				if (lcol && rcol) {
					lastsinglecol = lastcol;
					lastcol = -1;
				}
				lasttime = curtime;
				continue;
			}

			int thiscol = lcol ? 0 : 1;
			if (thiscol != lastcol || lastcol == -1) {
				// handle ohjumps here, not above, because we only
				// want to handle the last ohjump before an actual
				// cross column, we don't want to handle long
				// sequences of ohjumps inside rolls. technically
				// they would be jumptrillable, but the point is to
				// pick up _rolls_, we handle ohjumps elsewhere
				// basically we treat ohjumps as they were either a
				// cross column left or right, so that the roll
				// detection still picks up rolls with ohjumps in
				// them
				if (lastcol == -1) {
					// dump an extra value, cuz
					if (rcol)
						if (trunc_ms < max_ms_value)
							lr.push_back(max_ms_value);
					if (lcol)
						if (trunc_ms < max_ms_value)
							rl.push_back(max_ms_value);

					// l vs r shouldn't matter here
					++ltaps;
					++rtaps;
				}

				if (rcol) {
					if (trunc_ms < max_ms_value)
						lr.push_back(trunc_ms);
					++ltaps;
				} else if (lcol) {
					++rtaps;
					if (trunc_ms < max_ms_value)
						rl.push_back(trunc_ms);
				}
				lasttime = curtime;
			} else {
				// the idea here was to help boost anchored patterns
				// but all it did was just buff rolls with frequent
				// directional swaps and anchored patterns are
				// already more or less fine possibly look into
				// taking this out of the lower level roll scaler as
				// well

				//	if (trunc_ms < trunc_ms)
				//		cur_vals.push_back(trunc_ms);
			}

			lastcol = thiscol;
		}

		itv_arrayTWO.push_back(cur_vals);

		int cv_taps = ltaps + rtaps;
		itv_taps.push_back(interval_taps);
		itv_cv_taps.push_back(cv_taps);
		itv_single_taps.push_back(single_taps);

		unsigned int window_taps = 0;
		for (auto& n : itv_taps)
			window_taps += n;

		unsigned int window_cv_taps = 0;
		for (auto& n : itv_cv_taps)
			window_cv_taps += n;

		unsigned int window_single_taps = 0;
		for (auto& n : itv_single_taps)
			window_single_taps += n;

		// push current interval values into deque, there should
		// always be space since we pop front at the start of the
		// loop if there isn't
		// k lets try just running the pass with the rolls (lower
		// mean indicates the rolls are flowing in that direction,
		// for patterns that aren't rolls this should be
		// functionally insignificant
		itv_array.push_back(mean(lr) < mean(rl) ? lr : rl);

		// clear vectors before using them
		window_vals.clear();
		unique_vals.clear();
		filtered_vals.clear();

		unsigned int totalvalues = 0;
		for (auto& v : itv_array)
			totalvalues += v.size();

		// the unique val is not really used at the moment,
		// basically we filter out the vectors for stuff way outside
		// the applicable range, like 500 ms hits that would
		// otherwise throw off the variation estimations unique val
		// is actually used for optimizing cases where you have
		// absolute pure roll formation and you know youwant to
		// slamjam it, there's probably a faster way to do the same
		// thing but w.e for now
		for (auto& v : itv_array)
			for (auto& n : v) {
				if (!unique_vals.count(n)) // 0.5% profiler
					unique_vals.insert(n); // 1% profiler
				window_vals.push_back(n);
			}
		float v_mean = mean(window_vals);
		for (auto& v : window_vals)
			if (v < mean_cutoff_factor * v_mean)
				filtered_vals.push_back(v);

		float f_mean = mean(filtered_vals);

		// bigger the lower the proportion of cross column single
		// taps i.e. more anchors this is kinda buggy atm and
		// producing sub 1 values for ??? reasons, but i don't think
		// it makes too much difference and i don't want to spend
		// more time on it right now
		float cv_prop = window_cv_taps == 0
						  ? 1
						  : static_cast<float>(window_taps) /
							  static_cast<float>(window_cv_taps);

		// bigger the lower the proportion of single taps to total
		// taps i.e. more chords
		float chord_prop = window_single_taps == 0
							 ? 1
							 : static_cast<float>(window_taps) /
								 static_cast<float>(window_single_taps);

		// handle anchors, chord filler, empty sections and single
		// notes
		if (cv_taps == 0 || single_taps == 0 || totalvalues < 1) {
			doot[WideRangeRoll][i] = 1.f;
			doot[Chaos][i] = 1.f;
			continue;
		}

		float pmod = min_mod;
		// these cases are likely a "true roll" and we can optimize
		// by just direct setting them, if unique_vals == 1 then we
		// only have a single instance of left->right or right->left
		// ms values across 2seconds, this usually indicates a
		// straight roll, if filtered vals == 1 the same thing
		// applies, but we filtered out the occasional direction
		// swap or a long break before the roll started. The
		// scenario in which this would not indicate a straight roll
		// is aggressively rolly js or hs, and while downscaling
		// those is a goal, we shouldn't attempt to do it here, so
		// we'll upscale the min mod by the anchor/chord proportion
		// the < is for catching empty stuff that may have reached
		// here edit: pretty sure this doesn't really effectively
		// push up js/hs but it's w.e, we don't use this mod on
		// those passes atm and it's more about the graphs looking
		// pretty ugly than anything else
		if (filtered_vals.size() <= 1 || unique_vals.size() <= 1) {
			doot[WideRangeRoll][i] =
			  CalcClamp(min_mod * chord_prop * cv_prop, min_mod, max_mod);
			doot[Chaos][i] = 1.f;
			continue;
		}

		// scale base default to cv_prop and chord_prop, we want the
		// base to be higher for filler sections that are mostly
		// slow anchors/js/whatever, if there's filler that's in
		// slow roll formation there's not much we can do about it
		// affecting adjacant hard intervals on the smooth pass,
		// however since we're running on a moving window the effect
		// is mitigated both chord_prop and cv_prop are 1.0+
		// multipliers it's kind of hard to have a roll if you only
		// have 12 notes in 2 seconds, ideally we would steer away
		// from discrete cutoffs of any kind but it's kind of hard
		// to resist the temptation as it should be pretty safe
		// here, even supposing we award a full modifier to
		// something that we shouldn't have, it should make no
		// practical difference apart from slightly upping the next
		// hard interval on smooth, yes this is redundant with the
		// above, but i figured it would be clearer to split up
		// slightly different cases
		if (window_taps <= 12) {
			doot[WideRangeRoll][i] = CalcClamp(
			  min_mod + (1.5f * chord_prop) + (0.5f * cv_prop), min_mod, 1.f);
			doot[Chaos][i] = 1.f;
			continue;
		}

		// we've handled basic cases and stuff that could cause /0
		// errors (hopefully) do maths now
		float unique_prop = static_cast<float>(unique_vals.size()) /
							static_cast<float>(window_vals.size());
		float cv_window = cv(window_vals);
		float cv_filtered = cv(filtered_vals);
		float cv_unique = cv(unique_vals);

		// this isn't really robust if theres a really short flam
		// involved anywhere e.g. a 5ms flam offset for an oh jump
		// when everything else is 50 ms will make the above way too
		// high, something else must be used, leaving this here so i
		// don't forget and try to use it again or some other idiot
		// float mean_prop = f_mean /
		// static_cast<float>(*std::min_element(filtered_vals.begin(),
		//												filtered_vals.end()));
		// mean_prop = 1.f;
		/*
		if (debugmode) {
			std::string rarp = "window vals: ";
			for (auto& a : filtered_vals) {
				rarp.append(std::to_string(a));
				rarp.append(", ");
			}
			std::cout << rarp << std::endl;
		}
		*/
		/*
		if (debugmode)
			std::cout << "cprop: " << cv_prop << std::endl;

		if (debugmode)
			std::cout << "cv: " << cv_window << cv_filtered <<
		cv_unique
					  << std::endl;
		if (debugmode)
			std::cout << "uprop: " << unique_prop << std::endl;

		if (debugmode)
			std::cout << "mean/min: " << mean_prop << std::endl;
			*/

		// if (debugmode)
		//	std::cout << "cv prop " << cv_prop << "\n" << std::endl;

		// basically the idea here is long sets of rolls if you only
		// count values from specifically left->right or
		// right->left, whichever lower, will have long sequences of
		// identical values that should become very exaggerated over
		// the course of 2.5 seconds compared to other files, since
		// the values will be identical mean/min should be much
		// closer to 1 compared to legitimate patterns and the
		// number of notes contained in the roll compared to total
		// notes (basically we don't count anchors as notes) will
		// also be closer to 1; we want to pretty much only detect
		// long cheesable sets of notes so multiplying window range
		// cv by mean/min and totaltaps/cvtaps should put almost
		// everything else over a 1.0 multiplier. perhaps mean/min
		// should be calculated on the full window like taps/cvtaps
		// are
		pmod = cv_filtered * cv_prop * 1.75f;
		pmod += 0.4f;
		// both too sensitive and unreliable i think
		// pmod += 1.25f * unique_prop;
		pmod = CalcClamp(pmod, min_mod, max_mod);

		doot[WideRangeRoll][i] = pmod;
		// if (debugmode)
		//	std::cout << "final mod " << doot[WideRangeRoll][i] <<
		//"\n"
		//			  << std::endl;

		// chayoss stuff
		window_vals.clear();
		filtered_vals.clear();
		for (auto& v : itv_arrayTWO)
			for (auto& n : v) {
				window_vals.push_back(n);
			}
		v_mean = mean(window_vals);
		for (auto& v : window_vals)
			if (v < mean_cutoff_factor * v_mean)
				filtered_vals.push_back(v);
		/*
		if (debugmode) {
			std::string rarp = "chaos vals: ";
			for (auto& a : filtered_vals) {
				rarp.append(std::to_string(a));
				rarp.append(", ");
			}
			std::cout << rarp << std::endl;
		}
		*/

		// attempt #437 at some kind of complex pattern picker upper
		float butt = 0.f;
		int whatwhat = 0;
		std::sort(filtered_vals.begin(), filtered_vals.end());
		if (filtered_vals.size() > 1) {
			for (auto& in : filtered_vals)
				for (auto& the : filtered_vals) {
					if (in == the) {
						butt += 1.f;
						++whatwhat;
						continue;
					}
					if (in > the) {
						float prop = static_cast<float>(in) / the;
						int mop = static_cast<int>(prop);
						float flop = prop - static_cast<float>(mop);
						if (flop == 0.f) {
							butt += 1.f;
							++whatwhat;
							continue;
						} else if (flop >= 0.5f) {
							flop = abs(flop - 1.f) + 1.f;
							butt += flop;

							// if (debugmode)
							//	std::cout << "flop over: " << flop
							//<< "
							// in: " <<
							// in
							//			  << " the: " << the << " mop: "
							//<<
							// mop
							//			  << std::endl;
						} else if (flop < 0.5f) {
							flop += 1.f;
							butt += flop;

							// if (debugmode)
							//	std::cout << "flop under: " << flop
							//<< "
							// in: "
							//<< in
							//			  << " the: " << the << " mop: "
							//<<
							// mop
							//			  << std::endl;
						}
						++whatwhat;
					}
				}
		} else {
			doot[Chaos][i] = 1.f;
		}
		if (whatwhat == 0)
			whatwhat = 1;
		doot[Chaos][i] =
		  CalcClamp(butt / static_cast<float>(whatwhat) - 0.075f, 0.98f, 1.04f);
	}

	// covering a window of 4 intervals does act as a smoother, and
	// a better one than a double smooth for sure, however we still
	// want to run the smoother anyway to rough out jagged edges and
	// interval splicing error
	if (SmoothPatterns)
		Smooth(doot[WideRangeRoll], 1.f);
	return;
}

// hyper explicit mega murder of long chains of
// 12211221122112211221122112211221
// look into consistent spacing checks
void
Calc::WideRangeJumptrillScaler(const vector<NoteInfo>& NoteInfo,
							   unsigned int t1,
							   unsigned int t2,
							   float music_rate,
							   vector<float> doot[])
{
	doot[WideRangeJumptrill].resize(nervIntervals.size());

	static const float min_mod = 0.25f;
	static const float max_mod = 1.f;
	unsigned int itv_window = 6;

	deque<int> itv_taps;
	deque<int> itv_ccacc;

	int lastcol = -1;
	int last_cc_dir = -1;
	int anchors_hit = 0;
	int crop_circles = 0;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// if (debugmode)
		//	std::cout << "new interval " << i << std::endl;

		int interval_taps = 0;
		int ccacc_counter = 0;

		// drop the oldest interval values if we have reached full
		// size
		if (itv_taps.size() == itv_window) {
			itv_taps.pop_front();
			itv_ccacc.pop_front();
		}

		for (int row : nervIntervals[i]) {
			bool lcol = NoteInfo[row].notes & t1;
			bool rcol = NoteInfo[row].notes & t2;
			interval_taps += (static_cast<int>(lcol) + static_cast<int>(rcol));

			if (!(lcol ^ rcol)) {
				if (!(lcol || rcol)) {
					continue;
				}
				// not sure yet how oh jumps interact with this
				if (lcol && rcol) {
					lastcol = -1;
				}
				continue;
			}

			int thiscol = lcol ? 0 : 1;
			if (thiscol != lastcol || lastcol == -1) {
				if (rcol) {
					if (anchors_hit == 1 && last_cc_dir == 10) {
						// if (debugmode)
						//	std::cout << "ccacc detected ending on "
						//<<
						// thiscol
						//			  << std::endl;
						++ccacc_counter;
					}

					anchors_hit = 0;
					last_cc_dir = 01;
				} else if (lcol) {
					if (anchors_hit == 1 && last_cc_dir == 01) {
						// if (debugmode)
						//	std::cout << "ccacc detected ending on "
						//<<
						// thiscol
						//			  << std::endl;
						++ccacc_counter;
					}

					last_cc_dir = 10;
					anchors_hit = 0;
				}
			} else {
				++anchors_hit;
				// if (debugmode)
				//	std::cout << "anchor hit " << std::endl;
			}

			lastcol = thiscol;
		}

		itv_taps.push_back(interval_taps);
		itv_ccacc.push_back(max(ccacc_counter - 1, 0));

		if (ccacc_counter > 0)
			++crop_circles;
		else
			--crop_circles;
		if (crop_circles < 0)
			crop_circles = 0;

		// if (debugmode)
		//	std::cout << "crop circles: " << crop_circles <<
		// std::endl;

		unsigned int window_taps = 0;
		for (auto& n : itv_taps)
			window_taps += n;

		unsigned int window_ccacc = 0;
		for (auto& n : itv_ccacc)
			window_ccacc += n;

		// if (debugmode)
		//	std::cout << "window taps: " << window_taps <<
		// std::endl;
		// if (debugmode)
		//	std::cout << "window ccacc: " << window_ccacc <<
		// std::endl;

		float pmod = 1.f;

		if (window_ccacc > 0 && crop_circles > 0)
			pmod =
			  static_cast<float>(window_taps) /
			  static_cast<float>(window_ccacc * (1 + max(crop_circles, 5)));

		doot[WideRangeJumptrill][i] = CalcClamp(pmod, min_mod, max_mod);
		// if (debugmode)
		//	std::cout << "final mod " << doot[OHTrill][i] << "\n" <<
		// std::endl;
	}

	if (SmoothPatterns)
		Smooth(doot[WideRangeJumptrill], 1.f);
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

	if (SmoothPatterns)
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

	if (SmoothPatterns)
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

	if (SmoothPatterns) {
		Smooth(doot[TheThing], 1.f);
		Smooth(doot[TheThing], 1.f);
	}
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
	if (SmoothPatterns)
		Smooth(doot[FlamJam], 1.f);
}

void
Calc::WideRangeRanMan(const vector<NoteInfo>& NoteInfo,
					  unsigned int t1,
					  unsigned int t2,
					  float music_rate,
					  vector<float> doot[])
{
	doot[RanMan].resize(nervIntervals.size());

	static const float min_mod = 0.25f;
	static const float max_mod = 1.f;
	unsigned int itv_window = 6;

	deque<int> itv_taps;
	deque<int> itv_ccacc;

	int lastcol = -1;
	int last_cc_dir = -1;
	int anchors_hit = 0;
	int crop_circles = 0;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// if (debugmode)
		//	std::cout << "new interval " << i << std::endl;

		int interval_taps = 0;
		int ccacc_counter = 0;

		// drop the oldest interval values if we have reached full
		// size
		if (itv_taps.size() == itv_window) {
			itv_taps.pop_front();
			itv_ccacc.pop_front();
		}

		for (int row : nervIntervals[i]) {
			bool lcol = NoteInfo[row].notes & t1;
			bool rcol = NoteInfo[row].notes & t2;
			interval_taps += (static_cast<int>(lcol) + static_cast<int>(rcol));

			if (!(lcol ^ rcol)) {
				if (!(lcol || rcol)) {
					continue;
				}
				// not sure yet how oh jumps interact with this
				if (lcol && rcol) {
					lastcol = -1;
				}
				continue;
			}

			int thiscol = lcol ? 0 : 1;
			if (thiscol != lastcol || lastcol == -1) {
				if (rcol) {
					if (anchors_hit == 1 && last_cc_dir == 10) {
						// if (debugmode)
						//	std::cout << "ccacc detected ending on "
						//<<
						// thiscol
						//			  << std::endl;
						++ccacc_counter;
					}

					anchors_hit = 0;
					last_cc_dir = 01;
				} else if (lcol) {
					if (anchors_hit == 1 && last_cc_dir == 01) {
						// if (debugmode)
						//	std::cout << "ccacc detected ending on "
						//<<
						// thiscol
						//			  << std::endl;
						++ccacc_counter;
					}

					last_cc_dir = 10;
					anchors_hit = 0;
				}
			} else {
				++anchors_hit;
				// if (debugmode)
				//	std::cout << "anchor hit " << std::endl;
			}

			lastcol = thiscol;
		}

		itv_taps.push_back(interval_taps);
		itv_ccacc.push_back(max(ccacc_counter - 1, 0));

		if (ccacc_counter > 0)
			++crop_circles;
		else
			--crop_circles;
		if (crop_circles < 0)
			crop_circles = 0;

		// if (debugmode)
		//	std::cout << "crop circles: " << crop_circles <<
		// std::endl;

		unsigned int window_taps = 0;
		for (auto& n : itv_taps)
			window_taps += n;

		unsigned int window_ccacc = 0;
		for (auto& n : itv_ccacc)
			window_ccacc += n;

		// if (debugmode)
		//	std::cout << "window taps: " << window_taps <<
		// std::endl;
		// if (debugmode)
		//	std::cout << "window ccacc: " << window_ccacc <<
		// std::endl;

		float pmod = 1.f;

		if (window_ccacc > 0 && crop_circles > 0)
			pmod =
			  static_cast<float>(window_taps) /
			  static_cast<float>(window_ccacc * (1 + max(crop_circles, 5)));

		doot[RanMan][i] = CalcClamp(pmod, min_mod, max_mod);
		// if (debugmode)
		//	std::cout << "final mod " << doot[OHTrill][i] << "\n" <<
		// std::endl;
	}

	if (SmoothPatterns)
		Smooth(doot[RanMan], 1.f);
	return;
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
	//SavePatternModParamXmlToDir();
}
#pragma endregion

int mina_calc_version = 330;
int
GetCalcVersion()
{
#ifdef USING_NEW_CALC
	return mina_calc_version;
#else
	return 263;
#endif
}
