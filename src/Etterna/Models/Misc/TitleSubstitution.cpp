#include "Etterna/Globals/global.h"
#include "TitleSubstitution.h"
#include "Etterna/Models/Fonts/FontCharAliases.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"

static const std::string TRANSLATIONS_PATH = "Data/Translations.xml";
static const std::string ERASE_MARKER = "-erase-";

struct TitleTrans
{
	Regex TitleFrom, SubFrom, ArtistFrom;
	TitleFields Replacement;

	/* If this is true, no translit fields will be generated automatically. */
	bool translit;
	TitleTrans() { translit = true; }

	TitleTrans(const TitleFields& tf, bool translit_)
	  : Replacement(tf)
	  , translit(translit_)
	{
	}

	bool Matches(const TitleFields& tf, TitleFields& to);

	void LoadFromNode(const XNode* pNode);
};

bool
TitleTrans::Matches(const TitleFields& from, TitleFields& to)
{
	if (!TitleFrom.Replace(Replacement.Title, from.Title, to.Title))
		return false; /* no match */
	if (!SubFrom.Replace(Replacement.Subtitle, from.Subtitle, to.Subtitle))
		return false; /* no match */
	if (!ArtistFrom.Replace(Replacement.Artist, from.Artist, to.Artist))
		return false; /* no match */

	return true;
}

void
TitleTrans::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "Translation");

	FOREACH_CONST_Attr(pNode, attr)
	{
		/* Surround each regex with ^(...)$, to force all comparisons to default
		 * to being a full-line match.  (Add ".*" manually if this isn't
		 * wanted.) */
		const auto& sKeyName = attr->first;
		const auto sValue = attr->second->GetValue<std::string>();
		if (sKeyName == "DontTransliterate")
			translit = false;
		else if (sKeyName == "TitleFrom")
			TitleFrom = std::string("^(" + sValue + ")$");
		else if (sKeyName == "ArtistFrom")
			ArtistFrom = std::string("^(" + sValue + ")$");
		else if (sKeyName == "SubtitleFrom")
			SubFrom = std::string("^(" + sValue + ")$");
		else if (sKeyName == "TitleTo")
			Replacement.Title = sValue;
		else if (sKeyName == "ArtistTo")
			Replacement.Artist = sValue;
		else if (sKeyName == "SubtitleTo")
			Replacement.Subtitle = sValue;
		else if (sKeyName == "TitleTransTo")
			Replacement.TitleTranslit = sValue;
		else if (sKeyName == "ArtistTransTo")
			Replacement.ArtistTranslit = sValue;
		else if (sKeyName == "SubtitleTransTo")
			Replacement.SubtitleTranslit = sValue;
		else
			LuaHelpers::ReportScriptErrorFmt("Unknown TitleSubst tag: \"%s\"",
											 sKeyName.c_str());
	}
}

void
TitleSubst::AddTrans(const TitleTrans& tr)
{
	ASSERT(tr.TitleFrom.IsSet() || tr.SubFrom.IsSet() || tr.ArtistFrom.IsSet());
	ttab.push_back(new TitleTrans(tr));
}

void
TitleSubst::Subst(TitleFields& tf)
{
	for (auto& iter : ttab) {
		auto tt = iter;

		TitleFields to;
		if (!tt->Matches(tf, to))
			continue;

		/* The song matches.  Replace whichever strings aren't empty. */
		if (!tt->Replacement.Title.empty() &&
			tf.Title != tt->Replacement.Title) {
			if (tt->translit)
				tf.TitleTranslit = tf.Title;
			tf.Title = (tt->Replacement.Title != ERASE_MARKER) ? to.Title
															   : std::string();
			FontCharAliases::ReplaceMarkers(tf.Title);
		}
		if (!tt->Replacement.Subtitle.empty() &&
			tf.Subtitle != tt->Replacement.Subtitle) {
			if (tt->translit)
				tf.SubtitleTranslit = tf.Subtitle;
			tf.Subtitle = (tt->Replacement.Subtitle != ERASE_MARKER)
							? to.Subtitle
							: std::string();
			FontCharAliases::ReplaceMarkers(tf.Subtitle);
		}
		if (!tt->Replacement.Artist.empty() &&
			tf.Artist != tt->Replacement.Artist) {
			if (tt->translit)
				tf.ArtistTranslit = tf.Artist;
			tf.Artist = (tt->Replacement.Artist != ERASE_MARKER)
						  ? to.Artist
						  : std::string();
			FontCharAliases::ReplaceMarkers(tf.Artist);
		}

		/* These are used when applying kanji to a field that doesn't have the
		 * correct data.  Should be used sparingly. */
		if (!tt->Replacement.TitleTranslit.empty()) {
			tf.TitleTranslit = (tt->Replacement.TitleTranslit != ERASE_MARKER)
								 ? tt->Replacement.TitleTranslit
								 : std::string();
			FontCharAliases::ReplaceMarkers(tf.TitleTranslit);
		}
		if (!tt->Replacement.SubtitleTranslit.empty()) {
			tf.SubtitleTranslit =
			  (tt->Replacement.SubtitleTranslit != ERASE_MARKER)
				? tt->Replacement.SubtitleTranslit
				: std::string();
			FontCharAliases::ReplaceMarkers(tf.SubtitleTranslit);
		}
		if (!tt->Replacement.ArtistTranslit.empty()) {
			tf.ArtistTranslit = (tt->Replacement.ArtistTranslit != ERASE_MARKER)
								  ? tt->Replacement.ArtistTranslit
								  : std::string();
			FontCharAliases::ReplaceMarkers(tf.ArtistTranslit);
		}

		// Matched once.  Keep processing to allow multiple matching entries.
		// For example, allow one entry to translate a title, and another entry
		// to translate the artist.
	}
}

TitleSubst::TitleSubst(const std::string& section)
{
	Load(TRANSLATIONS_PATH, section);
}

void
TitleSubst::Load(const std::string& filename, const std::string& section)
{
	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, filename)) {
		// LoadFromFile will show its own error
		// LOG->Trace("Error opening %s: %s", filename.c_str(),
		// f.GetError().c_str() );
		return;
	}

	auto pGroup = xml.GetChild(section);
	if (pGroup == nullptr)
		return;
	FOREACH_CONST_Child(pGroup, child)
	{
		if (child->GetName() != "Translation")
			continue;

		TitleTrans tr;
		tr.LoadFromNode(child);
		AddTrans(tr);
	}
}

TitleSubst::~TitleSubst()
{
	for (auto& i : ttab)
		delete i;
}
