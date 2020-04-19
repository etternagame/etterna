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
	if (stream > tech || js > tech || hs > tech)
		if (stream > js && stream > hs)
			stam = Chisel(stream - 0.1f,
						  2.56f,
						  score_goal,
						  true,
						  false,
						  true,
						  false,
						  false);
		else if (js > hs)
			stam = Chisel(
			  js - 0.1f, 2.56f, score_goal, true, false, true, true, false);
		else
			stam = Chisel(
			  hs - 0.1f, 2.56f, score_goal, true, false, true, false, true);
	else
		stam = Chisel(
		  tech - 0.1f, 2.56f, score_goal, true, false, false, false, false);

	js *= 0.95f;
	hs *= 0.975f;
	stam *= 0.935f;

	float chordjack = jack * 0.75f;
	tech *= 0.95f;

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

	difficulty.stream *=
	  allhandsdownscaler * manyjumpsdownscaler * lotquaddownscaler;
	difficulty.jumpstream *=
	  nojumpsdownscaler * allhandsdownscaler * lotquaddownscaler;
	difficulty.handstream *= nohandsdownscaler * allhandsdownscaler * 1.015f *
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

	difficulty.jack *= 0.925f;

	if (highest == difficulty.technical) {
		difficulty.technical -= CalcClamp(
		  4.5f - difficulty.technical + difficulty.handstream, 0.f, 4.5f);
		difficulty.technical -= CalcClamp(
		  4.5f - difficulty.technical + difficulty.jumpstream, 0.f, 4.5f);
	}

	difficulty.technical *= 1.025f;
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

	ProcessedFingers fingers;
	for (int i = 0; i < 4; i++) {
		fingers.emplace_back(ProcessFinger(NoteInfo, i, music_rate));
	}

	left_hand.InitDiff(fingers[0], fingers[1]);
	left_hand.InitPoints(fingers[0], fingers[1]);
	left_hand.ohjumpscale = OHJumpDownscaler(NoteInfo, 1, 2);
	left_hand.anchorscale = Anchorscaler(NoteInfo, 1, 2);
	left_hand.rollscale = RollDownscaler(fingers[0], fingers[1]);
	left_hand.hsscale = HSDownscaler(NoteInfo);
	left_hand.jumpscale = JumpDownscaler(NoteInfo);

	right_hand.InitDiff(fingers[2], fingers[3]);
	right_hand.InitPoints(fingers[2], fingers[3]);
	right_hand.ohjumpscale = OHJumpDownscaler(NoteInfo, 4, 8);
	right_hand.anchorscale = Anchorscaler(NoteInfo, 4, 8);
	right_hand.rollscale = RollDownscaler(fingers[2], fingers[3]);
	right_hand.hsscale = left_hand.hsscale;
	right_hand.jumpscale = left_hand.jumpscale;

	switch (debugMod) {
		case CalcPatternMod::Jump:
			left_hand.debug = left_hand.jumpscale;
			right_hand.debug = right_hand.jumpscale;
			break;
		case CalcPatternMod::Anchor:
			left_hand.debug = left_hand.anchorscale;
			right_hand.debug = right_hand.anchorscale;
			break;
		case CalcPatternMod::HS:
			left_hand.debug = left_hand.hsscale;
			right_hand.debug = right_hand.hsscale;
			break;
		case CalcPatternMod::OHJump:
			left_hand.debug = left_hand.ohjumpscale;
			right_hand.debug = right_hand.ohjumpscale;
			break;
		case CalcPatternMod::Roll:
			left_hand.debug = left_hand.rollscale;
			right_hand.debug = right_hand.rollscale;
			break;
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

float
Calc::Chisel(float player_skill,
			 float resolution,
			 float score_goal,
			 bool stamina,
			 bool jack,
			 bool nps,
			 bool js,
			 bool hs)
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
				: left_hand.CalcInternal(player_skill, stamina, nps, js, hs) +
					right_hand.CalcInternal(player_skill, stamina, nps, js, hs);

		} while (gotpoints / MaxPoints < score_goal);
		player_skill -= resolution;
		resolution /= 2.f;
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

	for (size_t i = 0; i < f1.size(); i++) {
		float nps = 1.6f * static_cast<float>(f1[i].size() + f2[i].size());
		float left_difficulty = CalcMSEstimate(f1[i]);
		float right_difficulty = CalcMSEstimate(f2[i]);
		float difficulty = max(left_difficulty, right_difficulty);
		v_itvNPSdiff[i] = finalscaler * nps;
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
Hand::StamAdjust(float x, vector<float>& diff)
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
		CalcClamp(mod, floor, ceil);
		o[i] = diff[i] * mod;
	}
	return o;
}

float
Hand::CalcInternal(float x, bool stam, bool nps, bool js, bool hs)
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

	const vector<float>& v = stam ? StamAdjust(x, diff) : diff;
	finalMSDvals = v; // bad bad bad bad bad bad bad bad bad bad
	float output = 0.f;
	std::vector<float> pointloss;
	for (size_t i = 0; i < v.size(); i++) {
		float gainedpoints =
		  x > v[i] ? v_itvpoints[i] : v_itvpoints[i] * pow(x / v[i], 1.8f);

		output += gainedpoints;
		pointloss.push_back(v_itvpoints[i] - gainedpoints);
	}
	pointslost = pointloss; // to the bone
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
		output.push_back( taps != 0 ? pow(1 - (static_cast<float>(jumptaps)/
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
		output[i] = taps != 0 ? sqrt(sqrt(1 - (static_cast<float>(handtaps) /
											   static_cast<float>(taps))))
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

vector<float>
Calc::RollDownscaler(const Finger& f1, const Finger& f2)
{
	vector<float> output(
	  f1.size()); // this is slightly problematic because if one finger is
				  // longer than the other you could potentially have different
				  // results with f1 and f2 switched
	for (size_t i = 0; i < f1.size(); i++) {
		if (f1[i].size() + f2[i].size() <= 1) {
			output[i] = 1.f;
			continue;
		}
		vector<float> hand_intervals;
		for (float time1 : f1[i])
			hand_intervals.emplace_back(time1);
		for (float time2 : f2[i])
			hand_intervals.emplace_back(time2);

		float interval_mean = mean(hand_intervals);

		for (float& note : hand_intervals)
			if (interval_mean / note < 0.6f)
				note = interval_mean;

		float interval_cv = cv(hand_intervals) + 0.85f;
		output[i] = interval_cv >= 1.0f
					  ? min(sqrt(sqrt(interval_cv)), 1.075f)
					  : interval_cv * interval_cv * interval_cv;

		if (logpatterns)
			std::cout << "ro " << output[i] << std::endl;
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
	if (NoteInfo.empty()) {
		return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	}
	return std::make_unique<Calc>()->CalcMain(NoteInfo, musicrate, min(goal, ssrcap));
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
				vector<vector<float>>& handInfo,
				CalcPatternMod cpm)
{
	if (NoteInfo.size() <= 1)
		return;

	std::unique_ptr<Calc> debugRun = std::make_unique<Calc>();
	debugRun->debugMod = cpm;
	debugRun->CalcMain(NoteInfo, musicrate, min(goal, ssrcap));

	// Locate and modify the uses of left/right debug in the code
	handInfo.push_back(debugRun->left_hand.debug);
	handInfo.push_back(debugRun->right_hand.debug);
}


int
GetCalcVersion()
{
	return 264;
}
