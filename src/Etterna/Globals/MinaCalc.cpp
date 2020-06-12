#include "MinaCalc.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <cassert>

#include "MinaCalc/Ulbu.h"
#include "MinaCalcHelpers.h"

using std::max;
using std::min;
using std::pow;
using std::vector;

static const char note_map[16][5]{ "----", "1---", "-1--", "11--",
								   "--1-", "1-1-", "-11-", "111-",
								   "---1", "1--1", "-1-1", "11-1",
								   "--11", "1-11", "-111", "1111" };

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
static const std::array<float, NUM_Skillset> basescalers = {
	0.F, 0.97F, 0.92F, 0.83F, 0.94F, 0.95F, 0.91F, 0.9F
};

static thread_local int numitv = 0;

void
Calc::TotalMaxPoints()
{
	MaxPoints = 0;
	for (int i = 0; i < numitv; i++) {
		MaxPoints += l_hand.v_itvpoints[i] + r_hand.v_itvpoints[i];
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

inline void
InitBaseDiff(const int& h, Finger& f1, Finger& f2)
{
	for (int i = 0; i < f1.size(); i++) {
		float nps = 1.6F * static_cast<float>(f1[i].size() + f2[i].size());
		soap.at(h).at(NPSBase).at(i) = finalscaler * nps;
	}
}

auto
Calc::ProcessFinger(const vector<NoteInfo>& NoteInfo,
					unsigned int t,
					float music_rate,
					float offset,
					bool& joke_file_mon) -> Finger
{
	// optimization, just allocate memory here once and recycle this vector
	vector<float> temp_queue(max_rows_for_single_interval);
	vector<int> temp_queue_two(max_rows_for_single_interval);
	unsigned int row_counter = 0;
	unsigned int row_counter_two = 0;

	if (numitv >= max_intervals) {
		joke_file_mon = true;
		return {};
	}

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
		if (row_counter >= max_rows_for_single_interval ||
			row_counter_two >= max_rows_for_single_interval) {
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

	// for multi offset passes- super breaks stuff atm dunno why???
	// const int fo_rizzy = ssr ? 5 : 1;
	const int fo_rizzy = 1;
	vector<vector<float>> the_hizzle_dizzles(fo_rizzy);
	for (int WHAT_IS_EVEN_HAPPEN_THE_BOMB = 0;
		 WHAT_IS_EVEN_HAPPEN_THE_BOMB < fo_rizzy;
		 ++WHAT_IS_EVEN_HAPPEN_THE_BOMB) {

		bool continue_calc = InitializeHands(
		  NoteInfo, music_rate, 0.1F * WHAT_IS_EVEN_HAPPEN_THE_BOMB);

		// if we exceed max_rows_for_single_interval during
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

	TheGreatBazoinkazoinkInTheSky ulbu_that_which_consumes_all;
	ulbu_that_which_consumes_all.recieve_sacrifice(NoteInfo);

	InitBaseDiff(left_hand, fingers[0], fingers[1]);
	InitBaseDiff(right_hand, fingers[2], fingers[3]);

	l_hand.InitPoints(fingers[0], fingers[1]);
	r_hand.InitPoints(fingers[2], fingers[3]);

	ulbu_that_which_consumes_all(nervIntervals, music_rate);

	l_hand.InitAdjDiff();
	r_hand.InitAdjDiff();

	// post pattern mod smoothing for cj
	Smooth(base_adj_diff.at(left_hand).at(Skill_Chordjack), 1.f, numitv);
	Smooth(base_adj_diff.at(right_hand).at(Skill_Chordjack), 1.f, numitv);

	//// debug info loop
	// if (debugmode) {
	//	for (auto& hp : spoopy) {
	//		auto& hand = hp.first;

	//		// pattern mods and base msd never change, set degbug output
	//		// for them now

	//		// 3 = number of different debug types
	//		hand.debugValues.resize(3);
	//		hand.debugValues[0].resize(ModCount);
	//		hand.debugValues[1].resize(NUM_CalcDiffValue);
	//		hand.debugValues[2].resize(NUM_CalcDebugMisc);

	//		for (int i = 0; i < ModCount; ++i) {
	//			hand.debugValues[0][i] = doot.at(hp.second).at(i);
	//		}

	//		// set everything but final adjusted output here
	//		for (int i = 0; i < NUM_CalcDiffValue - 1; ++i) {
	//			hand.debugValues[1][i] = hand.soap.at(hi).at(i];
	//		}
	//	}
	//}
	return true;
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

			l_hand.CalcInternal(gotpoints, player_skill, ss, stamina);

			// only run the other hand if we're still above the reqpoints, if
			// we're already below, there's no point
			if (gotpoints > reqpoints) {
				r_hand.CalcInternal(gotpoints, player_skill, ss, stamina);
			}

		} while (gotpoints < reqpoints);
		player_skill -= resolution;
		resolution /= 2.F;
	}

	// these are the values for msd/stam adjusted msd/pointloss the
	// latter two are dependent on player_skill and so should only
	// be recalculated with the final value already determined
	// getting the jackstam debug output right is lame i know
	if (debugoutput) {
		l_hand.CalcInternal(gotpoints, player_skill, ss, stamina, debugoutput);
		r_hand.CalcInternal(gotpoints, player_skill, ss, stamina, debugoutput);
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
		  VOHTrill,
		  // Roll,
		  Chaos,
		  WideRangeRoll,
		  WideRangeJumptrill,
		  FlamJam,
		  OHJumpMod,
		  Balance,
		  RanMan,
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
		  VOHTrill,
		  RanMan,
		  // Roll,
		  // WideRangeAnchor,
		},

		// hs
		{
		  HS,
		  OHJumpMod,
		  TheThing,
		  WideRangeAnchor,
		  WideRangeRoll,
		  OHTrill,
		  VOHTrill,
		  // Roll
		  RanMan,
		},

		// stam, nothing, don't handle here
		{},

		// jackspeed
		{},

		// chordjack
		{ CJ, CJDensity },

		// tech, duNNO wat im DOIN
		{
		  OHTrill,
		  VOHTrill,
		  Balance,
		  // Roll,
		  OHJumpMod,
		  Chaos,
		  WideRangeJumptrill,
		  WideRangeBalance,
		  WideRangeRoll,
		  FlamJam,
		  RanMan,
		  WideRangeAnchor,
		  TheThing,
		  TheThing2,
		  RanMan,
		},

	};

	// ok this loop is pretty wack i know, for each interval
	for (int i = 0; i < numitv; ++i) {
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
				tp_mods[ss] *= doot.at(hi).at(pmod).at(i);
			}
		}

		// main skillset loop, for each skillset that isn't overall
		// or stam
		for (int ss = 0; ss < NUM_Skillset; ++ss) {
			if (ss == Skill_Overall || ss == Skill_Stamina) {
				continue;
			}

			// this should work and not be super slow?
			auto& adj_diff = base_adj_diff.at(hi).at(ss).at(i);
			auto& stam_base = base_diff_for_stam_mod.at(hi).at(ss).at(i);

			// might need optimization, or not since this is not
			// outside of a dumb loop now and is done once instead
			// of a few hundred times
			float funk =
			  soap.at(hi).at(NPSBase).at(i) * tp_mods[ss] * basescalers.at(ss);
			adj_diff = funk;
			stam_base = funk;
			switch (ss) {
				// do funky special case stuff here
				case Skill_Stream:
					adj_diff *=
					  CalcClamp(fastsqrt(doot.at(hi).at(RanMan).at(i) - 0.125F),
								1.F,
								1.05F);
					break;

				// test calculating stam for js/hs on max js/hs diff
				// we want hs to count against js so they are
				// mutually exclusive
				case Skill_Jumpstream:
					adj_diff /= max(doot.at(hi).at(HS).at(i), 1.F);
					adj_diff *=
					  CalcClamp(fastsqrt(doot.at(hi).at(RanMan).at(i) - 0.125F),
								0.98F,
								1.06F);
					adj_diff /=
					  fastsqrt(doot.at(hi).at(OHJumpMod).at(i) * 0.95F);

					/*adj_diff *=
					  CalcClamp(fastsqrt(doot.at(hi).at(RanMan).at(i) -
					  0.2f), 1.f, 1.05f);*/
					// maybe we should have 2 loops to avoid doing
					// math twice
					stam_base = max(adj_diff,
									soap.at(hi).at(NPSBase).at(i) *
									  tp_mods[Skill_Handstream]);
					break;
				case Skill_Handstream:
					// adj_diff /= fastsqrt(doot.at(hi).at(OHJump).at(i));
					stam_base = max(funk,
									soap.at(hi).at(NPSBase).at(i) *
									  tp_mods[Skill_Jumpstream]);
					break;
				case Skill_JackSpeed:
					adj_diff = soap.at(hi).at(JackBase).at(i) *
							   tp_mods[Skill_JackSpeed] * basescalers.at(ss) /
							   max(fastpow(doot.at(hi).at(CJ).at(i), 2.F), 1.F);
					break;
				case Skill_Chordjack:
					adj_diff = soap.at(hi).at(CJBase).at(i) *
							   tp_mods[Skill_Chordjack] * basescalers.at(ss);
					break;
				case Skill_Technical:
					adj_diff =
					  soap.at(hi).at(TechBase).at(i) * tp_mods[ss] *
					  basescalers.at(ss) /
					  max(fastpow(doot.at(hi).at(CJ).at(i), 2.F), 1.F) /
					  max(max(doot.at(hi).at(Stream).at(i),
							  doot.at(hi).at(JS).at(i)),
						  doot.at(hi).at(HS).at(i)) *
					  doot.at(hi).at(Chaos).at(i) /
					  fastsqrt(doot.at(hi).at(RanMan).at(i));
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

	if (stam && ss != Skill_JackSpeed) {
		StamAdjust(x, ss);
	}

	// final difficulty values to use
	const auto& v = stam ? stam_adj_diff : base_adj_diff.at(hi).at(ss);
	float powindromemordniwop = 1.7F;
	if (ss == Skill_Chordjack) {
		powindromemordniwop = 1.7F;
	}

	// i don't like the copypasta either but the boolchecks where
	// they were were too slow
	// if (debug) {
	//	debugValues[2][StamMod].resize(v.size());
	//	debugValues[2][PtLoss].resize(v.size());
	//	// final debug output should always be with stam activated
	//	StamAdjust(x, ss, true);
	//	debugValues[1][MSD] = stam_adj_diff;

	//	for (int i = 0; i < v.size(); ++i) {
	//		if (x < v[i]) {
	//			auto pts = static_cast<float>(v_itvpoints[i]);
	//			float lostpoints =
	//			  (pts - (pts * fastpow(x / v[i], powindromemordniwop)));
	//			gotpoints -= lostpoints;
	//			debugValues[2][PtLoss][i] = abs(lostpoints);
	//		}
	//	}
	//} else {
	for (int i = 0; i < numitv; ++i) {
		if (x < v[i]) {
			auto pts = static_cast<float>(v_itvpoints[i]);
			gotpoints -= (pts - (pts * fastpow(x / v[i], powindromemordniwop)));
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
	const auto& base_diff = base_diff_for_stam_mod.at(hi).at(ss);
	// but apply the mod growth to these values
	// they might be the same, or not
	const auto& diff = base_adj_diff.at(hi).at(ss);

	// i don't like the copypasta either but the boolchecks where
	// they were were too slow
	if (debug) {
		for (int i = 0; i < numitv; i++) {
			avs1 = avs2;
			avs2 = base_diff.at(i);
			mod += ((((avs1 + avs2) / 2.F) / (stam_prop * x)) - 1.F) / stam_mag;
			if (mod > 0.95F) {
				stam_floor += (mod - 0.95F) / stam_fscale;
			}
			local_ceil = stam_ceil * stam_floor;

			mod = min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
			stam_adj_diff.at(i) = diff.at(i) * mod;
			debugValues[2][StamMod][i] = mod;
		}
	} else {
		for (int i = 0; i < numitv; i++) {
			avs1 = avs2;
			avs2 = base_diff.at(i);
			mod += ((((avs1 + avs2) / 2.F) / (stam_prop * x)) - 1.F) / stam_mag;
			if (mod > 0.95F) {
				stam_floor += (mod - 0.95F) / stam_fscale;
			}
			local_ceil = stam_ceil * stam_floor;

			mod = min(CalcClamp(mod, stam_floor, local_ceil), super_stam_ceil);
			stam_adj_diff.at(i) = diff.at(i) * mod;
		}
	}
}

static const float ssr_goal_cap = 0.965F; // goal cap to prevent insane scaling
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

	handInfo.emplace_back(debugRun->l_hand.debugValues);
	handInfo.emplace_back(debugRun->r_hand.debugValues);

	// asdkfhjasdkfhaskdfjhasfd
	if (!DoesFileExist(calc_params_xml)) {
		TheGreatBazoinkazoinkInTheSky ublov;
		ublov.write_params_to_disk();
	}
}

int mina_calc_version = 394;
auto
GetCalcVersion() -> int
{
	return mina_calc_version;
}
