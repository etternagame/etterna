//
// Created by Robert on 11/20/2019.
//

#include <vector>
#include <cmath>
#include <algorithm>
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "SoloCalc.h"

// using std::vector;
using namespace ::std;

const float finalscaler = 2.564f * 1.05f * 1.1f * 1.10f * 1.10f *
						  1.025f; // multiplier to standardize baselines

inline void
Smooth(vector<float>& input, float neutral)
{
	float f2 = neutral;
	float f3 = neutral;

	for (float& i : input) {
		float f1 = f2;
		f2 = f3;
		f3 = i;
		i = (f1 + f2 + f3) / 3;
	}
}

inline void
DifficultyMSSmooth(vector<float>& input)
{
	float f2 = 0.f;

	for (float& i : input) {
		float f1 = f2;
		f2 = i;
		i = (f1 + f2) / 2.f;
	}
}

float
CalcMSEstimate(vector<float>& input)
{
	if (input.empty())
		return 0.f;

	std::sort(input.begin(), input.end());
	float m = 0;
	size_t End = std::min(input.size(), static_cast<size_t>(6));
	for (size_t i = 0; i < End; i++)
		m += input[i];
	return 1375.f * End / m;
}

float
CalcInternal(float x, vector<float>& diff, vector<int>& v_itvpoints)
{
	float output = 0.f;
	for (size_t i = 0; i < diff.size(); i++) {
		output += x > diff[i] ? v_itvpoints[i]
							  : v_itvpoints[i] * std::pow(x / diff[i], 1.8f);
	}
	return output;
}

float
Chisel(float score_goal,
	   vector<float>& ldiff,
	   vector<int>& lv_itvpoints,
	   vector<float>& rdiff,
	   vector<int>& rv_itvpoints,
	   float MaxPoints)
{
	float lower = 0.0f;
	float upper = 100.0f;
	while (upper - lower > 0.01f) {
		float mid = (lower + upper) / 2.f;
		float gotpoints = CalcInternal(mid, ldiff, lv_itvpoints) +
						  CalcInternal(mid, rdiff, rv_itvpoints);
		if (gotpoints / MaxPoints < score_goal) {
			lower = mid;
		} else {
			upper = mid;
		}
	}

	return lower;
}

void
setHandDiffs(vector<float>& NPSdiff,
			 vector<float>& MSdiff,
			 vector<vector<vector<float>>>& AllIntervals,
			 int column)
{
	for (size_t i = 0; i < AllIntervals[column].size(); i++) {
		float nps =
		  1.6f * static_cast<float>(AllIntervals[column][i].size() +
									AllIntervals[column + 1][i].size() +
									AllIntervals[column + 2][i].size());
		float left_difficulty = CalcMSEstimate(AllIntervals[column][i]);
		float middle_difficulty = CalcMSEstimate(AllIntervals[column + 1][i]);
		float right_difficulty = CalcMSEstimate(AllIntervals[column + 2][i]);
		float difficulty = std::max(
		  std::max(left_difficulty, middle_difficulty), right_difficulty);
		NPSdiff[i] = finalscaler * nps;
		MSdiff[i] = finalscaler * (5.f * difficulty + 4.f * nps) / 9.f;
	}
	Smooth(NPSdiff, 0.f);
	DifficultyMSSmooth(MSdiff);
}

MinaSD
SoloCalc(const std::vector<NoteInfo>& notes)
{

	MinaSD allrates;

	int rateCount = 21;

	if (notes.size() > 1) {
		for (int i = 7; i < rateCount; i++) {
			auto tempVal = SoloCalc(notes, i / 10.f, 0.93f);
			allrates.emplace_back(tempVal);
		}
	} else {
		vector<float> o{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };

		for (int i = 7; i < rateCount; i++) {
			allrates.emplace_back(o);
		}
	}
	return allrates;
}

vector<float>
SoloCalc(const std::vector<NoteInfo>& notes, float music_rate, float goal)
{
	vector<float> skills{};

	vector<vector<vector<float>>> AllIntervals(6, vector<vector<float>>());
	int num_itv =
	  static_cast<int>(std::ceil(notes.back().rowTime / (music_rate * 0.5f)));
	for (unsigned int t = 0; t < 6; t++) {
		int Interval = 0;
		float last = -5.f;
		AllIntervals[t] = vector<vector<float>>(num_itv, vector<float>());
		unsigned int column = 1u << t;

		for (auto i : notes) {
			float scaledtime = i.rowTime / music_rate;

			// why does this OOB calcing certain files if we don't include the
			// bounds check?
			while ((scaledtime > static_cast<float>(Interval + 1) * 0.5f) &&
				   Interval < static_cast<int>(AllIntervals[t].size() - 1))
				++Interval;

			if (i.notes & column) {
				AllIntervals[t][Interval].emplace_back(
				  std::min(std::max(1000 * (scaledtime - last), 40.f), 5000.f));
				last = scaledtime;
			}
		}
	}
	vector<float> lv_itvNPSdiff(AllIntervals[0].size());
	vector<float> lv_itvMSdiff(AllIntervals[0].size());
	vector<float> rv_itvNPSdiff(AllIntervals[0].size());
	vector<float> rv_itvMSdiff(AllIntervals[0].size());

	setHandDiffs(lv_itvNPSdiff, lv_itvMSdiff, AllIntervals, 0);
	setHandDiffs(rv_itvNPSdiff, rv_itvMSdiff, AllIntervals, 3);

	vector<int> lv_itvpoints;
	vector<int> rv_itvpoints;

	for (size_t i = 0; i < AllIntervals[0].size(); i++)
		lv_itvpoints.emplace_back(static_cast<int>(AllIntervals[0][i].size()) +
								  static_cast<int>(AllIntervals[1][i].size()) +
								  static_cast<int>(AllIntervals[2][i].size()));
	for (size_t i = 0; i < AllIntervals[0].size(); i++)
		rv_itvpoints.emplace_back(static_cast<int>(AllIntervals[3][i].size()) +
								  static_cast<int>(AllIntervals[4][i].size()) +
								  static_cast<int>(AllIntervals[5][i].size()));

	float MaxPoints = 0.f;
	for (size_t i = 0; i < lv_itvpoints.size(); i++)
		MaxPoints += static_cast<float>(lv_itvpoints[i] + rv_itvpoints[i]);
	float sd = Chisel(min(goal, 0.965f),
					  lv_itvMSdiff,
					  lv_itvpoints,
					  rv_itvMSdiff,
					  rv_itvpoints,
					  MaxPoints);

	// hack to return the same sd for all skillsets, for now
	// doing it this way because of how ix structured this -five
	skills.reserve(8);
	for (int i = 0; i < 8; i++) {
		skills.emplace_back(sd);
	}

	return skills;
}
