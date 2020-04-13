// MinaCalc.cpp : Defines the exported functions for the DLL application.
//

#include "MinaCalc.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <mutex>
#include <cmath>
#include <fstream>

using namespace std;

#define SAFE_DELETE(p)                                                         \
	{                                                                          \
		delete p;                                                              \
		p = NULL;                                                              \
	}

template<typename T, typename U>
inline U
lerp(T x, U l, U h)
{
	return static_cast<U>(x * (h - l) + l);
}

template<typename T, typename U, typename V>
inline void
CalcClamp(T& x, U l, V h)
{
	if (x > static_cast<T>(h))
		x = static_cast<T>(h);
	else if (x < static_cast<T>(l))
		x = static_cast<T>(l);
}

inline float
mean(vector<float>& v)
{
	float sum = 0.f;
	for (size_t i = 0; i < v.size(); ++i)
		sum += v[i];

	return sum / v.size();
}

// Coefficient of variance
inline float
cv(vector<float>& v)
{
	float sum = 0.f;
	float mean;
	float sd = 0.f;

	for (size_t i = 0; i < v.size(); i++)
		sum += v[i];

	mean = sum / v.size();
	for (size_t i = 0; i < v.size(); i++)
		sd += pow(v[i] - mean, 2);

	return sqrt(sd / v.size()) / mean;
}

inline float
downscalebaddies(float& f, float sg)
{
	CalcClamp(f, 0.f, 100.f);
	if (sg >= 0.93f)
		return f;
	float o = f * 1 - sqrt(0.93f - sg);
	CalcClamp(f, 0.f, 100.f);
	return o;
}

// Specifically for pattern modifiers as the neutral value is 1
inline void
PatternSmooth(vector<float>& v)
{
	float f1 = 1.f;
	float f2 = 1.f;
	float f3 = 1.f;
	float total = 3.f;

	for (size_t i = 0; i < v.size(); i++) {
		total -= f1;
		f1 = f2;
		f2 = f3;
		f3 = v[i];
		total += f3;
		v[i] = (f1 + f2 + f3) / 3;
	}
}

inline void
DifficultySmooth(vector<float>& v)
{
	float f1 = 0.f;
	float f2 = 0.f;
	float f3 = 0.f;
	float total = 0.f;

	for (size_t i = 0; i < v.size(); i++) {
		total -= f1;
		f1 = f2;
		f2 = f3;
		f3 = v[i];
		total += f3;
		v[i] = (f1 + f2 + f3) / 3;
	}
}

inline void
DifficultyMSSmooth(vector<float>& v)
{
	float f1 = 0.f;
	float f2 = 0.f;

	for (size_t i = 0; i < v.size(); i++) {
		f1 = f2;
		f2 = v[i];
		v[i] = (f1 + f2) / 2.f;
	}
}

inline float
AggregateScores(vector<float>& invector, float rating, float res, int iter)
{
	float sum;
	do {
		rating += res;
		sum = 0.0f;
		for (int i = 0; i < static_cast<int>(invector.size()); i++) {
			sum += 2.f / erfc(0.5f * (invector[i] - rating)) - 1.f;
		}
	} while (3 < sum);
	if (iter == 11)
		return rating;
	return AggregateScores(invector, rating - res, res / 2.f, iter + 1);
}

float
normalizer(float x, float y, float z1, float z2)
{
	float norm = ((x / y) - 1.f) * z1;
	CalcClamp(norm, 0.f, 1.f);
	float o = x * z2 * norm + x * (1.f - z2);
	return o;
}

float
jumpprop(const vector<NoteInfo>& NoteInfo)
{
	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	int taps = 0;
	int jamps = 0;

	for (size_t r = 0; r < NoteInfo.size(); r++) {
		int notes = (NoteInfo[r].notes & left ? 1 : 0) +
					(NoteInfo[r].notes & down ? 1 : 0) +
					(NoteInfo[r].notes & up ? 1 : 0) +
					(NoteInfo[r].notes & right ? 1 : 0);
		taps += notes;
		if (notes == 2)
			jamps += notes;
	}

	return static_cast<float>(jamps) / static_cast<float>(taps);
}

