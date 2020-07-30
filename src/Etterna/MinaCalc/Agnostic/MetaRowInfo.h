#pragma once
#include "HA_Sequencing.h"
#include "MetaIntervalInfo.h"
#include "../SequencingHelpers.h"

/* counterpart to metahandinfo */

struct metaRowInfo
{
	static const bool dbg = false;
	static const bool dbg_lv2 = false;

	float time = s_init;
	// time from last row (ms)
	float ms_now = ms_init;
	int count = 0;
	int last_count = 0;
	int last_last_count = 0;
	unsigned notes = 0;
	unsigned last_notes = 0;
	unsigned last_last_notes = 0;

	// per row bool flags, these must be directly set every row
	bool alternating_chordstream = false;
	bool alternating_chord_single = false;
	bool gluts_maybe = false; // not really used/tested yet
	bool twas_jack = false;

	void reset()
	{		
		time = s_init;
		ms_now = ms_init;
		count = 0;
		last_count = 0;
		last_last_count = 0;
		notes = 0;
		last_notes = 0;
		last_last_notes = 0;

		alternating_chordstream = false;
		alternating_chord_single = false;
		gluts_maybe = false;
		twas_jack = false;
	}

	void set_row_variations(metaItvInfo& mitvi) const
	{
		// already determined there's enough variation in this interval
		if (!mitvi.basically_vibro) {
			return;
		}

		// trying to fill array with up to 3 unique row_note configurations
		for (auto& t : mitvi.row_variations) {
			// already a stored value here
			if (t != 0) {
				// already have one of these
				if (t == notes) {
					return;
				}
			} else if (t == 0) {
				// nothing stored here and isn't a duplicate, store it and
				// iterate num_var
				t = notes;
				++mitvi.num_var;

				// check if we filled the array with unique values. since we
				// start by assuming anything is basically vibro, set the flag
				// to false if it is
				if (mitvi.row_variations[2] != 0) {
					mitvi.basically_vibro = false;
				}
				return;
			}
		}
	}

	// scan for jacks and jack counts between this row and the last
	void jack_scan(metaItvInfo& mitvi)
	{
		twas_jack = false;

		for (const auto& id : col_ids) {
			if (is_jack_at_col(id, notes, last_notes)) {
				// not scaled to the number of jacks anymore
				++mitvi.actual_jacks;
				twas_jack = true;
				// try to pick up gluts maybe?
				if (count > 1 && column_count(last_notes) > 1) {
					++mitvi.shared_chord_jacks;
				}
			}
		}

		// if we used the normal actual_jack for CJ too we're saying something
		// like "chordjacks" are harder if they share more columns from chord to
		// chord" which is not true, it is in fact either irrelevant or the
		// inverse depending on the scenario, this is merely to catch stuff like
		// splithand jumptrills registering as chordjacks when they shouldn't be
		if (twas_jack) {
			++mitvi.actual_jacks_cj;
		}
	}

	void basic_row_sequencing(const metaRowInfo& last, metaItvInfo& mitvi)
	{
		jack_scan(mitvi);
		set_row_variations(mitvi);

		// check if we have a bunch of stuff like [123]4[123] [12]3[124] which
		// isn't actually chordjack, its just broken hs/js, and in fact with the
		// level of leniency that is currently being applied to generic
		// proportions, lots of heavy js/hs is being counted as cj for their 2nd
		// rating, and by a close margin too, we can't just look for [123]4, we
		// need to finish the sequence to be sure i _think_ we only want to do
		// this for single notes, we could abstract it to a more generic pattern
		// template, but let's be restrictive for now
		alternating_chordstream =
		  is_alternating_chord_stream(notes, last_notes, last.last_notes);
		if (alternating_chordstream) {
			++mitvi.definitely_not_jacks;
		}

		if (alternating_chordstream) {
			// put mixed density stuff here later
		}

		// only cares about single vs chord, not jacks
		alternating_chord_single =
		  is_alternating_chord_single(count, last.count);
		if (alternating_chord_single) {
			if (!twas_jack) {
				mitvi.seriously_not_js -= 3;
			}
		}

		if (last.count == 1 && count == 1) {
			mitvi.seriously_not_js =
			  0 > mitvi.seriously_not_js ? 0 : mitvi.seriously_not_js;
			++mitvi.seriously_not_js;

			// light js really stops at [12]321[23] kind of
			// density, anything below that should be picked up
			// by speed, and this stop rolls between jumps
			// getting floated up too high
			if (mitvi.seriously_not_js > 3) {

				mitvi.not_js += mitvi.seriously_not_js;
				// give light hs the light js treatment
				mitvi.not_hs += mitvi.seriously_not_js;
			}
		} else if (last.count > 1 && count > 1) {
			// suppress jumptrilly garbage a little bit
			mitvi.not_hs += count;
			mitvi.not_js += count;

			// might be overkill
			if ((notes & last_notes) == 0) {
				++mitvi.not_hs;
				++mitvi.not_js;
			} else {
				gluts_maybe = true;
			}
		}

		if ((notes & last_notes) == 0 && count > 1 && last_count > 1) {
			if ((last_notes & last.last_notes) == 0 && last_count > 1) {
				mitvi.dunk_it = true;
			}
		}
	}

	void operator()(const metaRowInfo& last,
					metaItvInfo& mitvi,
					const float& row_time,
					const int& row_count,
					const unsigned& row_notes)
	{
		time = row_time;
		last_last_count = last.last_count;
		last_count = last.count;
		count = row_count;

		last_last_notes = last.last_notes;
		last_notes = last.notes;
		notes = row_notes;

		ms_now = ms_from(time, last.time);

		mitvi._itvi.update_tap_counts(count);
		basic_row_sequencing(last, mitvi);
	}
};
