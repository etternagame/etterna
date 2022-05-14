//
// Created by Robert on 11/20/2019.
// Modified for N-keys by poco0317
//

#include <vector>
#include <cmath>
#include <algorithm>
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "SoloCalc.h"

#include <numeric>

// using std::vector;
using namespace ::std;

// multipliers to each skillset
/*
static const std::array<float, NUM_Skillset> basescalars = {
	0.F, 1.F, 1.F, 1.F, 1.F, 1.F, 1.F, 1.F
};
*/

// global multiplier for baseline standardization
static constexpr float finalscaler = 2.6f * 1.4f;

inline void
Smooth(std::vector<float>& input, float neutral)
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
DifficultyMSSmooth(std::vector<float>& input)
{
	float f2 = 0.f;

	for (float& i : input) {
		float f1 = f2;
		f2 = i;
		i = (f1 + f2) / 2.f;
	}
}

float
CalcMSEstimate(std::vector<float>& input)
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
CalcInternal(float x, std::vector<float>& diff, std::vector<int>& v_itvpoints)
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
	   std::vector<float>& ldiff,
	   std::vector<int>& lv_itvpoints,
	   std::vector<float>& rdiff,
	   std::vector<int>& rv_itvpoints,
	   std::vector<float>& mdiff,
	   std::vector<int>& mv_itvpoints,
	   float MaxPoints)
{
	float lower = 0.0f;
	float upper = 100.0f;
	while (upper - lower > 0.01f) {
		float mid = (lower + upper) / 2.f;
		float gotpoints = CalcInternal(mid, ldiff, lv_itvpoints) +
						  CalcInternal(mid, rdiff, rv_itvpoints) +
						  CalcInternal(mid, mdiff, mv_itvpoints);
		if (gotpoints / MaxPoints < score_goal) {
			lower = mid;
		} else {
			upper = mid;
		}
	}

	return lower;
}

void
setHandDiffs(std::vector<float>& NPSdiff,
			 std::vector<float>& MSdiff,
			 std::vector<vector<vector<float>>>& AllIntervals,
			 unsigned columnToStart,
			 unsigned columnsPerHand)
{
	// early exit out of bounds
	if (columnToStart > AllIntervals.size() || columnToStart < 0 || columnToStart + columnsPerHand > AllIntervals.size())
		return;
	
	for (size_t i = 0; i < AllIntervals[columnToStart].size(); i++) {
		
		float nps = 0;
		for (unsigned col = columnToStart; col < columnToStart + columnsPerHand;
			 col++)
			nps += static_cast<float>(AllIntervals[col][i].size());
		nps *= 1.6f;

		float difficulty = 0.f;
		for (unsigned col = columnToStart; col < columnToStart + columnsPerHand; col++) {
			float finger_difficulty = CalcMSEstimate(AllIntervals[col][i]);
			if (finger_difficulty > difficulty)
				difficulty = finger_difficulty;
		}
		
		NPSdiff[i] = finalscaler * nps;
		MSdiff[i] = finalscaler * (5.f * difficulty + 4.f * nps) / 9.f;
	}
	Smooth(NPSdiff, 0.f);
	DifficultyMSSmooth(MSdiff);
}

MinaSD
SoloCalc(const std::vector<NoteInfo>& notes, unsigned columnCount)
{

	MinaSD allrates;

	int rateCount = 21;

	if (notes.size() > 1) {
		for (int i = 7; i < rateCount; i++) {
			auto tempVal = SoloCalc(notes, columnCount, i / 10.f, 0.93f);
			allrates.emplace_back(tempVal);
		}
	} else {
		std::vector<float> o{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };

		for (int i = 7; i < rateCount; i++) {
			allrates.emplace_back(o);
		}
	}
	return allrates;
}