float
handprop(const vector<NoteInfo>& NoteInfo)
{
	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	int taps = 0;
	int hands = 0;

	for (size_t r = 0; r < NoteInfo.size(); r++) {
		int notes = (NoteInfo[r].notes & left ? 1 : 0) +
					(NoteInfo[r].notes & down ? 1 : 0) +
					(NoteInfo[r].notes & up ? 1 : 0) +
					(NoteInfo[r].notes & right ? 1 : 0);
		taps += notes;
		if (notes == 3)
			hands += notes;
	}

	return static_cast<float>(hands) / static_cast<float>(taps);
}

float
notcj(const vector<NoteInfo>& NoteInfo)
{
	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	int taps = 0;
	int hands = 0;
	int lcol = 0;
	int lnotes = 0;
	for (size_t r = 0; r < NoteInfo.size(); r++) {
		int lefty = NoteInfo[r].notes & left ? 1 : 0;
		int downy = NoteInfo[r].notes & down ? 1 : 0;
		int upy = NoteInfo[r].notes & up ? 1 : 0;
		int righty = NoteInfo[r].notes & right ? 1 : 0;

		int notes = lefty + downy + upy + righty;
		int tcol = 0;
		if (notes == 1) {
			if (lefty)
				tcol = left;
			if (downy)
				tcol = down;
			if (upy)
				tcol = up;
			if (righty)
				tcol = right;

			if (tcol != lcol && (lnotes == 3 || lnotes == 2))
				taps += 1;
		}

		if ((notes == 3 || notes == 2) && lnotes == 1) {
			if (lefty && lcol == left)
				taps -= 1;
			if (downy && lcol == down)
				taps -= 1;
			if (upy && lcol == up)
				taps -= 1;
			if (righty && lcol == right)
				taps -= 1;
			taps += 1;
		}

		taps += notes;
		if (notes == 3)
			hands += notes;

		lnotes = notes;
		if (lefty)
			lcol = left;
		if (downy)
			lcol = down;
		if (upy)
			lcol = up;
		if (righty)
			lcol = right;
	}

	return static_cast<float>(hands) / static_cast<float>(taps);
}

float
quadprop(const vector<NoteInfo>& NoteInfo)
{
	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	int taps = 0;
	int quads = 0;

	for (size_t r = 0; r < NoteInfo.size(); r++) {
		int notes = (NoteInfo[r].notes & left ? 1 : 0) +
					(NoteInfo[r].notes & down ? 1 : 0) +
					(NoteInfo[r].notes & up ? 1 : 0) +
					(NoteInfo[r].notes & right ? 1 : 0);
		taps += notes;
		if (notes == 4)
			quads += notes;
	}

	return static_cast<float>(quads) / static_cast<float>(taps);
}

