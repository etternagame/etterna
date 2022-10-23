#pragma once
#include "HD_BasicSequencing.h"
#include "HD_MetaSequencing.h"

/* this is a row by row sequencer that constructs basic and advanced hand based
 * patterns given noteinfo input for a current row, and its own output of the
 * last row */

// perhaps this should contain no timing information, only pattern information?

/// potentially outdated comment below
/// this should contain most everything needed for the generic pattern mods,
/// extremely specific sequencing will take place in separate areas like with
/// rm_sequencing, and widerange scalers should track their own interval queues
/// metanoteinfo is generated per row, from current noteinfo and the previous
/// metanoteinfo object, each metanoteinfo stores some basic information from
/// the last object, allowing us to look back 3-4 rows into the past without
/// having to explicitly store more than 2 mni objects, and we can recycle the
/// pointers as we generate the info metanoteinfo is generated per _hand_, not
/// per note or column. it contains the relevant information for determining
/// what the column configuation of each hand is for any row, and it contains
/// timestamp arrays for each column, so it is unnecessary to generate
/// information per note, even though in some ways it might be more convenient
/// or clearer
struct metaHandInfo
{
	/// col
	col_type _ct = col_init;
	col_type _last_ct = col_init;

	/// type of cross column hit
	base_type _bt = base_type_init;
	base_type _last_bt = base_type_init;

	/// needed for the BIGGEST BRAIN PLAYS
	base_type last_last_bt = base_type_init;

	// whomst've
	meta_type _mt = meta_type_init;
	meta_type _last_mt = meta_type_init;

	/// number of offhand taps before this row
	int offhand_taps = 0;
	int offhand_ohjumps = 0;

	/// we need to reset everything between hands or the trailing values from the
	/// end of one will carry over into the start of the other, not a huge
	/// practical deal but it could theoretically be abused and it's good
	/// practice to reset anyway
	void full_reset()
	{
		_ct = col_init;
		_last_ct = col_init;

		_bt = base_type_init;
		_last_bt = base_type_init;
		last_last_bt = base_type_init;

		_mt = meta_type_init;
		_last_mt = meta_type_init;
	}

	void operator()(const metaHandInfo& last, const col_type& ct)
	{
		// this should never ever be called on col_empty
		assert(ct != col_empty);

		_ct = ct;

		// set older values, yeah yeah, i know
		_last_ct = last._ct;
		last_last_bt = last._last_bt;
		_last_bt = last._bt;
		_last_mt = last._mt;

		/* ok now actually do stuff */

		// update this hand's cc type for this row
		_bt = determine_base_pattern_type(ct, _last_ct);

		// now that we have determined base_type, we can look for more complex
		// patterns
		_mt = determine_meta_type(
		  _bt, _last_bt, last_last_bt, last.last_last_bt, _last_mt);
	}
};
