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
using std::pair;
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
	0.F, 0.97F, 0.92F, 0.83F, 0.94F, 0.95F, 0.73F, 1.F
};

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
hit_the_road(float x, float y, int /*mode*/) -> float
{
	return (CalcClamp(
	  magic_num - (magic_num * fastpow(x / y, 2.5F)), 0.F, magic_num));
}

// returns a positive number or 0, output should be subtracted
auto
Calc::JackLoss(float x, int mode, float mpl, bool stam, bool debug) -> float
{
	return 0.f;

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
			eff_ms = 500.f; // sum(window_taps) + mode_buffers[mode];
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
					for (int i = 0; i < h.soap[NPSBase].size(); ++i) {
						float val = h.base_adj_diff[highest_base_skillset][i] /
									h.soap[TechBase][i];
						h.debugValues[0][TotalPatternMod][i] = val;
					}
				} else if (highest_base_skillset == Skill_Chordjack) {
					h.debugValues[0][TotalPatternMod].resize(numitv);
					for (int i = 0; i < h.soap[NPSBase].size(); ++i) {
						float val = h.base_adj_diff[highest_base_skillset][i] /
									(h.soap[CJBase][i] + h.soap[NPSBase][i]) /
									2.F;
						h.debugValues[0][TotalPatternMod][i] = val;
					}
				} else {
					h.debugValues[0][TotalPatternMod].resize(numitv);
					for (int i = 0; i < h.soap[NPSBase].size(); ++i) {
						float val = h.base_adj_diff[highest_base_skillset][i] /
									h.soap[NPSBase][i];
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

	//// sequence jack immediately so we can ref pass & sort in calc
	//// msestimate without things going be wackying
	// for (auto m : zto3) {
	//	jacks[m]->resize(4);
	//	for (auto t : zto3) {
	//		SequenceJack(fingers[t], t, m);

	//		// resize stam adjusted jack vecs, technically if we flattened
	//		// the vector we could allocate only once for all rate passes
	//		// when doing caching, but for various other reasons it was
	//		// easier to keep them split by intervals in a double vector,
	//		// this should maybe be changed?
	//		stam_adj_jacks[t].resize(fingers[t].size());
	//		for (int i = 0; i < fingers[t].size(); ++i) {
	//			stam_adj_jacks[t][i].resize(fingers[t][i].size());
	//		}
	//	}
	//}

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

	// post pattern mod smoothing for cj
	Smooth(left_hand.base_adj_diff[Skill_Chordjack], 1.f);
	Smooth(right_hand.base_adj_diff[Skill_Chordjack], 1.f);

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

void
Hand::InitBaseDiff(Finger& f1, Finger& f2)
{
	static const bool dbg = false;

	for (int i = 0; i < NUM_CalcDiffValue - 1; ++i) {
		soap[i].resize(f1.size());
	}

	for (int i = 0; i < f1.size(); i++) {
		// scaler for things with higher things
		static const float higher_thing_scaler = 1.175F;
		float nps = 1.6F * static_cast<float>(f1[i].size() + f2[i].size());
		soap[NPSBase][i] = finalscaler * nps;
	}

	Smooth(soap[NPSBase], 0.F);
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

			left_hand.CalcInternal(gotpoints, player_skill, ss, stamina);

			// already can't reach goal, move on
			if (gotpoints > reqpoints) {
				right_hand.CalcInternal(gotpoints, player_skill, ss, stamina);
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
		  // Roll,
		  // WideRangeAnchor,
		},

		// hs
		{ HS,
		  OHJumpMod,
		  TheThing,
		  WideRangeAnchor,
		  WideRangeRoll,
		  OHTrill,
		  Roll },

		// stam, nothing, don't handle here
		{},

		// jackspeed
		{
		  OHTrill,
		  Balance,
		  Roll,
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
		},

		// chordjack
		{ CJ, CJDensity, WideRangeAnchor },

		// tech, duNNO wat im DOIN
		{
		  OHTrill,
		  Balance,
		  Roll,
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
		},

	};

	for (int i = 0; i < NUM_Skillset; ++i) {
		base_adj_diff[i].resize(soap[NPSBase].size());
		base_diff_for_stam_mod[i].resize(soap[NPSBase].size());
	}

	// ok this loop is pretty wack i know, for each interval
	for (int i = 0; i < soap[NPSBase].size(); ++i) {
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
			float funk = soap[NPSBase][i] * tp_mods[ss] * basescalers[ss];
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
					  fastsqrt(doot[RanMan][i] - 0.125F), 0.98F, 1.06F);
					adj_diff /= fastsqrt(doot[OHJumpMod][i] * 0.95F);

					/*adj_diff *=
					  CalcClamp(fastsqrt(doot[RanMan][i] - 0.2f), 1.f, 1.05f);*/
					// maybe we should have 2 loops to avoid doing
					// math twice
					stam_base = max(
					  adj_diff, soap[NPSBase][i] * tp_mods[Skill_Handstream]);
					break;
				case Skill_Handstream:
					// adj_diff /= fastsqrt(doot[OHJump][i]);
					stam_base =
					  max(funk, soap[NPSBase][i] * tp_mods[Skill_Jumpstream]);
					break;
				case Skill_JackSpeed:
					adj_diff = soap[JackBase][i] * tp_mods[Skill_JackSpeed] *
							   basescalers[ss] /
							   max(fastpow(doot[CJ][i], 2.F), 1.F);
					break;
				case Skill_Chordjack:
					adj_diff =
					  soap[CJBase][i] * tp_mods[Skill_Chordjack] *
					  basescalers[ss] *
					  CalcClamp(fastsqrt(doot[OHJumpMod][i]) + 0.06F, 0.F, 1.F);
					break;
				case Skill_Technical:
					adj_diff =
					  soap[TechBase][i] * tp_mods[ss] * basescalers[ss] /
					  max(fastpow(doot[CJ][i], 2.F), 1.F) /
					  max(max(doot[Stream][i], doot[JS][i]), doot[HS][i]) *
					  doot[Chaos][i] / fastsqrt(doot[RanMan][i]);
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

int mina_calc_version = 390;
auto
GetCalcVersion() -> int
{
#ifdef USING_NEW_CALC
	return mina_calc_version;
#else
	return 263;
#endif
}