vector<float>
Calc::CalcMain(const vector<NoteInfo>& NoteInfo,
			   float timingscale,
			   float score_goal)
{
	// LOG->Trace("%f", etaner.back());
	float grindscaler =
	  0.93f + (0.07f * (NoteInfo.back().rowTime - 30.f) / 30.f);
	CalcClamp(grindscaler, 0.93f, 1.f);

	float grindscaler2 =
	  0.873f + (0.13f * (NoteInfo.back().rowTime - 15.f) / 15.f);
	CalcClamp(grindscaler2, 0.87f, 1.f);

	float shortstamdownscaler =
	  0.9f + (0.1f * (NoteInfo.back().rowTime - 150.f) / 150.f);
	CalcClamp(shortstamdownscaler, 0.9f, 1.f);

	float jprop = jumpprop(NoteInfo);
	float nojumpsdownscaler = 0.8f + (0.2f * (jprop + 0.5f));
	CalcClamp(nojumpsdownscaler, 0.8f, 1.f);

	float hprop = handprop(NoteInfo);

	float nohandsdownscaler = 0.8f + (0.2f * (hprop + 0.75f));
	CalcClamp(nohandsdownscaler, 0.8f, 1.f);

	float allhandsdownscaler = 1.23f - hprop;
	CalcClamp(allhandsdownscaler, 0.85f, 1.f);

	float manyjampdownscaler = 1.43f - jprop;
	CalcClamp(manyjampdownscaler, 0.85f, 1.f);

	float qprop = quadprop(NoteInfo);
	float lotquaddownscaler = 1.13f - qprop;
	CalcClamp(lotquaddownscaler, 0.85f, 1.f);

	float jumpthrill = 1.625f - jprop - hprop;
	CalcClamp(jumpthrill, 0.85f, 1.f);

	vector<float> o;
	o.reserve(8);

	InitializeHands(NoteInfo, timingscale);
	TotalMaxPoints();
	float stream =
	  Chisel(0.1f, 10.24f, 1, false, false, true, false, false);
	float js = Chisel(0.1f, 10.24f, 1, false, false, true, true, false);
	float hs = Chisel(0.1f, 10.24f, 1, false, false, true, false, true);
	float tech =
	  Chisel(0.1f, 10.24f, 1, false, false, false, false, false);
	float jack =
	  Chisel(0.1f, 10.24f, 1, false, true, true, false, false);
	float jackstam =
	  jack;

	float techbase = max(stream, jack);
	float techorig = tech;
	tech = (tech / techbase) * tech;
	CalcClamp(tech, techorig * 0.85f, techorig);

	float stam = 0.f;
	if (stream > tech || js > tech || hs > tech)
		if (stream > js && stream > hs)
			stam = Chisel(
			  stream - 0.1f, 2.56f, 1, true, false, true, false, false);
		else if (js > hs)
			stam = Chisel(
			  js - 0.1f, 2.56f, 1, true, false, true, true, false);
		else
			stam = Chisel(
			  hs - 0.1f, 2.56f, 1, true, false, true, false, true);
	else
		stam = Chisel(
		  tech - 0.1f, 2.56f, 1, true, false, false, false, false);

	o.emplace_back(0.f); // temp
	o.emplace_back(downscalebaddies(stream, Scoregoal));

	js = normalizer(js, stream, 7.25f, 0.25f);
	o.emplace_back(downscalebaddies(js, Scoregoal));
	hs = normalizer(hs, stream, 6.5f, 0.3f);
	hs = normalizer(hs, js, 11.5f, 0.15f);
	o.emplace_back(downscalebaddies(hs, Scoregoal));

	float stambase = max(max(stream, tech * 0.96f), max(js, hs));
	if (stambase == stream)
		stambase *= 0.975f;

	stam = normalizer(stam, stambase, 7.75f, 0.2f);
	o.emplace_back(downscalebaddies(stam, Scoregoal));

	o.emplace_back(downscalebaddies(jack, Scoregoal));
	jackstam = normalizer(jackstam, jack, 5.5f, 0.25f);
	o.emplace_back(downscalebaddies(jackstam, Scoregoal));
	float technorm = max(max(stream, js), hs);
	tech = normalizer(tech, technorm, 8.f, .15f) * techscaler;
	o.emplace_back(downscalebaddies(tech, Scoregoal));

	float definitelycj = qprop + hprop + jprop + 0.2f;
	CalcClamp(definitelycj, 0.5f, 1.f);

	// chordjack
	float cj = o[3];

	o[1] *= allhandsdownscaler * manyjampdownscaler * lotquaddownscaler;
	o[2] *= nojumpsdownscaler * allhandsdownscaler * lotquaddownscaler;
	o[3] *= nohandsdownscaler * allhandsdownscaler * 1.015f *
			manyjampdownscaler * lotquaddownscaler;
	o[4] *= shortstamdownscaler * 0.985f * lotquaddownscaler;

	cj = normalizer(cj, o[3], 5.5f, 0.3f) * definitelycj * 1.025f;

	bool iscj = cj > o[5];
	if (iscj)
		o[6] = cj;

	o[7] *= allhandsdownscaler * manyjampdownscaler * lotquaddownscaler * 1.01f;

	float stamclamp = max(max(o[1], o[5]), max(o[2], o[3]));
	CalcClamp(o[4], 1.f, stamclamp * 1.1f);

	dumbvalue = (dumbvalue / static_cast<float>(dumbcounter));
	float stupidvalue = 1.f - (dumbvalue - 2.55f);
	CalcClamp(stupidvalue, 0.85f, 1.f);
	o[7] *= stupidvalue;

	if (stupidvalue <= 0.95f) {
		o[5] *= 1.f + (1.f - sqrt(stupidvalue));
	}

	float skadoot = max(o[3], o[2]);
	if (o[1] < skadoot)
		o[1] -= sqrt(skadoot - o[1]);

	float overall = AggregateScores(o, 0.f, 10.24f, 1);
	o[0] = downscalebaddies(overall, Scoregoal);

	float aDvg = mean(o) * 1.2f;
	for (size_t i = 0; i < o.size(); i++) {
		if (i == 1 || i == 2 || i == 7) {
			CalcClamp(o[i], 0.f, aDvg * 1.0416f);
			o[i] *= grindscaler * grindscaler2;
		} else {
			CalcClamp(o[i], 0.f, aDvg);
			o[i] *= grindscaler * grindscaler2;
		}
		o[i] = downscalebaddies(o[i], Scoregoal);
	}

	o[2] *= jumpthrill;
	o[3] *= jumpthrill;
	o[4] *= sqrt(jumpthrill) * 0.996f;
	o[7] *= sqrt(jumpthrill);

	float highest = 0.f;
	for (auto v : o) {
		if (v > highest)
			highest = v;
	}
	o[0] = AggregateScores(o, 0.f, 10.24f, 1);
	;

	float dating = 0.5f + (highest / 100.f);
	CalcClamp(dating, 0.f, 0.9f);

	if (Scoregoal < dating) {
		for (size_t i = 0; i < o.size(); i++) {
			o[i] = 0.f;
		}
	}

	o[5] *= 1.0075f;

	float hsnottech = o[7] - o[3];
	float jsnottech = o[7] - o[2];

	if (highest == o[7]) {
		hsnottech = 4.5f - hsnottech;
		CalcClamp(hsnottech, 0.f, 4.5f);
		o[7] -= hsnottech;

		jsnottech = 4.5f - jsnottech;
		CalcClamp(jsnottech, 0.f, 4.5f);
		o[7] -= jsnottech;
	}

	o[7] *= 1.025f;
	if (!iscj)
		o[6] *= 0.9f;

	highest = 0.f;
	o[0] = 0.f;
	for (auto v : o) {
		if (v > highest)
			highest = v;
	}
	o[0] = highest;

	if (debugMod == CalcPatternMod::MSD) {	// will need case switches later depending on which was highest skillset
		float ihatelifekillmeplease = Chisel(0.1f, 10.24f, score_goal, false, false, true, false, false);
		left_hand.debug = left_hand.finalMSDvals;
		right_hand.debug = right_hand.finalMSDvals;
	} else if (debugMod == CalcPatternMod::PtLoss) {
		left_hand.debug = left_hand.pointslost;
		right_hand.debug = right_hand.pointslost;
	}

	return o;
}

