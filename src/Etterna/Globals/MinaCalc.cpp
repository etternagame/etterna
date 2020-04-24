#pragma once
#include "MinaCalc.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric>

using std::max;
using std::min;
using std::pow;
using std::sqrt;
using std::vector;

template<typename T>
T
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

	return sqrt(sd / input.size()) / average;
}

inline float
downscale_low_accuracy_scores(float f, float sg)
{
	return sg >= 0.93f ? f : min(max(f - sqrt(0.93f - sg), 0.f), 100.f);
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

unsigned int
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

float
highest_difficulty(const DifficultyRating& difficulty)
{
	auto v = { difficulty.stream,	 difficulty.jumpstream,
			   difficulty.handstream, difficulty.stamina,
			   difficulty.jack,		  difficulty.chordjack,
			   difficulty.technical };
	return *std::max_element(v.begin(), v.end());
}

// DON'T WANT TO RECOMPILE HALF THE GAME IF I EDIT THE HEADER FILE
float finalscaler = 2.564f * 1.05f * 1.1f * 1.10f * 1.10f *
					1.025f; // multiplier to standardize baselines

// Stamina Model params
const float stam_ceil = 1.1f;	 // stamina multiplier max
const float stam_mag = 465.f;	 // multiplier generation scaler
const float stam_fscale = 2222.f; // how fast the floor rises (it's lava)
const float stam_prop =
  0.7f; // proportion of player difficulty at which stamina tax begins

// since we are no longer using the normalizer system we need to lower
// the base difficulty for each skillset and then detect pattern types
// to push down OR up, rather than just down and normalizing to a differential
// since chorded patterns have lower enps than streams, streams default to 1
// and chordstreams start lower
// stam is a special case and may use normalizers again
const float basescalers[NUM_SkillsetTWO] = { 0.f,   1.f, 0.9f, 0.925f,
											 0.95f, 1.f, 0.9f, 0.95f };

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

	vector<float> mcbloop(NUM_SkillsetTWO);
	// overall and stam will be left as 0.f by this loop
	for (int i = 0; i < NUM_SkillsetTWO; ++i)
		mcbloop[i] = Chisel(0.1f, 5.12f, score_goal, i, false);

	// stam is based on which calc produced the highest output without it
	size_t highest_base_skillset = std::distance(
	  mcbloop.begin(), std::max_element(mcbloop.begin(), mcbloop.end()));
	float base = mcbloop[highest_base_skillset];

	// rerun all with stam on, optimize by starting at the non-stam adjusted
	// base value for each skillset
	for (int i = 0; i < NUM_SkillsetTWO; ++i)
		mcbloop[i] = Chisel(mcbloop[i] - 1.28f, 0.64f, score_goal, i, true);

	// stam jams, stamina should push up the base ratings for files so files
	// that are more difficult by virtue of being twice as long for more or less
	// the same patterns don't get underrated, however they shouldn't be pushed
	// up a huge amount either, we want high stream scores to be equally
	// achieveable on longer or shorter files, ideally, the stam ratings itself
	// is a separate consideration and will be scaled to the degree to which the
	// stamina model affects the base rating

	// first - zoot the boot up the loot, we don't want to straddle below and
	// above 1 here
	float mcfroggerbopper =
	  pow(1.f + (mcbloop[highest_base_skillset] - base), 2) - 1.f;
	// all pow with the pow now
	float poodle_in_a_porta_potty = mcbloop[highest_base_skillset];

	// start with some% of the stam adjusted, scale the remaining to the pow'd
	// differential anything at or over 1 is reasonably intense but lets not get
	// it go too far how much stamina affects base ratings is handled in the
	// global stam params, this controls how much it takes for stamina to affect
	// the base rating, before it can positively affect the stamina rating,
	// basically we want to start increasing the stamina rating starting around
	// 5% bonus to base rating due to stamina, this gives us room to lower the
	// stamina rating for not-stamina files, preventing pollution of stamina
	// leaderboards with charts that are just very high rated but have no
	// stamina component
	mcfroggerbopper = CalcClamp(mcfroggerbopper, 0.f, 1.1f);
	mcbloop[Stamina] = 0.65f * poodle_in_a_porta_potty +
					   (mcfroggerbopper * 0.35f * poodle_in_a_porta_potty);

	// yes i know how dumb this looks
	DifficultyRating difficulty = { mcbloop[0], mcbloop[1], mcbloop[2],
									mcbloop[3], mcbloop[4], mcbloop[5],
									mcbloop[6], mcbloop[7] };
	vector<float> pumpkin = skillset_vector(difficulty);
	// sets the 'proper' debug output, doesn't (shouldn't) affect actual values
	// this is the only time debugoutput arg should be set to true
	if (debugmode)
		Chisel(mcbloop[highest_base_skillset] - 2.56f,
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
	return skillset_vector(difficulty);
}

float
Calc::JackLoss(const vector<float>& j, float x)
{
	float o = 0.f;
	for (size_t i = 0; i < j.size(); i++) {
		if (x < j[i])
			o += 7.f - (7.f * pow(x / (j[i] * 0.86f), 1.5f));
	}
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
	SetStreamMod(NoteInfo, left_hand.doot);
	right_hand.doot[HS] = left_hand.doot[HS];
	right_hand.doot[Jump] = left_hand.doot[Jump];
	right_hand.doot[CJ] = left_hand.doot[CJ];
	right_hand.doot[StreamMod] = left_hand.doot[StreamMod];

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

	j0 = SequenceJack(NoteInfo, 0, music_rate);
	j1 = SequenceJack(NoteInfo, 1, music_rate);
	j2 = SequenceJack(NoteInfo, 2, music_rate);
	j3 = SequenceJack(NoteInfo, 3, music_rate);
}

Finger
Calc::ProcessFinger(const vector<NoteInfo>& NoteInfo,
					unsigned int t,
					float music_rate)
{
	int Interval = 0;
	float last = -5.f;
	Finger AllIntervals(numitv, vector<float>());
	if (t == 0)
		nervIntervals = vector<vector<int>>(numitv, vector<int>());
	unsigned int column = 1u << t;

	for (size_t i = 0; i < NoteInfo.size(); i++) {
		float scaledtime = NoteInfo[i].rowTime / music_rate;

		while (scaledtime > static_cast<float>(Interval + 1) * IntervalSpan)
			++Interval;

		if (NoteInfo[i].notes & column) {
			AllIntervals[Interval].emplace_back(
			  CalcClamp(1000 * (scaledtime - last), 40.f, 5000.f));
			last = scaledtime;
		}

		if (t == 0 && NoteInfo[i].notes != 0)
			nervIntervals[Interval].emplace_back(i);
	}
	return AllIntervals;
}

void
Calc::TotalMaxPoints()
{
	for (size_t i = 0; i < left_hand.v_itvpoints.size(); i++)
		MaxPoints += static_cast<float>(left_hand.v_itvpoints[i] +
										right_hand.v_itvpoints[i]);
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
	for (int iter = 1; iter <= 7; iter++) {
		do {
			if (player_skill > 41.f)
				return player_skill;
			player_skill += resolution;
			if (ss == Overall || ss == Stamina)
				return 0.f; // not how we set these values

			// jack sequencer point loss for jack speed and (maybe?) cj
			if (ss == JackSpeed || ss == Chordjack || Technical)
				gotpoints = MaxPoints - JackLoss(j0, player_skill) -
							JackLoss(j1, player_skill) -
							JackLoss(j2, player_skill) -
							JackLoss(j3, player_skill);
			else
				// run standard calculator stuffies
				gotpoints = left_hand.CalcInternal(player_skill, ss, stamina) +
							right_hand.CalcInternal(player_skill, ss, stamina);
		} while (gotpoints / MaxPoints < score_goal);
		player_skill -= resolution;
		resolution /= 2.f;
	}

	// these are the values for msd/stam adjusted msd/pointloss the
	// latter two are dependent on player_skill and so should only
	// be recalculated with the final value already determined
	if (debugoutput) {
		left_hand.CalcInternal(player_skill, ss, stamina, debugoutput);
		right_hand.CalcInternal(player_skill, ss, stamina, debugoutput);
	}

	return player_skill + 2.f * resolution;
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

void
Hand::InitPoints(const Finger& f1, const Finger& f2)
{
	for (size_t ki_is_rising = 0; ki_is_rising < f1.size(); ++ki_is_rising)
		v_itvpoints.emplace_back(static_cast<int>(f1[ki_is_rising].size()) +
								 static_cast<int>(f2[ki_is_rising].size()));
}

void
Hand::StamAdjust(float x, vector<float>& diff, bool debug)
{
	float floor = 1.f; // stamina multiplier min (increases as chart advances)
	float mod = 1.f;   // mutliplier

	float avs1 = 0.f;
	float avs2 = 0.f;

	for (size_t i = 0; i < diff.size(); i++) {
		avs1 = avs2;
		avs2 = diff[i];
		float ebb = (avs1 + avs2) / 2.f;
		mod += ((ebb / (stam_prop * x)) - 1.f) / stam_mag;
		if (mod > 1.f)
			floor += (mod - 1.f) / stam_fscale;
		mod = CalcClamp(mod, floor, stam_ceil);
		stam_adj_diff[i] = diff[i] * mod;
		if (debug)
			debugValues[2][StamMod][i] = mod;
	}
}

// debug bool here is NOT the one in Calc, it is passed from chisel using the
// final difficulty as the starting point and should only be executed once per
// chisel
float
Hand::CalcInternal(float x, int ss, bool stam, bool debug)
{
	// vector<float> temppatternsmods;
	// we're going to recycle adj_diff for this part
	for (size_t i = 0; i < soap[BaseNPS].size(); ++i) {
		// only slightly less cancerous than before, this can/should be
		// refactored once the areas of redundancy are more clearly defined
		switch (ss) {
			case Overall: // should never be the case, handled up the stack
				break;
			case Stream: // vanilla, apply everything based on nps diff
				adj_diff[i] =
				  soap[BaseNPS][i] * doot[HS][i] * doot[Jump][i] * doot[CJ][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Jumpstream: // dont apply cj
				adj_diff[i] = soap[BaseNPS][i] * doot[HS][i] / doot[Jump][i] *
							  doot[StreamMod][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Handstream: // here cj counterbalances hs a bit, not good
				adj_diff[i] = soap[BaseNPS][i] / max(doot[HS][i], 0.925f) *
							  doot[Jump][i] * doot[StreamMod][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Stamina: // should never be the case, handled up the stack
				break;
			case JackSpeed: // use ms hybrid base
				adj_diff[i] = soap[BaseMSD][i] * doot[HS][i] * doot[Jump][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Chordjack: // use ms hybrid base
				adj_diff[i] =
				  soap[BaseMSD][i] / doot[CJ][i] * doot[StreamMod][i];
				adj_diff[i] *= basescalers[ss];
				break;
			case Technical: // use ms hybrid base
				adj_diff[i] =
				  soap[BaseMSD][i] * doot[HS][i] * doot[Jump][i] * doot[CJ][i];
				adj_diff[i] *= basescalers[ss];
				break;
		}

		// we always want to apply these mods, i think
		adj_diff[i] *= doot[Roll][i] * doot[OHJump][i] * doot[Anchor][i];
	}

	if (stam) {
		// not entirely happy how this is handled but stam is really a special
		// case
		for (auto& d : adj_diff)
			d *= basescalers[Stamina];
		StamAdjust(x, adj_diff);
	}

	// final difficulty values to use
	const vector<float>& v = stam ? stam_adj_diff : adj_diff;

	if (debug) {
		debugValues[2][StamMod].resize(v.size());
		debugValues[2][PtLoss].resize(v.size());
		// final debug output should always be with stam activated
		StamAdjust(x, adj_diff, true);
		debugValues[1][MSD] = stam_adj_diff;
	}

	float output = 0.f;
	for (size_t i = 0; i < v.size(); ++i) {
		float gainedpoints =
		  x > v[i] ? static_cast<float>(v_itvpoints[i])
				   : static_cast<float>(v_itvpoints[i]) * pow(x / v[i], 1.8f);

		output += gainedpoints;
		if (debug)
			debugValues[2][PtLoss][i] =
			  (static_cast<float>(v_itvpoints[i]) - gainedpoints);
	}

	return output;
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
						  ? sqrt(sqrt(1 - (static_cast<float>(jumps) /
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
		  CalcClamp(sqrt(sqrt(1.f - (static_cast<float>(chordtaps) /
									 static_cast<float>(taps) / 3.f))),
					0.5f,
					1.f);
	}
	if (SmoothPatterns)
		Smooth(doot[CJ], 1.f);
}

void
Calc::SetStreamMod(const vector<NoteInfo>& NoteInfo,
				   vector<float> doot[ModCount])
{
	doot[StreamMod].resize(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int singletaps = 0;

		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes == 1)
				singletaps += notes;
		}

		if (taps == 0 || singletaps == 0) {
			doot[StreamMod][i] = 1.f;
			continue;
		}

		doot[StreamMod][i] =
		  CalcClamp(sqrt(sqrt(1.f - (static_cast<float>(singletaps) /
									 static_cast<float>(taps) / 3.f))),
					0.5f,
					1.f);
	}
	if (SmoothPatterns)
		Smooth(doot[StreamMod], 1.f);
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

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		// roll downscaler stuff
		int totaltaps = 0;
		vector<float> lr;
		vector<float> rl;
		int ltaps = 0;
		int rtaps = 0;

		// ohj downscaler stuff
		int jumptaps = 0;		// more intuitive to count taps in jumps
		int maxseqjumptaps = 0; // basically the biggest sequence of ohj
		float ohj = 0.f;

		// BEWOOP
		vector<float> whatwhat;

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

				// dunno what im doing with this yet
				if (!lastcol == -1)
					whatwhat.push_back(curtime - lasttime);

				// yes we want to set this for jumps
				lasttime = curtime;

				// add jumptaps when hitting jumps for ohj
				if (lcol && rcol)
					jumptaps += 2;

				// set the largest ohj sequence
				maxseqjumptaps = max(maxseqjumptaps, jumptaps);

				continue;
			}
			// push to thing i dunno what im doing with yet
			whatwhat.push_back(curtime - lasttime);

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

		// something something push up polyrhythms???
		float butt = 0.f;
		std::sort(whatwhat.begin(), whatwhat.end());
		if (whatwhat.size() <= 1)
			butt = 1.f;
		else
			for (auto in : whatwhat)
				for (auto the : whatwhat)
					butt += static_cast<float>(static_cast<int>(in * 1000.f) %
							static_cast<int>(1000.f * the));

		if (!whatwhat.empty())
			butt /= static_cast<float>(whatwhat.size()) * 1000.f;

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
			doot[Chaos][i] = butt;
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
		doot[Roll][i] = CalcClamp(0.5f + sqrt(cv), 0.5f, 1.f);
		doot[OHTrill][i] = CalcClamp(0.5f + sqrt(cv), 0.8f, 1.f);
		doot[Chaos][i] = butt;

		// ohj stuff, wip
		if (jumptaps < 1 && maxseqjumptaps < 1)
			doot[OHJump][i] = 1.f;
		else {
			ohj = static_cast<float>(maxseqjumptaps + 1) /
				  static_cast<float>(totaltaps + 1);
			doot[OHJump][i] = CalcClamp(0.5f + sqrt(ohj), 0.5f, 1.f);
		}
	}

	if (SmoothPatterns) {
		Smooth(doot[Roll], 1.f);
		Smooth(doot[OHJump], 1.f);
	}

	return;
}

static const float ssrcap = 0.975f; // cap SSR at 96% so things don't get out of
									// hand YES WE ACTUALLY NEED THIS FFS
// Function to generate SSR rating
vector<float>
MinaSDCalc(const vector<NoteInfo>& NoteInfo, float musicrate, float goal)
{
	if (NoteInfo.size() <= 1) {
		return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	}
	return std::make_unique<Calc>()->CalcMain(
	  NoteInfo, musicrate, min(goal, ssrcap));
}

// Wrap difficulty calculation for all standard rates
MinaSD
MinaSDCalc(const vector<NoteInfo>& NoteInfo)
{
	MinaSD allrates;
	int lower_rate = 7;
	int upper_rate = 21;

	if (NoteInfo.size() > 1)
		for (int i = lower_rate; i < upper_rate; i++)
			allrates.emplace_back(
			  MinaSDCalc(NoteInfo, static_cast<float>(i) / 10.f, 0.93f));
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
	debugRun->CalcMain(NoteInfo, musicrate, min(goal, ssrcap));

	handInfo.emplace_back(debugRun->left_hand.debugValues);
	handInfo.emplace_back(debugRun->right_hand.debugValues);
}

int
GetCalcVersion()
{
	return 271;
}
