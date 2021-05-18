#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "RageUtil/Graphics/Display/RageDisplay.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenTestSound.h"
#include "Etterna/Singletons/ThemeManager.h"

REGISTER_SCREEN_CLASS(ScreenTestSound);

void
ScreenTestSound::Init()
{
	Screen::Init();

	this->AddChild(&HEEEEEEEEELP);

	HEEEEEEEEELP.SetXY(450, 400);
	HEEEEEEEEELP.LoadFromFont(THEME->GetPathF("Common", "normal"));
	HEEEEEEEEELP.SetZoom(.5f);
	HEEEEEEEEELP.SetText("p  Play\n"
						 "s  Stop\n"
						 "l  Set looping\n"
						 "a  Set autostop\n"
						 "c  Set continue");

	for (int i = 0; i < nsounds; ++i) {
		this->AddChild(&s[i].txt);
		s[i].txt.LoadFromFont(THEME->GetPathF("Common", "normal"));
		s[i].txt.SetZoom(.5f);
	}

	s[0].txt.SetXY(150, 100);
	s[1].txt.SetXY(450, 100);
	s[2].txt.SetXY(150, 250);
	s[3].txt.SetXY(450, 250);
	s[4].txt.SetXY(150, 400);

	// howl says that "oga" is preferred over "ogg" as the extension;
	// i personally don't give a shit until I see what the vorbis group
	// has to say about it.
	s[0].s.Load("Themes/default/Sounds/_common menu music (loop).ogg");
	s[1].s.Load("Themes/default/Sounds/ScreenTitleMenu change.ogg");
	s[2].s.Load("Themes/default/Sounds/ScreenEvaluation try extra1.ogg");
	s[3].s.Load("Themes/default/Sounds/ScreenGameplay oni die.ogg");
	s[4].s.Load("Themes/default/Sounds/Common back.ogg");

	// s[0].s.SetStartSeconds(45);
	// s[0].s.SetPositionSeconds();
	// s[4].s.SetLengthSeconds(1);
	RageSoundParams p;
	p.StopMode = RageSoundParams::M_STOP;
	// p.m_fRate = 1.20f;
	for (int i = 0; i < nsounds; ++i)
		s[i].s.SetParams(p);

	// s[0].s.SetStopMode(RageSound::M_LOOP);
	// s[0].s.Play(false);

	selected = 0;
	for (int i = 0; i < nsounds; ++i)
		UpdateText(i);
}

ScreenTestSound::~ScreenTestSound()
{
	for (int i = 0; i < nsounds; ++i) {
		/* Delete copied sounds. */
		vector<RageSound*>& snds = m_sSoundCopies[i];
		for (unsigned j = 0; j < snds.size(); ++j)
			delete snds[j];
	}
}

void
ScreenTestSound::UpdateText(int n)
{
	std::string fn = Basename(s[n].s.GetLoadedFilePath());

	vector<RageSound*>& snds = m_sSoundCopies[n];

	std::string pos;
	for (unsigned p = 0; p < snds.size(); ++p) {
		if (p)
			pos += ", ";
		pos += ssprintf("%.3f", snds[p]->GetPositionSeconds());
	}

	s[n].txt.SetText(
	  ssprintf("%i: %s\n"
			   "%s\n"
			   "%s\n"
			   "(%s)\n"
			   "%s",
			   n + 1,
			   fn.c_str(),
			   s[n].s.IsPlaying() ? "Playing" : "Stopped",
			   s[n].s.GetParams().StopMode == RageSoundParams::M_STOP
				 ? "Stop when finished"
				 : s[n].s.GetParams().StopMode == RageSoundParams::M_CONTINUE
					 ? "Continue until stopped"
					 : "Loop",
			   pos.size() ? pos.c_str() : "none playing",
			   selected == n ? "^^^^^^" : ""));
}

void
ScreenTestSound::Update(float f)
{
	Screen::Update(f);

	for (int i = 0; i < nsounds; ++i) {
		UpdateText(i);

		/* Delete copied sounds that have finished playing. */
		vector<RageSound*>& snds = m_sSoundCopies[i];
		for (unsigned j = 0; j < snds.size(); ++j) {
			if (snds[j]->IsPlaying())
				continue;
			delete snds[j];
			snds.erase(snds.begin() + j);
			--j;
		}
	}
}

bool
ScreenTestSound::Input(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false; // ignore

	switch (input.DeviceI.device) {
		case DEVICE_KEYBOARD:
			switch (input.DeviceI.button) {
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
					selected = input.DeviceI.button - '0' - 1;
					break;
				case 'p': {
					/* We want to be able to read the position of copied sounds;
					 * if we let RageSound copy itself, then the copy will be
					 * owned by RageSoundManager and we won't be allowed to
					 * touch it.  Copy it ourself. */
					RageSound* pCopy = new RageSound(s[selected].s);
					m_sSoundCopies[selected].push_back(pCopy);
					pCopy->Play(false);
					break;
				}
				case 's': {
					for (int i = 0; i < nsounds; ++i) {
						/* Stop copied sounds. */
						vector<RageSound*>& snds = m_sSoundCopies[i];
						for (unsigned j = 0; j < snds.size(); ++j)
							snds[j]->Stop();
					}
					break;
				}
				case 'l': {
					RageSoundParams p = s[selected].s.GetParams();
					p.StopMode = RageSoundParams::M_LOOP;
					s[selected].s.SetParams(p);
					break;
				}
				case 'a': {
					RageSoundParams p = s[selected].s.GetParams();
					p.StopMode = RageSoundParams::M_STOP;
					s[selected].s.SetParams(p);
					break;
				}
				case 'c': {
					RageSoundParams p = s[selected].s.GetParams();
					p.StopMode = RageSoundParams::M_CONTINUE;
					s[selected].s.SetParams(p);
					break;
				}
					/*			case KEY_LEFT:
									obj.SetX(obj.GetX() - 10);
									break;
								case KEY_RIGHT:
									obj.SetX(obj.GetX() + 10);
									break;
								case KEY_UP:
									obj.SetY(obj.GetY() - 10);
									break;
								case KEY_DOWN:
									obj.SetY(obj.GetY() + 10);
									break;
					*/
				default:
					return false;
			}
		default:
			return false;
	}
	return true;
}