// ugly jack stuff
vector<float>
Calc::JackStamAdjust(vector<float>& j, float x, bool jackstam)
{
	vector<float> o(j.size());
	float floor = 1.f;
	float mod = 1.f;
	float ceil = 1.15f;
	float fscale = 1750.f;
	float prop = 0.75f;
	float mag = 250.f;
	float multstam = 1.f;
	if (jackstam) {
		multstam = 1.f;
		prop = 0.55f;
		ceil = 1.5f;
		fscale = 1550.f;
		mag = 750.f;
	}

	for (size_t i = 0; i < j.size(); i++) {
		mod += ((j[i] * multstam / (prop * x)) - 1) / mag;
		if (mod > 1.f)
			floor += (mod - 1) / fscale;
		CalcClamp(mod, 1.f, ceil * sqrt(floor));
		o[i] = j[i] * mod;
	}
	return o;
}

float
Calc::JackLoss(vector<float>& j, float x, bool jackstam)
{
	const vector<float>& v = JackStamAdjust(j, x, jackstam);
	float o = 0.f;
	for (size_t i = 0; i < v.size(); i++) {
		if (x < v[i])
			o += 7.f - (7.f * pow(x / (v[i] * 0.96f), 1.5f));
	}
	CalcClamp(o, 0.f, 10000.f);
	return o;
}

JackSeq
Calc::SequenceJack(const vector<NoteInfo>& NoteInfo, int t)
{
	vector<float> o;
	float last = -5.f;
	float mats1 = 0.f;
	float mats2 = 0.f;
	float mats3 = 0.f;
	float timestamp = 0.f;
	int track = 1 << t;

	for (size_t i = 0; i < NoteInfo.size(); i++) {
		float scaledtime = NoteInfo[i].rowTime / MusicRate;
		if (NoteInfo[i].notes & track) {
			mats1 = mats2;
			mats2 = mats3;
			mats3 = 1000.f * (scaledtime - last);
			last = scaledtime;
			timestamp = (mats1 + mats2 + mats3) / 3.f;

			CalcClamp(timestamp, 25.f, mats3 * 1.4f);
			float tmp = 1 / timestamp * 2800.f;
			CalcClamp(tmp, 0.f, 50.f);
			o.emplace_back(tmp);
		}
	}
	return o;
}

