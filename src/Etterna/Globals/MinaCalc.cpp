#include "MinaCalc.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <memory>
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

	InitializeHands(NoteInfo, music_rate);
	TotalMaxPoints();
	float stream =
	  Chisel(0.1f, 10.24f, score_goal, false, false, true, false, false);
	float js =
	  Chisel(0.1f, 10.24f, score_goal, false, false, true, true, false);
	float hs =
	  Chisel(0.1f, 10.24f, score_goal, false, false, true, false, true);
	float tech =
	  Chisel(0.1f, 10.24f, score_goal, false, false, false, false, false);
	float jack =
	  Chisel(0.1f, 10.24f, score_goal, false, true, true, false, false);

	float techbase = max(stream, jack);
	tech = CalcClamp((tech / techbase) * tech, tech * 0.85f, tech);

	float stam;

	// stam is based which calc produced the highest output on base
	if (stream > tech || js > tech || hs > tech)
		if (stream > js && stream > hs)
			stam = Chisel(stream - 0.1f,
						  10.56f, score_goal, true, false, true, false, false);
		else if (js > hs)
			stam = Chisel(
			  js - 0.1f, 10.56f, score_goal, true, false, true, true, false);
		else
			stam = Chisel(
			  hs - 0.1f, 10.56f, score_goal, true, false, true, false, true);
	else
		stam = Chisel(
		  tech - 0.1f, 10.56f, score_goal, true, false, false, false, false);

	float chordjack = jack * 0.75f;

	DifficultyRating difficulty =
	  DifficultyRating{ 0.0,
						downscale_low_accuracy_scores(stream, score_goal),
						downscale_low_accuracy_scores(js, score_goal),
						downscale_low_accuracy_scores(hs, score_goal),
						downscale_low_accuracy_scores(stam, score_goal),
						downscale_low_accuracy_scores(jack, score_goal),
						downscale_low_accuracy_scores(chordjack, score_goal),
						downscale_low_accuracy_scores(tech, score_goal) };

	chordjack = difficulty.handstream;

	vector<float> pumpkin = skillset_vector(difficulty);
	// sets the 'proper' debug output, doesn't (shouldn't) affect actual values
	// this is the only time debugoutput arg should be set to true
	if (debugmode) {
		size_t idx = std::distance(
		  pumpkin.begin(), std::max_element(pumpkin.begin(), pumpkin.end()));
		float minval = *std::min_element(pumpkin.begin(), pumpkin.end());
		switch (idx) {
			case 1:
				Chisel(
					minval, 10.24f, score_goal, true, false, true, false, false, true);
				break;
			case 2:
				Chisel(
					minval, 10.24f, score_goal, true, false, true, true, false, true);
				break;
			case 3:
				Chisel(
					minval, 10.24f, score_goal, true, false, true, false, true, true);
				break;
			case 4:
				if (stream > tech || js > tech || hs > tech)
					if (stream > js && stream > hs)
						Chisel(stream - 0.1f, 10.56f, score_goal, true, false, true, false, false, true);
					else if (js > hs)
						Chisel(js - 0.1f, 10.56f, score_goal, true, false, true, true, false, true);
					else
						Chisel(hs - 0.1f, 10.56f, score_goal, true, false, true, false, true, true);
				else
					Chisel(tech - 0.1f, 10.56f, score_goal, true, false, false, false, false, true);
				break;
			case 5:
				Chisel(
					minval, 10.24f, score_goal, true, true, true, false, false, true);
				break;
			case 7:
				Chisel(
					minval, 10.24f, score_goal, true, false, false, false, false, true);
				break;
		}
	}


	// below are bandaids that the internal calc functions should handle prior
	// _some_ basic adjustment post-evalution may be warranted but this is way
	// too far, to the point where it becomes more difficult to make any positive
	// changes, though it also happens to be where almost all of the balance of 263

	// all relative scaling to specific skillsets should occur before this point, not
	// after (it ended up this way due to the normalizers which were dumb and removed)
	// stam is the only skillset that can/should be normalized to base values without
	// interfering with anything else (since it's not based on a type of pattern)

	// specific scaling to curve sub 93 scores down harder should take place last

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
			o += 7.f - (7.f * pow(x / (j[i] * 0.96f), 1.5f));
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

	if (debugmode) {
		left_hand.debugValues.resize(DebugCount);
		right_hand.debugValues.resize(DebugCount);
		for (size_t i = 0; i < DebugCount; ++i) {
			right_hand.debugValues[i].resize(numitv);
		}
	}

	ProcessedFingers fingers;
	for (int i = 0; i < 4; i++) {
		fingers.emplace_back(ProcessFinger(NoteInfo, i, music_rate));
	}

	// should probably structure this to avoid copypasta
	left_hand.InitDiff(fingers[0], fingers[1]);
	left_hand.InitPoints(fingers[0], fingers[1]);
	left_hand.ohjumpscale = OHJumpDownscaler(NoteInfo, 1, 2);
	left_hand.anchorscale = Anchorscaler(NoteInfo, 1, 2);
	left_hand.rollscale = RollDownscaler(NoteInfo, 1, 2, music_rate);
	left_hand.hsscale = HSDownscaler(NoteInfo);
	left_hand.jumpscale = JumpDownscaler(NoteInfo);

	right_hand.InitDiff(fingers[2], fingers[3]);
	right_hand.InitPoints(fingers[2], fingers[3]);
	right_hand.ohjumpscale = OHJumpDownscaler(NoteInfo, 4, 8);
	right_hand.anchorscale = Anchorscaler(NoteInfo, 4, 8);
	right_hand.rollscale = RollDownscaler(NoteInfo, 4, 8, music_rate);
	right_hand.hsscale = left_hand.hsscale;
	right_hand.jumpscale = left_hand.jumpscale;

	// these values never change during calc so set them immediately
	if (debugmode) {
		left_hand.debugValues[Jump] = left_hand.jumpscale;
		right_hand.debugValues[Jump] = left_hand.jumpscale;
		left_hand.debugValues[Anchor] = left_hand.anchorscale;
		right_hand.debugValues[Anchor] = right_hand.anchorscale;
		left_hand.debugValues[HS] = left_hand.hsscale;
		right_hand.debugValues[HS] = left_hand.hsscale;
		left_hand.debugValues[OHJump] = left_hand.ohjumpscale;
		right_hand.debugValues[OHJump] = right_hand.ohjumpscale;
		left_hand.debugValues[Roll] = left_hand.rollscale;
		right_hand.debugValues[Roll] = right_hand.rollscale;

		// basemsd is just adjusted ms, i think, maybe misnomer
		right_hand.debugValues[BaseNPS] = right_hand.v_itvNPSdiff;
		left_hand.debugValues[BaseNPS] = left_hand.v_itvNPSdiff;
		right_hand.debugValues[BaseMS] = right_hand.pureMSdiff;
		left_hand.debugValues[BaseMS] = left_hand.pureMSdiff;
		right_hand.debugValues[BaseMSD] = right_hand.v_itvMSdiff;
		left_hand.debugValues[BaseMSD] = left_hand.v_itvMSdiff;
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
			 bool stamina,
			 bool jack,
			 bool nps,
			 bool js,
			 bool hs, bool debugoutput)
{
	float gotpoints;
	for (int iter = 1; iter <= 7; iter++) {
		do {
			if (player_skill > 100.f)
				return player_skill;
			player_skill += resolution;
			gotpoints =
			  jack
				? MaxPoints - JackLoss(j0, player_skill) -
					JackLoss(j1, player_skill) - JackLoss(j2, player_skill) -
					JackLoss(j3, player_skill)
				   : left_hand.CalcInternal(	// don't do debug yet
					   player_skill, stamina, nps, js, hs, false) +
					right_hand.CalcInternal(player_skill, stamina, nps, js, hs, false);

		} while (gotpoints / MaxPoints < score_goal);
		player_skill -= resolution;
		resolution /= 2.f;
	}

	// these are the values for msd/stam adjusted msd/pointloss the
	// latter two are dependent on player_skill and so should only
	// be recalculated with the final value already determined
	if (debugoutput) {
		left_hand.CalcInternal(player_skill, stamina, nps, js, hs, debugoutput);
		right_hand.CalcInternal(player_skill, stamina, nps, js, hs, debugoutput);
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
	v_itvNPSdiff = vector<float>(f1.size());
	v_itvMSdiff = vector<float>(f1.size());
	pureMSdiff = vector<float>(f1.size());
	for (size_t i = 0; i < f1.size(); i++) {
		float nps = 1.6f * static_cast<float>(f1[i].size() + f2[i].size());
		float left_difficulty = CalcMSEstimate(f1[i]);
		float right_difficulty = CalcMSEstimate(f2[i]);
		float difficulty = max(left_difficulty, right_difficulty);
		v_itvNPSdiff[i] = finalscaler * nps;
		pureMSdiff[i] = finalscaler * difficulty;
		v_itvMSdiff[i] = finalscaler * (5.f * difficulty + 4.f * nps) / 9.f;
	}
	Smooth(v_itvNPSdiff, 0.f);
	if (SmoothDifficulty)
		DifficultyMSSmooth(v_itvMSdiff);
}

void
Hand::InitPoints(const Finger& f1, const Finger& f2)
{
	for (size_t i = 0; i < f1.size(); i++)
		v_itvpoints.emplace_back(static_cast<int>(f1[i].size()) +
								 static_cast<int>(f2[i].size()));
}

vector<float>
Hand::StamAdjust(float x, vector<float>& diff, bool debug)
{
	vector<float> o(diff.size());
	float floor = 1.f; // stamina multiplier min (increases as chart advances)
	float mod = 1.f;   // mutliplier

	float avs1 = 0.f;
	float avs2 = 0.f;

	for (size_t i = 0; i < diff.size(); i++) {
		avs1 = avs2;
		avs2 = diff[i];
		float ebb = (avs1 + avs2) / 2;
		mod += ((ebb / (prop * x)) - 1) / mag;
		if (mod > 1.f)
			floor += (mod - 1) / fscale;
		mod = CalcClamp(mod, floor, ceil);
		o[i] = diff[i] * mod;
		if (debug)
			o[i] = mod;
	}
	return o;
}

// debug bool here is NOT the one calc, it is passed from chisel using the final
// difficulty as the starting point and should only be executed once per chisel
float
Hand::CalcInternal(float x, bool stam, bool nps, bool js, bool hs, bool debug)
{
	vector<float> diff = nps ? v_itvNPSdiff : v_itvMSdiff;

	for (size_t i = 0; i < diff.size(); ++i) {
		diff[i] *=
		  hs
			? anchorscale[i] * sqrt(ohjumpscale[i]) * rollscale[i] *
				jumpscale[i]
			: (js ? hsscale[i] * hsscale[i] * anchorscale[i] *
					  sqrt(ohjumpscale[i]) * rollscale[i] * jumpscale[i]
				  : (nps
					   ? hsscale[i] * hsscale[i] * hsscale[i] * anchorscale[i] *
						   ohjumpscale[i] * ohjumpscale[i] * rollscale[i] *
						   jumpscale[i] * jumpscale[i]
					   : anchorscale[i] * sqrt(ohjumpscale[i]) * rollscale[i]));
	}

	const vector<float>& v = stam ? StamAdjust(x, diff, false) : diff;

	if (debug) {
		debugValues[MSD] = diff;	// pretty sure we need to copy
		// final debug output should always be with stam activated
		debugValues[StamMod] = StamAdjust(x, diff, true);
	}

	float output = 0.f;
	for (size_t i = 0; i < v.size(); i++) {
		float gainedpoints =
		  x > v[i] ? v_itvpoints[i] : v_itvpoints[i] * pow(x / v[i], 1.8f);

		output += gainedpoints;
		if (debug) {
			debugValues[PtLoss].resize(diff.size());
			debugValues[PtLoss][i] = (v_itvpoints[i] - gainedpoints);
		}
	}

	return output;
}

vector<float>
Calc::OHJumpDownscaler(const vector<NoteInfo>& NoteInfo,
					   unsigned int firstNote,
					   unsigned int secondNote)
{
	vector<float> output;

	for (const vector<int>& interval : nervIntervals) {
		int taps = 0;
		int jumptaps = 0;
		for (int row : interval) {
			int columns = 0;
			if (NoteInfo[row].notes & firstNote) {
				++columns;
				++taps;
			}
			if (NoteInfo[row].notes & secondNote) {
				++columns;
				++taps;
			}
			if (columns == 2)
				jumptaps += 2;
		}
		output.push_back(taps != 0 ? pow(1 - (static_cast<float>(jumptaps) /
											  static_cast<float>(taps) / 1.8f),
										 0.25f)
								   : 1.f);

		if (logpatterns)
			std::cout << "ohj " << output.back() << std::endl;
	}

	if (SmoothPatterns)
		Smooth(output, 1.f);
	return output;
}

vector<float>
Calc::Anchorscaler(const vector<NoteInfo>& NoteInfo,
				   unsigned int firstNote,
				   unsigned int secondNote)
{
	vector<float> output(nervIntervals.size());

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
		output[i] =
		  anyzero
			? 1.f
			: CalcClamp(sqrt(1 - (static_cast<float>(min(lcol, rcol)) /
								  static_cast<float>(max(lcol, rcol)) / 4.45f)),
						0.8f,
						1.05f);

		fingerbias += (static_cast<float>(max(lcol, rcol)) + 2.f) /
					  (static_cast<float>(min(lcol, rcol)) + 1.f);

		if (logpatterns)
			std::cout << "an " << output[i] << std::endl;
	}

	if (SmoothPatterns)
		Smooth(output, 1.f);
	return output;
}

