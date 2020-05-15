#include "Etterna/Globals/global.h"
#include "LyricsLoader.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/ThemeManager.h"

#include <algorithm>

// TODO: Use a marker for default color instead of a specific color that may
// accidentally get written back into a lyrics file.
#define LYRICS_DEFAULT_COLOR                                                   \
	THEME->GetMetricC("ScreenGameplay", "LyricDisplayDefaultColor")

static int
CompareLyricSegments(const LyricSegment& seg1, const LyricSegment& seg2)
{
	return static_cast<int>(seg1.m_fStartTime < seg2.m_fStartTime);
}

bool
LyricsLoader::LoadFromLRCFile(const std::string& sPath, Song& out)
{
	Locator::getLogger()->trace("LyricsLoader::LoadFromLRCFile({})", sPath.c_str());

	RageFile input;
	if (!input.Open(sPath)) {
		LuaHelpers::ReportScriptErrorFmt(
		  "Error opening file '%s' for reading: %s",
		  sPath.c_str(),
		  input.GetError().c_str());
		return false;
	}

	auto CurrentColor = LYRICS_DEFAULT_COLOR;

	out.m_LyricSegments.clear();

	for (;;) {
		std::string line;
		auto ret = input.GetLine(line);
		if (ret == 0) {
			break;
		}
		if (ret == -1) {
			LuaHelpers::ReportScriptErrorFmt("Error reading %s: %s",
											 input.GetPath().c_str(),
											 input.GetError().c_str());
			break;
		}

		utf8_remove_bom(line);

		if (!line.compare(0, 2, "//")) {
			continue;
		}
		// (most tags are in the format of...)
		// "[data1] data2".  Ignore whitespace at the beginning of the line.
		static Regex x("^ *\\[([^]]+)\\] *(.*)$");

		vector<std::string> matches;
		if (!x.Compare(line, matches)) {
			continue;
		}
		ASSERT(matches.size() == 2);

		auto& sValueName = matches[0];
		auto& sValueData = matches[1];
		StripCrnl(sValueData);

		// handle the data
		if (EqualsNoCase(sValueName, "COLOUR") ||
			EqualsNoCase(sValueName, "COLOR")) {
			// set color var here for this segment
			int r, g, b;
			auto result = sscanf(sValueData.c_str(), "0x%2x%2x%2x", &r, &g, &b);
			// According to the Dance With Intensity readme, one can set up to
			// ten colors in a line and access them via "{cX}", where X is 0-9.
			if (result != 3) {
				Locator::getLogger()->trace("The color value '{}' in '{}' is invalid.",
						   sValueData.c_str(), sPath.c_str());
				continue;
			}

			CurrentColor = RageColor(r / 256.0f, g / 256.0f, b / 256.0f, 1);
			continue;
		}

		// todo: handle [offset:xxxx] tag? -aj (xxxx is milliseconds)
		// offsets each timestamp after the offset by that amount.
		// float fLyricOffset = 0.0f;

		{
			/* If we've gotten this far, and no other statement caught this
			 * value before this does, assume it's a time value. */

			LyricSegment seg;
			seg.m_Color = CurrentColor;
			seg.m_fStartTime = HHMMSSToSeconds(sValueName);
			seg.m_sLyric = sValueData;

			auto bloo = seg.m_sLyric;

			s_replace(bloo,
					  "|",
					  "\n"); // Pipe symbols denote a new line in LRC files
			seg.m_sLyric = bloo;
			out.AddLyricSegment(seg);
		}
	}

	sort(out.m_LyricSegments.begin(),
		 out.m_LyricSegments.end(),
		 CompareLyricSegments);
	Locator::getLogger()->trace("LyricsLoader::LoadFromLRCFile done");

	return true;
}