int
Calc::fastwalk(const vector<NoteInfo>& NoteInfo)
{
	int Interval = 0;
	for (size_t i = 0; i < NoteInfo.size(); i++) {
		if (NoteInfo[i].rowTime / MusicRate >= Interval * IntervalSpan)
			++Interval;
	}
	return Interval;
}

void
Calc::InitializeHands(const vector<NoteInfo>& NoteInfo, float ts)
{
	numitv = fastwalk(NoteInfo);

	ProcessedFingers l;
	ProcessedFingers r;
	for (int i = 0; i < 4; i++) {
		if (i <= numTracks / 2 - 1)
			l.emplace_back(ProcessFinger(NoteInfo, i));
		else
			r.emplace_back(ProcessFinger(NoteInfo, i));
	}

	left_hand.InitHand(l[0], l[1], ts);
	left_hand.ohjumpscale = OHJumpDownscaler(NoteInfo, 0, 1);
	left_hand.anchorscale = Anchorscaler(NoteInfo, 0, 1);
	left_hand.rollscale = RollDownscaler(l[0], l[1]);
	left_hand.hsscale = HSDownscaler(NoteInfo);
	left_hand.jumpscale = JumpDownscaler(NoteInfo);

	right_hand.InitHand(r[0], r[1], ts);
	right_hand.ohjumpscale = OHJumpDownscaler(NoteInfo, 2, 3);
	right_hand.anchorscale = Anchorscaler(NoteInfo, 2, 3);
	right_hand.rollscale = RollDownscaler(r[0], r[1]);
	right_hand.hsscale = left_hand.hsscale;
	right_hand.jumpscale = left_hand.jumpscale;

	if (debugMod == CalcPatternMod::Jump) {
		left_hand.debug = left_hand.jumpscale;
		right_hand.debug = right_hand.jumpscale;
	} else if (debugMod == CalcPatternMod::Anchor) {
		left_hand.debug = left_hand.anchorscale;
		right_hand.debug = right_hand.anchorscale;
	} else if (debugMod == CalcPatternMod::HS) {
		left_hand.debug = left_hand.hsscale;
		right_hand.debug = right_hand.hsscale;
	} else if (debugMod == CalcPatternMod::OHJump) {
		left_hand.debug = left_hand.ohjumpscale;
		right_hand.debug = right_hand.ohjumpscale;
	} else if (debugMod == CalcPatternMod::Roll) {
		left_hand.debug = left_hand.rollscale;
		right_hand.debug = right_hand.rollscale;
	}

	j0 = SequenceJack(NoteInfo, 0);
	j1 = SequenceJack(NoteInfo, 1);
	j2 = SequenceJack(NoteInfo, 2);
	j3 = SequenceJack(NoteInfo, 3);

	vector<Finger> ltmp;
	vector<Finger> rtmp;
	l.swap(ltmp);
	r.swap(rtmp);

	l.shrink_to_fit();
	r.shrink_to_fit();
}

Finger
Calc::ProcessFinger(const vector<NoteInfo>& NoteInfo, int t)
{
	int Interval = 1;
	float last = -5.f;
	Finger AllIntervals(numitv);
	vector<float> CurrentInterval;
	float Timestamp;
	vector<int> itvnervtmp;
	vector<vector<int>> itvnerv(numitv);

	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	int column = 1 << t;
	for (size_t i = 0; i < NoteInfo.size(); i++) {
		float scaledtime = NoteInfo[i].rowTime / MusicRate;

		if (scaledtime >= Interval * IntervalSpan) {
			AllIntervals[Interval - 1] = CurrentInterval;
			CurrentInterval.clear();

			itvnerv[Interval - 1] = itvnervtmp;
			itvnervtmp.clear();
			++Interval;
		}

		if (NoteInfo[i].notes & column) {
			Timestamp = 1000 * (scaledtime - last);
			last = scaledtime;
			CalcClamp(Timestamp, 40.f, 5000.f);
			CurrentInterval.emplace_back(Timestamp);
		}

		if (t == 0 && (NoteInfo[i].notes & left || NoteInfo[i].notes & down ||
					   NoteInfo[i].notes & up || NoteInfo[i].notes & right)) {
			itvnervtmp.emplace_back(i);
		}
	}

	if (t == 0)
		nervIntervals = itvnerv;
	return AllIntervals;
}

void
Calc::TotalMaxPoints()
{
	for (size_t i = 0; i < left_hand.v_itvpoints.size(); i++)
		MaxPoints +=
		  static_cast<int>(left_hand.v_itvpoints[i] + right_hand.v_itvpoints[i]);
}