std::vector<float>
SoloCalc(const std::vector<NoteInfo>& notes, unsigned columnCount, float music_rate, float goal)
{
	std::vector<float> skills{};

	std::vector<vector<vector<float>>> AllIntervals(columnCount,
											   std::vector<vector<float>>());
	int num_itv =
	  static_cast<int>(std::ceil(notes.back().rowTime / (music_rate * 0.5f)));
	for (unsigned int t = 0; t < columnCount; t++) {
		int Interval = 0;
		float last = -5.f;
		AllIntervals[t] = std::vector<vector<float>>(num_itv, std::vector<float>());
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

	int columnsPerHand = columnCount / 2;
	bool hasCenterColumn = columnCount % 2 != 0;

	int leftHandStartColumn = 0;
	int rightHandStartColumn = columnCount / 2 + (hasCenterColumn ? 1 : 0);
	
	std::vector<int> lv_itvpoints;
	std::vector<int> mv_itvpoints;
	std::vector<int> rv_itvpoints;
	std::vector<float> lv_itvMSdiff;
	std::vector<float> mv_itvMSdiff;
	std::vector<float> rv_itvMSdiff;

	// check column count if the column count happens to be 1
	// column count 1 would break this logic
	if (columnCount > 1) {
		std::vector<float> lv_itvNPSdiff(AllIntervals[0].size());
		lv_itvMSdiff.resize(AllIntervals[0].size());
		std::vector<float> rv_itvNPSdiff(AllIntervals[0].size());
		rv_itvMSdiff.resize(AllIntervals[0].size());
		
		setHandDiffs(lv_itvNPSdiff,
					 lv_itvMSdiff,
					 AllIntervals,
					 leftHandStartColumn,
					 columnsPerHand);
		setHandDiffs(rv_itvNPSdiff,
					 rv_itvMSdiff,
					 AllIntervals,
					 rightHandStartColumn,
					 columnsPerHand);
		
		for (size_t i = 0; i < AllIntervals[0].size(); i++) {
			auto ptcnt = 0;
			for (auto col = leftHandStartColumn;
				 col < leftHandStartColumn + columnsPerHand;
				 col++) {
				ptcnt += static_cast<int>(AllIntervals[col][i].size());
			}
			lv_itvpoints.emplace_back(ptcnt);
		}
		for (size_t i = 0; i < AllIntervals[0].size(); i++) {
			auto ptcnt = 0;
			for (auto col = rightHandStartColumn;
				 col < rightHandStartColumn + columnsPerHand;
				 col++) {
				ptcnt += static_cast<int>(AllIntervals[col][i].size());
			}
			rv_itvpoints.emplace_back(ptcnt);
		}
	}
	
	// for the center column lovers
	if (hasCenterColumn) {
		std::vector<float> mv_itvNPSdiff(AllIntervals[0].size());
		mv_itvMSdiff.resize(AllIntervals[0].size());
		setHandDiffs(mv_itvNPSdiff,
				 mv_itvMSdiff,
				 AllIntervals,
				 leftHandStartColumn + columnsPerHand,
				 1);
		for (size_t i = 0; i < AllIntervals[0].size(); i++) {
			mv_itvpoints.emplace_back(static_cast<int>(
			  AllIntervals[std::max(0, rightHandStartColumn - 1)][i].size()));
		}
	}

	float MaxPoints = static_cast<float>(
	  std::accumulate(lv_itvpoints.begin(), lv_itvpoints.end(), 0) +
	  std::accumulate(mv_itvpoints.begin(), mv_itvpoints.end(), 0) +
	  std::accumulate(rv_itvpoints.begin(), rv_itvpoints.end(), 0));
	
	float sd = Chisel(min(goal, 0.965f),
					  lv_itvMSdiff,
					  lv_itvpoints,
					  rv_itvMSdiff,
					  rv_itvpoints,
					  mv_itvMSdiff,
					  mv_itvpoints,
					  MaxPoints);

	// hack to return the same sd for all skillsets, for now
	// doing it this way because of how ix structured this -five
	skills.reserve(8);
	for (int i = 0; i < 8; i++) {
		skills.emplace_back(sd);
	}

	return skills;
}