vector<float>
Calc::HSDownscaler(const vector<NoteInfo>& NoteInfo)
{
	vector<float> output(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int handtaps = 0;
		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes == 3)
				handtaps++;
		}
		output[i] =
		  taps != 0
			? 1 - (static_cast<float>(handtaps) / static_cast<float>(taps))
			: 1.f;

		if (logpatterns)
			std::cout << "hs " << output[i] << std::endl;
	}

	if (SmoothPatterns)
		Smooth(output, 1.f);
	return output;
}

vector<float>
Calc::JumpDownscaler(const vector<NoteInfo>& NoteInfo)
{
	vector<float> output(nervIntervals.size());

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		unsigned int taps = 0;
		unsigned int jumps = 0;
		for (int row : nervIntervals[i]) {
			unsigned int notes = column_count(NoteInfo[row].notes);
			taps += notes;
			if (notes == 2)
				jumps++;
		}
		output[i] = taps != 0 ? sqrt(sqrt(1 - (static_cast<float>(jumps) /
											   static_cast<float>(taps) / 3.f)))
							  : 1.f;

		if (logpatterns)
			std::cout << "ju " << output[i] << std::endl;
	}
	if (SmoothPatterns)
		Smooth(output, 1.f);
	return output;
}

// downscales full rolls or rolly js, it looks explicitly for consistent cross column
// timings on both hands; consecutive notes on the same column will reduce the penalty
// 0.5-1 multiplier
vector<float>
Calc::RollDownscaler(const vector<NoteInfo>& NoteInfo,
					 unsigned int t1,
					 unsigned int t2,
					 float music_rate)
{
	vector<float> output(nervIntervals.size());

	// not sure if these should persist between intervals or not
	int lastcol = -1;
	float lasttime = 0.f;

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		int totaltaps = 0;
		vector<float> lr;
		vector<float> rl;
		int ltaps = 0;
		int rtaps = 0;
		int ohj = 0;	// quick hack to *roll* in sequential ohj downscaling

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

				if (lcol && rcol)
					ohj += 1;
				else
					ohj -= 1;
				continue;
			}
			ohj -= 1;

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
				// consecutive notes should "poison" the current cross column vector
				// but without shifting the proportional scaling too much
				// this is to avoid treating 121212212121 too much like 121212121212

				// if we wanted to be _super explicit_ we could just reset the lr/rl
				// vectors when hitting a consecutive note (and/or jump), there are
				// advantages to being hyper explicit but at the moment this does
				// sort of pick up rolly js quite well
				if (thiscol)
					rl.push_back(curtime - lasttime);
				else
					lr.push_back(curtime - lasttime);
				totaltaps += 2;	// yes this is cheezy
			}
			lastcol = thiscol;
		}
		int cvtaps = ltaps + rtaps;
		if (cvtaps == 0) {
			if (ohj > 0)	// temp sequential ohj penalty 
				output[i] = CalcClamp(1.f * totaltaps / (ohj * 3.3f), 0.5f, 1.f);
			else
				output[i] = 1.f;
			continue; // longjacks
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
		if (mean(lr) < mean(rl)) // right is higher
			cv = (2.f * cv + cvlr) / 3.f;
		else
			cv = (2.f * cv + cvrl) / 3.f;

		// then scaled against how many taps we ignored
		float barf = static_cast<float>(totaltaps) / static_cast<float>(cvtaps);
		cv *= barf;
		output[i] = CalcClamp(0.5f + sqrt(cv), 0.5f, 1.f);
	}

	if (SmoothPatterns)
		Smooth(output, 1.f);

	return output;
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
				vector<vector<vector<float>>>& handInfo)
{
	if (NoteInfo.size() <= 1)
		return;

	std::unique_ptr<Calc> debugRun = std::make_unique<Calc>();
	debugRun->debugmode = true;
	debugRun->CalcMain(NoteInfo, musicrate, min(goal, ssrcap));

	// Locate and modify the uses of left/right debug in the code
	handInfo.push_back(debugRun->left_hand.debugValues);
	handInfo.push_back(debugRun->right_hand.debugValues);
}

int
GetCalcVersion()
{
	return 266;
}