float
Calc::Chisel(float pskill,
			 float res,
			 int iter,
			 bool stam,
			 bool jack,
			 bool nps,
			 bool js,
			 bool hs)
{
	float gotpoints = 0.f;
	do {
		if (pskill > 100.f)
			return pskill;
		pskill += res;
		if (jack) {
			{
				gotpoints = MaxPoints;
				gotpoints -=
				  JackLoss(j0, pskill, false) + JackLoss(j1, pskill, false) +
				  JackLoss(j2, pskill, false) + JackLoss(j3, pskill, false);
			}
		} else
			gotpoints = left_hand.CalcInternal(pskill, stam, nps, js, hs) +
						right_hand.CalcInternal(pskill, stam, nps, js, hs);

	} while (gotpoints / MaxPoints < Scoregoal);
	if (iter == 7)
		return pskill;
	return Chisel(
	  pskill - res, res / 2.f, iter + 1, stam, jack, nps, js, hs);
}

// Hand stuff
void
Hand::InitHand(Finger& f1, Finger& f2, float ts)
{
	SetTimingScale(ts);
	InitDiff(f1, f2);
	InitPoints(f1, f2);
}

float
Hand::CalcMSEstimate(vector<float>& v)
{
	if (v.empty())
		return 0.f;

	sort(v.begin(), v.end());
	float m = 0;
	v[0] *= 1.066f;
	size_t End = min(v.size(), static_cast<size_t>(6));
	for (size_t i = 0; i < End; i++)
		m += v[i];
	return 1 / (m / (End)) * 1375;
}

void
Hand::InitDiff(Finger& f1, Finger& f2)
{
	vector<float> tmpNPS(f1.size());
	vector<float> tmpMS(f1.size());

	for (size_t i = 0; i < f1.size(); i++) {
		float nps = 1.6f * static_cast<float>(f1[i].size() +
											  f2[i].size()); // intervalspan
		float aa = CalcMSEstimate(f1[i]);
		float bb = CalcMSEstimate(f2[i]);
		float ms = max(aa, bb);
		tmpNPS[i] = finalscaler * nps;
		tmpMS[i] =
		  finalscaler * (ms + ms + ms + ms + ms + nps + nps + nps + nps) / 9.f;
	}
	if (SmoothDifficulty)
		DifficultyMSSmooth(tmpMS);

	DifficultySmooth(tmpNPS);
	v_itvNPSdiff = tmpNPS;
	v_itvMSdiff = tmpMS;
}

void
Hand::InitPoints(Finger& f1, Finger& f2)
{
	for (size_t i = 0; i < f1.size(); i++)
		v_itvpoints.emplace_back(static_cast<int>(f1[i].size()) +
								 static_cast<int>(f2[i].size()));
}

vector<float>
Hand::StamAdjust(float x, vector<float> diff)
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
	vector<float> diff;

	if (nps)
		diff = v_itvNPSdiff;
	else
		diff = v_itvMSdiff;

	for (size_t i = 0; i < diff.size(); ++i) {
		if (hs)
			diff[i] = diff[i] * anchorscale[i] * sqrt(ohjumpscale[i]) *
					  rollscale[i] * jumpscale[i];
		else if (js)
			diff[i] = diff[i] * pow(hsscale[i], 2) * anchorscale[i] *
					  sqrt(ohjumpscale[i]) * rollscale[i] * jumpscale[i];
		else if (nps)
			diff[i] = diff[i] * pow(hsscale[i], 3) * anchorscale[i] *
					  pow(ohjumpscale[i], 2) * rollscale[i] *
					  pow(jumpscale[i], 2);
		else
			diff[i] =
			  diff[i] * anchorscale[i] * sqrt(ohjumpscale[i]) * rollscale[i];
	}

	const vector<float>& v = stam ? StamAdjust(x, diff) : diff;
	finalMSDvals = v; // bad bad bad bad bad bad bad bad bad bad
	float output = 0.f;
	std::vector<float> pointloss;
	for (size_t i = 0; i < v.size(); i++) {
		float gainedpoints = x > v[i]
							   ? v_itvpoints[i]
							   : v_itvpoints[i] * pow(x / v[i], 1.8f);

		output += gainedpoints;
		pointloss.push_back(v_itvpoints[i] - gainedpoints);
	}
	pointslost = pointloss; // to the bone
	return output;
}

