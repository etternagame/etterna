#include "MinaCalc.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <xmmintrin.h>
#include <cstring>

using std::max;
using std::min;
using std::pow;
using std::sqrt;
using std::vector;

// Relies on endiannes (significantly inaccurate)
inline double
fastpow(double a, double b)
{
	int u[2];
	std::memcpy(&u, &a, sizeof a);
	u[1] = static_cast<int>(b * (u[1] - 1072632447) + 1072632447);
	u[0] = 0;
	std::memcpy(&a, &u, sizeof a);
	return a;
}

// reasonably accurate taylor approximation for ^ 1.8
inline float
fast_pw(float x)
{
	float xbar = x - 0.5f;
	return 0.287175f + 1.03383f * xbar + 0.827063f * xbar * xbar;
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

template<typename T>
inline T
CalcClamp(T x, T l, T h)
{
	return x > h ? h : (x < l ? l : x);
}

inline float
mean(const vector<float>& v)
{
	return std::accumulate(begin(v), end(v), 0.f) / v.size();
}

// Coefficient of variation
inline float
cv(const vector<float>& input)
{
	float sd = 0.f;
	float average = mean(input);
	for (float i : input)
		sd += (i - average) * (i - average);

	return fastsqrt(sd / input.size()) / average;
}

inline float
downscale_low_accuracy_scores(float f, float sg)
{
	return sg >= 0.93f ? f : min(max(f - fastsqrt(0.93f - sg), 0.f), 100.f);
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
		i = (f1 + f2 + f3) / 3;
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

// DON'T WANT TO RECOMPILE HALF THE GAME IF I EDIT THE HEADER FILE
static const float finalscaler = 2.564f * 1.05f * 1.1f * 1.10f * 1.10f *
								 1.025f * 0.925f; // multiplier to standardize baselines

// ***note*** if we want max control over stamina we need to have one model for
// affecting the other skillsets to a certain degree, enough to push up longer
// stream ratings into contention with shorter ones, and another for both a more
// granular and influential modifier to calculate the end stamina rating with
// so todo on that

// Stamina Model params
static const float stam_ceil = 1.091234f;  // stamina multiplier max
static const float stam_mag = 505.f;	 // multiplier generation scaler
static const float stam_fscale = 2000.f; // how fast the floor rises (it's lava)
static const float stam_prop =
  0.725f; // proportion of player difficulty at which stamina tax begins

// since we are no longer using the normalizer system we need to lower
// the base difficulty for each skillset and then detect pattern types
// to push down OR up, rather than just down and normalizing to a differential
// since chorded patterns have lower enps than streams, streams default to 1
// and chordstreams start lower
// stam is a special case and may use normalizers again
static const float basescalers[NUM_Skillset] = {
	0.f, 0.975f, 0.9f, 0.925f, 0.f, 0.8f, 0.8f, 0.95f
};

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

float
Calc::JackLoss(const vector<float>& j, float x)
{
	float o = 0.f;
	for (size_t i = 0; i < j.size(); i++)
		if (x < j[i])
			o += 7.f - (7.f * pow(x / (j[i] * 0.96f), 1.5f));
	CalcClamp(o, 0.f, 10000.f);
	return o;
}

JackSeq
Calc::SequenceJack(const vector<NoteInfo>& NoteInfo,
				   unsigned int t,
				   float music_rate)
{
	vector<float> output;
	float last = -5.f;
	float interval1;
	float interval2 = 0.f;
	float interval3 = 0.f;
	unsigned int track = 1u << t;

	for (auto i : NoteInfo) {
		if (i.notes & track) {
			float current_time = i.rowTime / music_rate;
			interval1 = interval2;
			interval2 = interval3;
			interval3 = 1000.f * (current_time - last);
			last = current_time;
			output.emplace_back(
			  min(2800.f / min((interval1 + interval2 + interval3) / 3.f,
							   interval3 * 1.4f),
				  50.f));
		}
	}
	return output;
}

Finger
Calc::ProcessFinger(const vector<NoteInfo>& NoteInfo,
					unsigned int t,
					float music_rate)
{
	// optimization, just allocate memory here once and recycle this vector
	vector<float> temp_queue(5000);
	vector<int> temp_queue_two(5000);
	unsigned int row_counter = 0;
	unsigned int row_counter_two = 0;

	int Interval = 0;
	float last = -5.f;
	Finger AllIntervals(numitv, vector<float>());
	if (t == 0)
		nervIntervals = vector<vector<int>>(numitv, vector<int>());
	unsigned int column = 1u << t;

	for (size_t i = 0; i < NoteInfo.size(); i++) {
		float scaledtime = NoteInfo[i].rowTime / music_rate;

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
	float grindscaler =
	  CalcClamp(
		0.93f + (0.07f * (NoteInfo.back().rowTime - 30.f) / 30.f), 0.93f, 1.f) *
	  CalcClamp(
		0.873f + (0.13f * (NoteInfo.back().rowTime - 15.f) / 15.f), 0.87f, 1.f);

	float shortstamdownscaler = CalcClamp(
	  0.9f + (0.1f * (NoteInfo.back().rowTime - 150.f) / 150.f), 0.9f, 1.f);

	// all this garbage should be handled by pattern mods, before chisel is run
	// ok maybe that isn't a hard and fast rule, but we should avoid it here
	/*
	float jprop = chord_proportion(NoteInfo, 2);
	float nojumpsdownscaler =
	  CalcClamp(0.8f + (0.2f * (jprop + 0.5f)), 0.8f, 1.f);
	float manyjumpsdownscaler = CalcClamp(1.43f - jprop, 0.85f, 1.f);

	float hprop = chord_proportion(NoteInfo, 3);
	float nohandsdownscaler =
	  CalcClamp(0.8f + (0.2f * (hprop + 0.65f)), 0.8f, 1.f);
	float allhandsdownscaler = CalcClamp(1.23f - hprop, 0.85f, 1.f);

	float qprop = chord_proportion(NoteInfo, 4);
	float lotquaddownscaler = CalcClamp(1.13f - qprop, 0.85f, 1.f);

	float jumpthrill = CalcClamp(1.625f - jprop - hprop, 0.85f, 1.f);
	*/

	InitializeHands(NoteInfo, music_rate);
	TotalMaxPoints();

	vector<float> mcbloop(NUM_Skillset);
	// overall and stam will be left as 0.f by this loop
	for (int i = 0; i < NUM_Skillset; ++i)
		mcbloop[i] = Chisel(0.1f, 5.12f, score_goal, i, false);

	// stam is based on which calc produced the highest output without it
	size_t highest_base_skillset = std::distance(
	  mcbloop.begin(), std::max_element(mcbloop.begin(), mcbloop.end()));
	float base = mcbloop[highest_base_skillset];

	// rerun all with stam on, optimize by starting at the non-stam adjusted
	// base value for each skillset
	for (int i = 0; i < NUM_Skillset; ++i)
		mcbloop[i] = Chisel(mcbloop[i] - 0.32f, 0.64f, score_goal, i, true);

	// stam jams, stamina should push up the base ratings for files so files
	// that are more difficult by virtue of being twice as long for more or less
	// the same patterns don't get underrated, however they shouldn't be pushed
	// up a huge amount either, we want high stream scores to be equally
	// achieveable on longer or shorter files, ideally, the stam ratings itself
	// is a separate consideration and will be scaled to the degree to which the
	// stamina model affects the base rating, so while stamina should affect the
	// base skillset ratings slightly we want the degree to which it makes files
	// harder to be catalogued as the stamina rating
	// scaling down stuff that has no stamina component will help preventing
	// pollution of stamina leaderboards with charts that are just very high
	// rated but take no stamina
	float poodle_in_a_porta_potty = mcbloop[highest_base_skillset];

	// ends up being a multiplier between ~0.8 and ~1
	// tuning is a wip
	float mcfroggerbopper =
	  pow((poodle_in_a_porta_potty / base) - 0.075f, 2.5f);

	// we wanted to shift the curve down a lot before pow'ing but it was too
	// much to balance out, so we need to give some back, this is roughly
	// equivalent of multiplying by 1.05 but also not really because math
	// we don't want to push up the high end stuff anymore so just add to
	// let stuff down the curve catch up a little
	// remember we're operating on a multiplier
	mcfroggerbopper = CalcClamp(mcfroggerbopper + 0.05f, 0.8f, 1.09f);
	mcbloop[Skill_Stamina] = poodle_in_a_porta_potty * mcfroggerbopper;

	// yes i know how dumb this looks
	DifficultyRating difficulty = { mcbloop[0], mcbloop[1], mcbloop[2],
									mcbloop[3], mcbloop[4], mcbloop[5],
									mcbloop[6], mcbloop[7] };
	vector<float> pumpkin = skillset_vector(difficulty);
	// sets the 'proper' debug output, doesn't (shouldn't) affect actual values
	// this is the only time debugoutput arg should be set to true
	if (debugmode)
		Chisel(mcbloop[highest_base_skillset] - 0.16f,
			   0.32f,
			   score_goal,
			   highest_base_skillset,
			   true,
			   true);

	// below are bandaids that the internal calc functions should handle prior
	// _some_ basic adjustment post-evalution may be warranted but this is way
	// too far, to the point where it becomes more difficult to make any
	// positive changes, though it also happens to be where almost all of the
	// balance of 263

	// all relative scaling to specific skillsets should occur before this
	// point, not after (it ended up this way due to the normalizers which were
	// dumb and removed) stam is the only skillset that can/should be normalized
	// to base values without interfering with anything else (since it's not
	// based on a type of pattern)

	// specific scaling to curve sub 93 scores down harder should take place
	// last

	/*
	difficulty.stream *=
	  allhandsdownscaler * manyjumpsdownscaler * lotquaddownscaler;
	difficulty.jumpstream *=
	  nojumpsdownscaler * allhandsdownscaler * lotquaddownscaler;
	difficulty.handstream *= nohandsdownscaler * allhandsdownscaler *
							 manyjumpsdownscaler * lotquaddownscaler;
	difficulty.stamina = CalcClamp(
	  difficulty.stamina * shortstamdownscaler * 0.985f * lotquaddownscaler,
	  1.f,
	  max(max(difficulty.stream, difficulty.jack),
		  max(difficulty.jumpstream, difficulty.handstream)) *
		1.1f);
	difficulty.technical *=
	  allhandsdownscaler * manyjumpsdownscaler * lotquaddownscaler * 1.01f;

	chordjack *= CalcClamp(qprop + hprop + jprop + 0.2f, 0.5f, 1.f) * 1.025f;

	bool downscale_chordjack_at_end = false;
	if (chordjack > difficulty.jack)
		difficulty.chordjack = chordjack;
	else
		downscale_chordjack_at_end = true;

	fingerbias /= static_cast<float>(2 * nervIntervals.size());
	float finger_bias_scaling = CalcClamp(3.55f - fingerbias, 0.85f, 1.f);
	difficulty.technical *= finger_bias_scaling;

	if (finger_bias_scaling <= 0.95f) {
		difficulty.jack *= 1.f + (1.f - sqrt(finger_bias_scaling));
	}

	float max_js_hs = max(difficulty.handstream, difficulty.jumpstream);
	if (difficulty.stream < max_js_hs)
		difficulty.stream -= sqrt(max_js_hs - difficulty.stream);

	vector<float> temp_vec = skillset_vector(difficulty);
	float overall = AggregateScores(temp_vec, 0.f, 10.24f);
	difficulty.overall = downscale_low_accuracy_scores(overall, score_goal);

	temp_vec = skillset_vector(difficulty);
	float aDvg = mean(temp_vec) * 1.2f;
	difficulty.overall = downscale_low_accuracy_scores(
	  min(difficulty.overall, aDvg) * grindscaler, score_goal);
	difficulty.stream = downscale_low_accuracy_scores(
	  min(difficulty.stream, aDvg * 1.0416f) * grindscaler, score_goal);
	difficulty.jumpstream =
	  downscale_low_accuracy_scores(
		min(difficulty.jumpstream, aDvg * 1.0416f) * grindscaler, score_goal) *
	  jumpthrill;
	difficulty.handstream =
	  downscale_low_accuracy_scores(
		min(difficulty.handstream, aDvg) * grindscaler, score_goal) *
	  jumpthrill;
	difficulty.stamina =
	  downscale_low_accuracy_scores(min(difficulty.stamina, aDvg) * grindscaler,
									score_goal) *
	  sqrt(jumpthrill) * 0.996f;
	difficulty.jack = downscale_low_accuracy_scores(
	  min(difficulty.jack, aDvg) * grindscaler, score_goal);
	difficulty.chordjack = downscale_low_accuracy_scores(
	  min(difficulty.chordjack, aDvg) * grindscaler, score_goal);
	difficulty.technical =
	  downscale_low_accuracy_scores(
		min(difficulty.technical, aDvg * 1.0416f) * grindscaler, score_goal) *
	  sqrt(jumpthrill);

	float highest = max(difficulty.overall, highest_difficulty(difficulty));

	vector<float> temp = skillset_vector(difficulty);
	difficulty.overall = AggregateScores(temp, 0.f, 10.24f);

	if (downscale_chordjack_at_end) {
		difficulty.chordjack *= 0.9f;
	}

	float dating = CalcClamp(0.5f + (highest / 100.f), 0.f, 0.9f);

	if (score_goal < dating) {
		difficulty = DifficultyRating{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	}

	if (highest == difficulty.technical) {
		difficulty.technical -= CalcClamp(
		  4.5f - difficulty.technical + difficulty.handstream, 0.f, 4.5f);
		difficulty.technical -= CalcClamp(
		  4.5f - difficulty.technical + difficulty.jumpstream, 0.f, 4.5f);
	}
	 */

	difficulty.overall = highest_difficulty(difficulty);

	// the final push down, cap ssrs (score specific ratings) to stop vibro
	// garbage and calc abuse from polluting leaderboards too much, a "true"
	// 38 is still unachieved so a cap of 40 [sic] is _extremely_ generous
	// do this for SCORES only, not cached file difficulties
	auto bye_vibro_maybe_yes_this_should_be_refactored_lul =
	  skillset_vector(difficulty);
	if (capssr) {
		static const float ssrcap = 40.f;
		
		for (auto& r : bye_vibro_maybe_yes_this_should_be_refactored_lul)
			r = CalcClamp(r, r, ssrcap);
	}
	
	return bye_vibro_maybe_yes_this_should_be_refactored_lul;
}

void
Calc::InitializeHands(const vector<NoteInfo>& NoteInfo, float music_rate)
{
	numitv = static_cast<int>(
	  std::ceil(NoteInfo.back().rowTime / (music_rate * IntervalSpan)));

	// these get changed/updated frequently so allocate them once at the start
	left_hand.adj_diff.resize(numitv);
	right_hand.adj_diff.resize(numitv);
	left_hand.stam_adj_diff.resize(numitv);
	right_hand.stam_adj_diff.resize(numitv);

	// at least for the moment there are a few mods we want to apply evenly
	// to all skillset, so pre-multiply them in these after they're generated
	left_hand.pre_multiplied_pattern_mod_group_a.resize(numitv);
	right_hand.pre_multiplied_pattern_mod_group_a.resize(numitv);

	ProcessedFingers fingers;
	for (int i = 0; i < 4; i++)
		fingers.emplace_back(ProcessFinger(NoteInfo, i, music_rate));

	// initialize base difficulty and point values
	left_hand.InitDiff(fingers[0], fingers[1]);
	left_hand.InitPoints(fingers[0], fingers[1]);
	right_hand.InitDiff(fingers[2], fingers[3]);
	right_hand.InitPoints(fingers[2], fingers[3]);

	// set pattern mods
	SetAnchorMod(NoteInfo, 1, 2, left_hand.doot);
	SetAnchorMod(NoteInfo, 4, 8, right_hand.doot);

	// roll and ohj
	SetSequentialDownscalers(NoteInfo, 1, 2, music_rate, left_hand.doot);
	SetSequentialDownscalers(NoteInfo, 4, 8, music_rate, right_hand.doot);

	// these are evaluated on all columns so right and left are the same
	// these also may be redundant with updated stuff
	SetHSMod(NoteInfo, left_hand.doot);
	SetJumpMod(NoteInfo, left_hand.doot);
	SetCJMod(NoteInfo, left_hand.doot);
	SetStreamMod(NoteInfo, left_hand.doot, music_rate);
	SetFlamJamMod(NoteInfo, left_hand.doot, music_rate);
	right_hand.doot[HS] = left_hand.doot[HS];
	right_hand.doot[Jump] = left_hand.doot[Jump];
	right_hand.doot[CJ] = left_hand.doot[CJ];
	right_hand.doot[StreamMod] = left_hand.doot[StreamMod];
	right_hand.doot[Chaos] = left_hand.doot[Chaos];
	right_hand.doot[FlamJam] = left_hand.doot[FlamJam];

	// pattern mods and base msd never change so set them immediately
	if (debugmode) {
		left_hand.debugValues.resize(3);
		right_hand.debugValues.resize(3);
		left_hand.debugValues[0].resize(ModCount);
		right_hand.debugValues[0].resize(ModCount);
		left_hand.debugValues[1].resize(NUM_CalcDiffValue);
		right_hand.debugValues[1].resize(NUM_CalcDiffValue);
		left_hand.debugValues[2].resize(NUM_CalcDebugMisc);
		right_hand.debugValues[2].resize(NUM_CalcDebugMisc);

		for (size_t i = 0; i < ModCount; ++i) {
			left_hand.debugValues[0][i] = left_hand.doot[i];
			right_hand.debugValues[0][i] = right_hand.doot[i];
		}

		// set everything but final adjusted output here
		for (size_t i = 0; i < NUM_CalcDiffValue - 1; ++i) {
			left_hand.debugValues[1][i] = left_hand.soap[i];
			right_hand.debugValues[1][i] = right_hand.soap[i];
		}
	}

	// it's probably time to loop over hands more sensibly or
	// do this stuff inside the class
	for (int i = 0; i < numitv; ++i) {
		left_hand.pre_multiplied_pattern_mod_group_a[i] =
		  left_hand.doot[Roll][i] * left_hand.doot[OHJump][i] *
		  left_hand.doot[Anchor][i];
		right_hand.pre_multiplied_pattern_mod_group_a[i] =
		  right_hand.doot[Roll][i] * right_hand.doot[OHJump][i] *
		  right_hand.doot[Anchor][i];
	}

	j0 = SequenceJack(NoteInfo, 0, music_rate);
	j1 = SequenceJack(NoteInfo, 1, music_rate);
	j2 = SequenceJack(NoteInfo, 2, music_rate);
	j3 = SequenceJack(NoteInfo, 3, music_rate);
}

float
Hand::CalcMSEstimate(vector<float>& input)
{
	if (input.empty())
		return 0.f;

	sort(input.begin(), input.end());
	float m = 0;
	input[0] *= 1.066f; // This is gross
	size_t End = min(input.size(), static_cast<size_t>(6));
	for (size_t i = 0; i < End; i++)
		m += input[i];
	return 1375.f * End / m;
}

void
Hand::InitDiff(Finger& f1, Finger& f2)
{
	for (size_t i = 0; i < NUM_CalcDiffValue - 1; ++i)
		soap[i].resize(f1.size());

	for (size_t i = 0; i < f1.size(); i++) {
		float nps = 1.6f * static_cast<float>(f1[i].size() + f2[i].size());
		float left_difficulty = CalcMSEstimate(f1[i]);
		float right_difficulty = CalcMSEstimate(f2[i]);
		float difficulty = max(left_difficulty, right_difficulty);
		soap[BaseNPS][i] = finalscaler * nps;
		soap[BaseMS][i] = finalscaler * difficulty;
		soap[BaseMSD][i] = finalscaler * (5.f * difficulty + 3.f * nps) / 8.f;
	}
	Smooth(soap[BaseNPS], 0.f);
	if (SmoothDifficulty)
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
	int possiblepoints = 0;
	float reqpoints = static_cast<float>(MaxPoints) * score_goal;
	for (int iter = 1; iter <= 8; iter++) {
		do {
			if (player_skill > 100.f)
				return player_skill;
			player_skill += resolution;
			if (ss == Skill_Overall || ss == Skill_Stamina)
				return 0.f; // not how we set these values

			// reset tallied score
			gotpoints = 0.f;
			possiblepoints = 0;
			/*
			// jack sequencer point loss for jack speed and (maybe?) cj
			if (ss == JackSpeed || ss == Chordjack || ss == Technical)
				gotpoints -=(JackLoss(j0, player_skill) -
							JackLoss(j1, player_skill) -
							JackLoss(j2, player_skill) -
							JackLoss(j3, player_skill)) / static_cast<float>(1.f
			+ static_cast<float>(ss == Technical)); if (ss == JackSpeed || ss ==
			Chordjack) gotpoints += MaxPoints * 0.1f;
			*/
			// run standard calculator stuffies
			left_hand.CalcInternal(gotpoints, player_skill, ss, stamina);
			right_hand.CalcInternal(gotpoints, player_skill, ss, stamina);
		} while (gotpoints < reqpoints);
		player_skill -= resolution;
		resolution /= 2.f;
	}

	// these are the values for msd/stam adjusted msd/pointloss the
	// latter two are dependent on player_skill and so should only
	// be recalculated with the final value already determined
	if (debugoutput) {
		left_hand.CalcInternal(
		  gotpoints, player_skill, ss, stamina, debugoutput);
		right_hand.CalcInternal(
		  gotpoints, player_skill, ss, stamina, debugoutput);
	}

	return player_skill + 2.f * resolution;
}

// debug bool here is NOT the one in Calc, it is passed from chisel using the
// final difficulty as the starting point and should only be executed once per
// chisel
void
Hand::CalcInternal(float& gotpoints,
				   float& x,
				   int ss,
				   bool stam,
				   bool debug)
{
	// vector<float> temppatternsmods;
	// we're going to recycle adj_diff for this part
	for (size_t i = 0; i < soap[BaseNPS].size(); ++i) {
		// only slightly less cancerous than before, this can/should be
		// refactored once the areas of redundancy are more clearly defined
		switch (ss) {
			case Skill_Overall: // should never be the case, handled up the
								// stack
				break;
			case Skill_Stream: // vanilla, apply everything based on nps diff
				adj_diff[i] = soap[BaseNPS][i] * doot[HS][i] * doot[Jump][i] *
							  doot[CJ][i] * doot[Chaos][i] * doot[FlamJam][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Skill_Jumpstream: // dont apply cj
				adj_diff[i] = soap[BaseNPS][i] * doot[HS][i] / doot[Jump][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Skill_Handstream: // here cj counterbalances hs a bit, not good
				adj_diff[i] =
				  soap[BaseNPS][i] / max(doot[HS][i], 0.925f) * doot[Jump][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Skill_Stamina: // should never be the case, handled up the
								// stack
				break;
			case Skill_JackSpeed: // use ms hybrid base
				adj_diff[i] = soap[BaseMSD][i] * doot[HS][i] * doot[Jump][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Skill_Chordjack: // use ms hybrid base
				adj_diff[i] = soap[BaseMSD][i] / doot[CJ][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Skill_Technical: // use ms hybrid base
				adj_diff[i] = soap[BaseMSD][i] * doot[HS][i] * doot[Jump][i] *
							  doot[CJ][i] * doot[Chaos][i];
				adj_diff[i] *= basescalers[ss];
				break;
		}

		// we always want to apply these mods, i think
		adj_diff[i] *= pre_multiplied_pattern_mod_group_a[i];
	}

	if (stam)
		StamAdjust(x, adj_diff);

	// final difficulty values to use
	const vector<float>& v = stam ? stam_adj_diff : adj_diff;

	// i don't like the copypasta either but the boolchecks where they were
	// were too slow
	if (debug) {
		debugValues[2][StamMod].resize(v.size());
		debugValues[2][PtLoss].resize(v.size());
		// final debug output should always be with stam activated
		StamAdjust(x, adj_diff, true);
		debugValues[1][MSD] = stam_adj_diff;

		for (size_t i = 0; i < v.size(); ++i) {
			float gainedpoints = x > v[i] ? static_cast<float>(v_itvpoints[i])
										  : static_cast<float>(v_itvpoints[i]) * fast_pw(x / v[i]);
			gotpoints += gainedpoints;
			debugValues[2][PtLoss][i] =
			  (static_cast<float>(v_itvpoints[i]) - gainedpoints);
		}
	} else
		for (size_t i = 0; i < v.size(); ++i)
			gotpoints += x > v[i] ? static_cast<float>(v_itvpoints[i])
								  : static_cast<float>(v_itvpoints[i]) *
									  fast_pw(x / v[i]);
}

void
Hand::StamAdjust(float x, vector<float>& diff, bool debug)
{
	float floor = 1.f; // stamina multiplier min (increases as chart advances)
	float mod = 1.f;   // mutliplier

	float avs1 = 0.f;
	float avs2 = 0.f;

	// i don't like the copypasta either but the boolchecks where they were
	// were too slow
	if (debug)
		for (size_t i = 0; i < diff.size(); i++) {
			avs1 = avs2;
			avs2 = diff[i];
			mod += ((((avs1 + avs2) / 2.f) / (stam_prop * x)) - 1.f) / stam_mag;
			if (mod > 1.f)
				floor += (mod - 1.f) / stam_fscale;
			mod = CalcClamp(mod, floor, stam_ceil);
			stam_adj_diff[i] = diff[i] * mod;
			debugValues[2][StamMod][i] = mod;
		}
	else
		for (size_t i = 0; i < diff.size(); i++) {
			avs1 = avs2;
			avs2 = diff[i];
			mod += ((((avs1 + avs2) / 2.f) / (stam_prop * x)) - 1.f) / stam_mag;
			if (mod > 1.f)
				floor += (mod - 1.f) / stam_fscale;
			mod = CalcClamp(mod, floor, stam_ceil);
			stam_adj_diff[i] = diff[i] * mod;
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

void
Calc::SetHSMod(const vector<NoteInfo>& NoteInfo, vector<float> doot[ModCount])
{
	doot[HS].resize(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int handtaps = 0;
		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes == 3)
				handtaps++;
		}
		doot[HS][i] =
		  taps != 0
			? 1 - pow((static_cast<float>(handtaps) / static_cast<float>(taps)),
					  1.5f)
			: 1.f;
	}

	if (SmoothPatterns)
		Smooth(doot[HS], 1.f);
}

void
Calc::SetJumpMod(const vector<NoteInfo>& NoteInfo, vector<float> doot[ModCount])
{
	doot[Jump].resize(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int jumps = 0;
		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes == 2)
				jumps++;
		}
		doot[Jump][i] = taps != 0
						  ? fastsqrt(fastsqrt(1 - (static_cast<float>(jumps) /
										   static_cast<float>(taps) / 3.f)))
						  : 1.f;
	}
	if (SmoothPatterns)
		Smooth(doot[Jump], 1.f);
}

// dunno what we're really doin here exactly
void
Calc::SetCJMod(const vector<NoteInfo>& NoteInfo, vector<float> doot[])
{
	doot[CJ].resize(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int chordtaps = 0;

		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes > 1)
				chordtaps += notes;
		}

		if (taps == 0 || chordtaps == 0) {
			doot[CJ][i] = 1.f;
			continue;
		}

		doot[CJ][i] =
		  CalcClamp(fastsqrt(fastsqrt(1.f - (static_cast<float>(chordtaps) /
									 static_cast<float>(taps) / 3.f))),
					0.5f,
					1.f);
	}
	if (SmoothPatterns)
		Smooth(doot[CJ], 1.f);
}

// try to sniff out chords that are built as flams. BADLY NEEDS REFACTOR
void
Calc::SetFlamJamMod(const vector<NoteInfo>& NoteInfo,
					vector<float> doot[],
					float& music_rate)
{
	doot[FlamJam].resize(nervIntervals.size());
	// scan for flam chords in this window
	float grouping_tolerance = 25.f;
	// tracks which columns were seen in the current flam chord
	// this is essentially the same as if NoteInfo[row].notes
	// was tracked over multiple rows
	int cols = 0;
	// all permutations of these values are unique identifiers
	int col_id[4] = { 1, 2, 4, 8 };
	// unused atm but we might want this information, allocate once
	vector<int> flam_rows(4);
	// timing points of the elements of the flam chord, allocate once
	vector<float> flamjam(4);
	// we don't actually need this counter since we can derive it from cols but
	// it might just be faster to track it locally since we will be recycling
	// the flamjam vector memory
	int flam_row_counter = 0;
	bool flamjamslamwham = false;

	// in each interval
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// build up flam detection for this interval
		vector<float> temp_mod;

		// row loop to pick up flams within the interval
		for (int row : nervIntervals[i]) {
			// perhaps we should start tracking this instead of tracking it over
			// and over....
			float scaled_time = NoteInfo[row].rowTime / music_rate * 1000.f;

			// this can be optimized a lot by properly mapping out the notes
			// value to arrow combinations (as it is constructed from them) and
			// deterministic

			// we are traversing intervals->rows->columns
			for (auto& id : col_id) {
				// check if there's a note here
				bool isnoteatcol = NoteInfo[row].notes & id;
				if (isnoteatcol) {
					// we're past the tolerance range, break if we have grouped
					// more than 1 note, or if we have filled an entire quad.
					// with this behavior if we fill a quad of 192nd flams with
					// order 1234 and there's still another note on 1 within the
					// tolerance range we'll flag this as a flam chord and
					// downscale appropriately, not sure if we want this as it
					// could be the case that there is a second flamchord
					// immediately after, and it's just vibro, or it could be
					// the case that there are complex reasonable patterns
					// following, perhaps a different behavior would be better

					// we cannot exceed tolerance without at least 1 note
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
					if ((tol_exceed && flam_row_counter > 1) || flam_row_counter == 4)
						// at least a flam jump has been detected, flag it
						flamjamslamwham = true;

					// if we have identified a flam chord in some way; handle and
					// reset, we don't want to skip the notes in this iteration
					// yes this should be done in the column loop since a flam
					// can start and end on any columns in any order

					// conditions to be here are at least 2 different columns
					// have been logged as part of a flam chord and we have
					// exceeded the tolerance for flam duration, or we have a
					// full quad flam detected, though not necessarily exceeding
					// the tolerance window. we do want to reset if it doesn't,
					// because we want to pick up vibro flams and nerf them into
					// oblivion too, i think
					if (flamjamslamwham) {
						// we'll construct the final pattern mod value from the
						// flammyness and number of detected flam chords
						float mod_part = 0.f;

						// lower means more cheesable means nerf harder
						float fc_dur =
						  flamjam[flam_row_counter - 1] - flamjam[0];

						// we don't want to affect explicit chords, but we have
						// to be sure that the entire flam we've picked up is an
						// actual chord and only an actual chord, if the first
						// and last elements detected were on the same row,
						// ignore it, trying to do fc_dur == 0.f didn't work
						// because of float precision
						if (flam_rows[0] != flam_rows[flam_row_counter - 1]) {
							// basic linear scale for testing purposes, scaled
							// to the window length and also flam size
							mod_part =
							  fc_dur / grouping_tolerance / flam_row_counter;
							temp_mod.push_back(mod_part);
						}

						// reset
						flam_row_counter = 0;
						cols = 0;
						flamjamslamwham = false;
					}

					// we know chord flams can't contain multiple notes of the
					// same column (those are just gluts), reset if detected
					// even within the tolerance range (we can't be outside of
					// it here by definition)
					if (cols & id) {
						flamjamslamwham = false;

						// reset
						flam_row_counter = 0;
						cols = 0;
					}

					// conditions to reach here are that a note in this column
					// has not been logged yet and we are still within the
					// grouping tolerance. we don't need cur/last times here,
					// the time of the first element will be used to determine
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

		// finishing the row loop leaves us with instances of flamjams
		// forgive a single instance of a chord flam for now; handle none
		if (temp_mod.size() < 2)
			doot[FlamJam][i] = 1.f;

		float wee = 0.f;
		for (auto& v : temp_mod)
			wee += v;

		// we can do this for now without worring about /0 since size is at
		// least 2 to get here
		wee /= static_cast<float>(temp_mod.size() - 1);

		wee = CalcClamp(1.f - wee, 0.5f, 1.f);
		doot[FlamJam][i] = wee;

		// reset the stuffs, _theoretically_ since we are sequencing we don't
		// even need at all to clear the flam detection however then we have
		// to handle cases like a single note in an interval and i don't feel
		// like doing that, a small number of flams that happen to straddle
		// the interval splice points shouldn't make a huge difference, but if
		// they do then we should deal with it
		temp_mod.clear();
		flam_row_counter = 0;
		cols = 0;
	}
	if (SmoothPatterns)
		Smooth(doot[FlamJam], 1.f);
}

// since the calc skillset balance now operates on +- rather than just - and
// then normalization, we will use this to depress the stream rating for
// non-stream files. edit: ok technically this should be done in the sequential
// pass however that's getting so bloated and efficiency has been optimized
// enough we can just loop through noteinfo sequentially a few times and it's
// whatever

// the chaos mod is also determined here for the moment, which pushes up polys
// and stuff... idk how it even works myself tbh its a pretty hackjobjob
void
Calc::SetStreamMod(const vector<NoteInfo>& NoteInfo,
				   vector<float> doot[ModCount],
				   float music_rate)
{
	doot[StreamMod].resize(nervIntervals.size());
	vector<float> giraffeasaurus(5000);
	float lasttime = -1.f;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int singletaps = 0;
		unsigned int boink = 0;
		vector<float> whatwhat;
		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes == 1)
				singletaps += notes;

			float curtime = NoteInfo[row].rowTime / music_rate;

			// asdlkfaskdjlf
			for (unsigned int pajamas = 0; pajamas < notes; ++pajamas)
				giraffeasaurus[boink + pajamas] = curtime - lasttime;
			boink += notes;
			lasttime = curtime;
		}

		whatwhat.resize(boink);
		for (unsigned int n = 0; n < boink; ++n)
			whatwhat[n] = giraffeasaurus[n];

		// something something push up polyrhythms???
		float butt = 0.f;
		std::sort(whatwhat.begin(), whatwhat.end(), [](float a, float b) {
			return a > b;
		});
		if (whatwhat.size() <= 1)
			butt = 1.f;
		else
			for (auto& in : whatwhat)
				for (auto& the : whatwhat)
					if (in >= the)
						if (in <= 3.f * the)
							if (the * 10000.f > 0.5f)
								butt += fastsqrt(fastsqrt(static_cast<float>(
								  static_cast<int>(in * 10000.f + 0.5f) %
								  static_cast<int>(10000.f * the + 0.5f))));

		if (!whatwhat.empty())
			butt /= static_cast<float>(whatwhat.size());
		butt = fastsqrt(butt) / 7.5f;

		butt = CalcClamp(butt + 0.8f, 0.95f, 1.1f);

		if (taps == 0 || singletaps == 0) {
			doot[StreamMod][i] = 1.f;
			doot[Chaos][i] = butt;
			continue;
		}

		doot[StreamMod][i] =
		  CalcClamp(fastsqrt(fastsqrt(1.f - (static_cast<float>(singletaps) /
									 static_cast<float>(taps) / 3.f))),
					0.5f,
					1.f);

		doot[Chaos][i] = butt;
	}
	if (SmoothPatterns) {
		Smooth(doot[StreamMod], 1.f);
		Smooth(doot[Chaos], 1.f);
		Smooth(doot[Chaos], 1.f);
	}
}

// downscales full rolls or rolly js, it looks explicitly for consistent cross
// column timings on both hands; consecutive notes on the same column will
// reduce the penalty 0.5-1 multiplier also now downscales ohj because we don't
// want to run this loop too often even if it makes code slightly clearer, i
// think, new ohj scaler is the same as the last one but gives higher weight to
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
	doot[OHTrill].resize(nervIntervals.size());
	doot[Chaos].resize(nervIntervals.size());

	// not sure if these should persist between intervals or not
	// not doing so makes the pattern detection slightly more strict
	// doing so will give the calc some context from the previous
	// interval but might have strange practical consequences
	// another major benefit of retaining last col from the previous
	// interval is we don't have to keep resetting it and i don't like
	// how that case is handled atm
	int lastcol = -1;
	float lasttime = 0.f;
	vector<float> lr;
	vector<float> rl;
	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// roll downscaler stuff
		int totaltaps = 0;
		lr.clear();
		rl.clear();
		int ltaps = 0;
		int rtaps = 0;

		// ohj downscaler stuff
		int jumptaps = 0;		// more intuitive to count taps in jumps
		int maxseqjumptaps = 0; // basically the biggest sequence of ohj
		float ohj = 0.f;

		for (int row : nervIntervals[i]) {
			bool lcol = NoteInfo[row].notes & t1;
			bool rcol = NoteInfo[row].notes & t2;
			totaltaps += (static_cast<int>(lcol) + static_cast<int>(rcol));
			float curtime = NoteInfo[row].rowTime / music_rate;

			// skip first element and ignore jumps/no taps
			if (!(lcol ^ rcol) || lastcol == -1) {

				// fully skip empty rows, set nothing
				if (!(lcol || rcol))
					continue;

				// it shouldn't matter if first row is a jump
				if (lastcol == -1)
					if (lcol)
						lastcol = 0;
					else
						lastcol = 1;

				// yes we want to set this for jumps
				lasttime = curtime;

				// add jumptaps when hitting jumps for ohj
				if (lcol && rcol)
					jumptaps += 2;

				// set the largest ohj sequence
				maxseqjumptaps = max(maxseqjumptaps, jumptaps);

				continue;
			}

			int thiscol = lcol < rcol;
			if (thiscol != lastcol) { // ignore consecutive notes
				if (thiscol) { // right column, push to right to left vector
					rl.push_back(curtime - lasttime);
					++rtaps;
				} else {
					lr.push_back(curtime - lasttime);
					++ltaps;
				}
				lasttime = curtime; // only log cross column lasttimes
			} else {
				// consecutive notes should "poison" the current cross column
				// vector but without shifting the proportional scaling too much
				// this is to avoid treating 121212212121 too much like
				// 121212121212

				// if we wanted to be _super explicit_ we could just reset the
				// lr/rl vectors when hitting a consecutive note (and/or jump),
				// there are advantages to being hyper explicit but at the
				// moment this does sort of pick up rolly js quite well, though
				// it would probably be more responsible longterm to have an
				// explicit roll detector an explicit trill detector, and an
				// explicit rolly js detector thing is debugging all 3 and
				// making sure they work as intended and in exclusion is just as
				// hard as making a couple of more generic mods and accepting
				// they will overlap in certain scenarios though again on the
				// other hand explicit modifiers are easier to tune you just
				// have to do a lot more of it
				if (thiscol)
					rl.push_back(curtime - lasttime);
				else
					lr.push_back(curtime - lasttime);
				totaltaps += 2; // yes this is cheezy
			}

			// ohj downscaler stuff
			// we know between the following that the latter is more difficult
			// [12][12][12]222[12][12][12]
			// [12][12][12]212[12][12][12]
			// so we want to penalize not only any break in the ohj sequence
			// but further penalize breaks which contain cross column taps
			// this should also reflect the difference between [12]122[12],
			// [12]121[12] cases like 121[12][12]212[12][12]121 should probably
			// have some penalty but likely won't with this setup, but everyone
			// hates that anyway and it would be quite difficult to force the
			// current setup to do so without increasing complexity
			// significantly (probably)
			jumptaps -=
			  1; // we're already on single notes, so just decrement a lil
			if (thiscol != lastcol) // easier to read if we do it again
				jumptaps -= 2;

			lastcol = thiscol;
		}

		int cvtaps = ltaps + rtaps;

		// if this is true we have some combination of single notes and
		// jumps where the single notes are all on the same column
		if (cvtaps == 0) {
			// we don't want to treat 2[12][12][12]2222
			// 2222[12][12][12]2 differently, so use the
			// max sequence here exclusively
			if (maxseqjumptaps > 0)
				doot[OHJump][i] =
				  CalcClamp(1.f * static_cast<float>(totaltaps) /
							  (static_cast<float>(maxseqjumptaps) * 2.5f),
							0.5f,
							1.f);
			else // single note longjacks, do nothing
				doot[OHJump][i] = 1.f;

			// no rolls here by definition
			doot[Roll][i] = 1.f;
			doot[OHTrill][i] = 1.f;

			continue;
		}

		float cvlr = 0.2f;
		float cvrl = 0.2f;
		if (ltaps > 1)
			cvlr = cv(lr);
		if (rtaps > 1)
			cvrl = cv(rl);

		// weighted average, but if one is empty we want it to skew high not low
		// due to * 0
		float cv = ((cvlr * (ltaps + 1)) + (cvrl * (rtaps + 1))) /
				   static_cast<float>(cvtaps + 2);

		// the vector with the lower mean should carry a little more weight
		float mlr = mean(lr);
		float mrl = mean(rl);
		bool rl_is_higher = mlr < mrl;
		cv = rl_is_higher ? (2.f * cv + cvlr) / 3.f : (2.f * cv + cvrl) / 3.f;

		// if we want to screen out trills and focus on just rolls we can
		// compare mean values, proper rolls will have one set with a mean 3x
		// above the other trills will be 1:1 equal, this is a simple linear
		// downscale for test purposes and i dont if it has unintended
		// consequences, but it does work for saw-saw
		float yes_trills = 0.f;
		if (1) {
			float notrills = 1.f;
			if (mlr > 0.f && mrl > 0.f) {
				float div = rl_is_higher ? mrl / mlr : mlr / mrl;
				div = CalcClamp(div, 1.f, 3.f);
				notrills = CalcClamp(2.f - div, 0.f, 1.f);
			}
			yes_trills = cv - notrills; // store high oh trill detection in case
										// we want to do stuff with it later
			cv += notrills * 1.f;		// just straight up add to cv
		}

		// then scaled against how many taps we ignored
		float barf = static_cast<float>(totaltaps) / static_cast<float>(cvtaps);
		cv *= barf;
		yes_trills *= barf;

		// we just want a minimum amount of variation to escape getting
		// downscaled cap to 1 (it's not an inherently bad idea to upscale sets
		// of patterns with high variation but we shouldn't do that here,
		// probably)
		doot[Roll][i] = CalcClamp(0.5f + fastsqrt(cv), 0.5f, 1.f);
		doot[OHTrill][i] = CalcClamp(0.5f + fastsqrt(cv), 0.8f, 1.f);

		// ohj stuff, wip
		if (jumptaps < 1 && maxseqjumptaps < 1)
			doot[OHJump][i] = 1.f;
		else {
			ohj = static_cast<float>(maxseqjumptaps + 1) /
				  static_cast<float>(totaltaps + 1);
			doot[OHJump][i] = CalcClamp(0.5f + fastsqrt(ohj), 0.5f, 1.f);
		}
	}

	if (SmoothPatterns) {
		Smooth(doot[Roll], 1.f);
		Smooth(doot[OHJump], 1.f);
	}

	return;
}

static const float ssr_goal_cap = 0.965f; // goal cap to prevent insane scaling

// Function to generate SSR rating
vector<float>
MinaSDCalc(const vector<NoteInfo>& NoteInfo, float musicrate, float goal)
{
	if (NoteInfo.size() <= 1)
		return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
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
		cacheRun->capssr = false;
		for (int i = lower_rate; i < upper_rate; i++) {
			allrates.emplace_back(cacheRun->CalcMain(
			  NoteInfo, static_cast<float>(i) / 10.f, 0.93f));
		}
	}
	
	else {
		vector<float> output{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
		for (int i = lower_rate; i < upper_rate; i++)
			allrates.emplace_back(output);
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
	if (NoteInfo.size() <= 1)
		return;

	std::unique_ptr<Calc> debugRun = std::make_unique<Calc>();
	debugRun->debugmode = true;
	debugRun->CalcMain(NoteInfo, musicrate, min(goal, ssr_goal_cap));

	handInfo.emplace_back(debugRun->left_hand.debugValues);
	handInfo.emplace_back(debugRun->right_hand.debugValues);
}

int
GetCalcVersion()
{
	return 273;
}