// pattern modifiers
vector<float>
Calc::OHJumpDownscaler(const vector<NoteInfo>& NoteInfo, int t1, int t2)
{
	vector<float> o(nervIntervals.size());
	int firstNote = 1 << t1;
	int secondNote = 1 << t2;

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		if (nervIntervals[i].empty())
			o[i] = 1.f;
		else {
			int taps = 0;
			int jumptaps = 0;
			for (size_t r = 0; r < nervIntervals[i].size(); r++) {
				int row = nervIntervals[i][r];
				if (NoteInfo[row].notes & firstNote) {
					++taps;
					if (NoteInfo[row].notes & secondNote) {
						jumptaps += 2;
						++taps;
					}
				}
			}
			o[i] = taps != 0 ? pow(1 - (static_cast<float>(jumptaps) /
										static_cast<float>(taps) / 2.5f),
								   0.25f)
							 : 1.f;

			if (logpatterns)
				cout << "ohj " << o[i] << endl;
		}
	}

	if (SmoothPatterns)
		PatternSmooth(o);
	return o;
}

// pattern modifiers
vector<float>
Calc::Anchorscaler(const vector<NoteInfo>& NoteInfo, int t1, int t2)
{
	vector<float> o(nervIntervals.size());
	int firstNote = 1 << t1;
	int secondNote = 1 << t2;

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		if (nervIntervals[i].empty())
			o[i] = 1.f;
		else {
			int lcol = 0;
			int rcol = 0;
			for (size_t r = 0; r < nervIntervals[i].size(); r++) {
				int row = nervIntervals[i][r];
				if (NoteInfo[row].notes & firstNote)
					++lcol;
				if (NoteInfo[row].notes & secondNote)
					++rcol;
			}
			bool anyzero = lcol == 0 || rcol == 0;
			o[i] = anyzero
					 ? 1.f
					 : sqrt(1 - (static_cast<float>(min(lcol, rcol)) /
								 static_cast<float>(max(lcol, rcol)) / 4.45f));

			float stupidthing = (static_cast<float>(max(lcol, rcol)) + 2.f) /
								(static_cast<float>(min(lcol, rcol)) + 1.f);
			dumbvalue += stupidthing;
			++dumbcounter;

			CalcClamp(o[i], 0.8f, 1.05f);

			if (logpatterns)
				cout << "an " << o[i] << endl;
		}
	}

	if (SmoothPatterns)
		PatternSmooth(o);
	return o;
}

vector<float>
Calc::HSDownscaler(const vector<NoteInfo>& NoteInfo)
{
	vector<float> o(nervIntervals.size());
	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		if (nervIntervals[i].empty())
			o[i] = 1.f;
		else {
			int taps = 0;
			int handtaps = 0;
			for (size_t r = 0; r < nervIntervals[i].size(); r++) {
				int row = nervIntervals[i][r];
				int notes = (NoteInfo[row].notes & left ? 1 : 0) +
							(NoteInfo[row].notes & down ? 1 : 0) +
							(NoteInfo[row].notes & up ? 1 : 0) +
							(NoteInfo[row].notes & right ? 1 : 0);
				taps += notes;
				if (notes == 3)
					handtaps += notes;
			}
			o[i] = taps != 0 ? sqrt(sqrt(1 - (static_cast<float>(handtaps) /
											  static_cast<float>(taps) / 3.f)))
							 : 1.f;

			if (logpatterns)
				cout << "hs " << o[i] << endl;
		}
	}

	if (SmoothPatterns)
		PatternSmooth(o);
	return o;
}

vector<float>
Calc::JumpDownscaler(const vector<NoteInfo>& NoteInfo)
{
	vector<float> o(nervIntervals.size());
	int left = 1;
	int down = 1 << 1;
	int up = 1 << 2;
	int right = 1 << 3;

	for (size_t i = 0; i < nervIntervals.size(); i++) {
		if (nervIntervals[i].empty())
			o[i] = 1.f;
		else {
			int taps = 0;
			int jamps = 0;
			for (size_t r = 0; r < nervIntervals[i].size(); r++) {
				int row = nervIntervals[i][r];
				int notes = (NoteInfo[row].notes & left ? 1 : 0) +
							(NoteInfo[row].notes & down ? 1 : 0) +
							(NoteInfo[row].notes & up ? 1 : 0) +
							(NoteInfo[row].notes & right ? 1 : 0);
				taps += notes;
				if (notes == 2)
					jamps += notes;
			}
			o[i] = taps != 0 ? sqrt(sqrt(1 - (static_cast<float>(jamps) /
											  static_cast<float>(taps) / 6.f)))
							 : 1.f;

			if (logpatterns)
				cout << "ju " << o[i] << endl;
		}
	}
	if (SmoothPatterns)
		PatternSmooth(o);
	return o;
}

vector<float>
Calc::RollDownscaler(Finger f1, Finger f2)
{
	vector<float> o(f1.size());
	for (size_t i = 0; i < f1.size(); i++) {
		if (f1[i].empty() && f2[i].empty())
			o[i] = 1.f;
		else {
			vector<float> cvint;
			for (size_t ii = 0; ii < f1[i].size(); ii++)
				cvint.emplace_back(f1[i][ii]);
			for (size_t ii = 0; ii < f2[i].size(); ii++)
				cvint.emplace_back(f2[i][ii]);

			float mmm = mean(cvint);

			for (size_t i = 0; i < cvint.size(); ++i)
				cvint[i] = mmm / cvint[i] < 0.6f ? mmm : cvint[i];

			if (cvint.size() == 1) {
				o[i] = 1.f;
				continue;
			}

			float dacv = cv(cvint);
			if (dacv >= 0.15)
				o[i] = sqrt(sqrt(0.85f + dacv));
			else
				o[i] = pow(0.85f + dacv, 3);
			CalcClamp(o[i], 0.f, 1.075f);

			if (logpatterns)
				cout << "ro " << o[i] << endl;
		}
	}

	if (SmoothPatterns)
		PatternSmooth(o);

	return o;
}

void
Calc::Purge()
{
	vector<float> tmp1;
	vector<float> tmp2;
	vector<float> tmp3;
	vector<float> tmp4;

	j0.swap(tmp1);
	j1.swap(tmp2);
	j2.swap(tmp3);
	j3.swap(tmp4);

	j0.shrink_to_fit();
	j1.shrink_to_fit();
	j2.shrink_to_fit();
	j3.shrink_to_fit();

	vector<float> l1;
	vector<float> l2;
	vector<float> l3;
	vector<float> l4;
	vector<float> l5;

	left_hand.ohjumpscale.swap(l1);
	left_hand.anchorscale.swap(l2);
	left_hand.rollscale.swap(l3);
	left_hand.hsscale.swap(l4);
	left_hand.jumpscale.swap(l5);

	left_hand.ohjumpscale.shrink_to_fit();
	left_hand.anchorscale.shrink_to_fit();
	left_hand.rollscale.shrink_to_fit();
	left_hand.hsscale.shrink_to_fit();
	left_hand.jumpscale.shrink_to_fit();

	vector<float> r1;
	vector<float> r2;
	vector<float> r3;
	vector<float> r4;
	vector<float> r5;

	right_hand.ohjumpscale.swap(l1);
	right_hand.anchorscale.swap(l2);
	right_hand.rollscale.swap(l3);
	right_hand.hsscale.swap(l4);
	right_hand.jumpscale.swap(l5);

	right_hand.ohjumpscale.shrink_to_fit();
	right_hand.anchorscale.shrink_to_fit();
	right_hand.rollscale.shrink_to_fit();
	right_hand.hsscale.shrink_to_fit();
	right_hand.jumpscale.shrink_to_fit();
}

static const float ssrcap = 0.965f; // cap SSR at 96% so things don't get out of hand YES WE ACTUALLY NEED THIS FFS

// Function to generate SSR rating
vector<float>
MinaSDCalc(const vector<NoteInfo>& NoteInfo,
		   float musicrate,
		   float goal)
{
	vector<float> o;

	unique_ptr<Calc> doot = make_unique<Calc>();
	doot->MusicRate = musicrate;
	CalcClamp(
	  goal, 0.f, ssrcap);
	doot->Scoregoal = goal;
	o = doot->CalcMain(NoteInfo, musicrate, min(goal, ssrcap));

	doot->Purge();

	return o;
}

// Wrap difficulty calculation for all standard rates
MinaSD
MinaSDCalc(const vector<NoteInfo>& NoteInfo)
{

	MinaSD allrates;

	int rateCount = 21;

	if (!NoteInfo.empty()) {
		for (int i = 7; i < rateCount; i++) {
			auto tempVal =
				MinaSDCalc(NoteInfo, i / 10.f, 0.93f);
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
	return 263;
}
