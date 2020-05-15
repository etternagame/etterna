#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "Core/Services/Locator.hpp"

#include <algorithm>

InputQueue* INPUTQUEUE =
  nullptr; // global and accessible from anywhere in our program

const unsigned MAX_INPUT_QUEUE_LENGTH = 32;

InputQueue::InputQueue()
{
	FOREACH_ENUM(GameController, gc)
	m_aQueue[gc].resize(MAX_INPUT_QUEUE_LENGTH);
}

void
InputQueue::RememberInput(const InputEventPlus& iep)
{
	if (!iep.GameI.IsValid())
		return;

	int c = iep.GameI.controller;
	if (m_aQueue[c].size() >= MAX_INPUT_QUEUE_LENGTH) // full
		m_aQueue[c].erase(m_aQueue[c].begin(),
						  m_aQueue[c].begin() +
							(m_aQueue[c].size() - MAX_INPUT_QUEUE_LENGTH + 1));
	m_aQueue[c].push_back(iep);
}

bool
InputQueue::WasPressedRecently(
  GameController c,
  const GameButton button,
  const std::chrono::steady_clock::time_point& OldestTimeAllowed,
  InputEventPlus* pIEP)
{
	for (int queue_index = m_aQueue[c].size() - 1; queue_index >= 0;
		 queue_index--) // iterate newest to oldest
	{
		const InputEventPlus& iep = m_aQueue[c][queue_index];
		if (iep.DeviceI.ts < OldestTimeAllowed) // buttons are too old.  Stop
												// searching because we're not
												// going to find a match
			return false;

		if (iep.GameI.button != button)
			continue;

		if (pIEP != nullptr)
			*pIEP = iep;

		return true;
	}

	return false; // didn't find the button
}

void
InputQueue::ClearQueue(GameController c)
{
	m_aQueue[c].clear();
}

static const float g_fSimultaneousThreshold = 0.05f;

bool
InputQueueCode::EnteredCode(GameController controller) const
{
	if (controller == GameController_Invalid)
		return false;
	if (m_aPresses.empty())
		return false;

	// iterate newest to oldest
	int iSequenceIndex = m_aPresses.size() - 1; // count down
	const vector<InputEventPlus>& aQueue = INPUTQUEUE->GetQueue(controller);
	int iQueueIndex = aQueue.size() - 1;
	while (iQueueIndex >= 0) {
		/* If the buttons are too old, stop searching because we're not going to
		 * find a match. */
		std::chrono::duration<float> timeDelta =
		  std::chrono::steady_clock::now() - aQueue[iQueueIndex].DeviceI.ts;
		float inputDelta = timeDelta.count();
		if (m_fMaxSecondsBack != -1 && inputDelta > m_fMaxSecondsBack)
			return false;

		/* If the last press is an input type we're not interested in, skip it
		 * and look again. */
		const ButtonPress& Press = m_aPresses[iSequenceIndex];
		if (!Press.m_InputTypes[aQueue[iQueueIndex].type]) {
			--iQueueIndex;
			continue;
		}

		/* Search backwards for all of Press.m_aButtonsToPress pressed within
		 * g_fTapThreshold seconds with m_aButtonsToHold pressed.  Start looking
		 * at iQueueIndex. */

		bool bMatched = false;
		int iMinSearchIndexUsed = iQueueIndex;
		for (int b = 0; b < static_cast<int>(Press.m_aButtonsToPress.size());
			 ++b) {
			const InputEventPlus* pIEP = nullptr;
			int iQueueSearchIndex = iQueueIndex;
			for (; iQueueSearchIndex >= 0;
				 --iQueueSearchIndex) // iterate newest to oldest
			{
				const InputEventPlus& iep = aQueue[iQueueSearchIndex];

				std::chrono::duration<float> timeDelta =
				  aQueue[iQueueIndex].DeviceI.ts - iep.DeviceI.ts;
				float simulInputDelta = timeDelta.count();
				if (simulInputDelta >
					g_fSimultaneousThreshold) // buttons are too old.  Stop
											  // searching because we're not
											  // going to find a match
					break;

				if (!Press.m_InputTypes[iep.type])
					continue;

				if (iep.GameI.button == Press.m_aButtonsToPress[b]) {
					pIEP = &iep;
					break;
				}
			}
			if (pIEP == nullptr)
				break; // didn't find the button

			// Check that m_aButtonsToHold were being held when the buttons were
			// pressed.
			bool bAllHeldButtonsOK = true;
			for (auto i : Press.m_aButtonsToHold) {
				GameInput gi(controller, i);
				if (!INPUTMAPPER->IsBeingPressed(
					  gi, MultiPlayer_Invalid, &pIEP->InputList))
					bAllHeldButtonsOK = false;
			}
			for (auto i : Press.m_aButtonsToNotHold) {
				GameInput gi(controller, i);
				if (INPUTMAPPER->IsBeingPressed(
					  gi, MultiPlayer_Invalid, &pIEP->InputList))
					bAllHeldButtonsOK = false;
			}
			if (!bAllHeldButtonsOK)
				continue;
			iMinSearchIndexUsed =
			  std::min(iMinSearchIndexUsed, iQueueSearchIndex);
			if (b == static_cast<int>(Press.m_aButtonsToPress.size()) - 1)
				bMatched = true;
		}

		if (!bMatched) {
			/* The press wasn't matched.  If m_bAllowIntermediatePresses is
			 * true, skip the last press, and look again. */
			if (!Press.m_bAllowIntermediatePresses)
				return false;
			--iQueueIndex;
			continue;
		}

		if (iSequenceIndex == 0) {
			// we matched the whole pattern.  Empty the queue so we don't match
			// on it again.
			INPUTQUEUE->ClearQueue(controller);
			return true;
		}

		/* The press was matched. */
		iQueueIndex = iMinSearchIndexUsed - 1;
		--iSequenceIndex;
	}

	return false;
}

bool
InputQueueCode::Load(std::string sButtonsNames)
{
	m_aPresses.clear();

	vector<std::string> asPresses;
	split(sButtonsNames, ",", asPresses, false);
	for (auto& sPress : asPresses) {
		vector<std::string> asButtonNames;

		split(sPress, "-", asButtonNames, false);

		if (asButtonNames.empty()) {
			if (!sButtonsNames.empty())
				Locator::getLogger()->trace("Ignoring empty code \"{}\".", sButtonsNames.c_str());
			return false;
		}

		m_aPresses.push_back(ButtonPress());
		for (auto sButtonName : asButtonNames) // for each button in this code
		{
			bool bHold = false;
			bool bNotHold = false;
			for (;;) {
				if (sButtonName.substr(0, 1) == "+") {
					m_aPresses.back().m_InputTypes[IET_REPEAT] = true;
					sButtonName.erase(0, 1);
				} else if (sButtonName.substr(0, 1) == "~") {
					m_aPresses.back().m_InputTypes[IET_FIRST_PRESS] = false;
					m_aPresses.back().m_InputTypes[IET_RELEASE] = true;
					sButtonName.erase(0, 1);
				} else if (sButtonName.substr(0, 1) == "@") {
					sButtonName.erase(0, 1);
					bHold = true;
				} else if (sButtonName.substr(0, 1) == "!") {
					sButtonName.erase(0, 1);
					bNotHold = true;
				} else {
					break;
				}
			}

			// Search for the corresponding GameButton
			const GameButton gb =
			  INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(sButtonName);
			if (gb == GameButton_Invalid) {
				Locator::getLogger()->trace(
				  "The code \"{}\" contains an unrecognized button \"{}\".",
				  sButtonsNames.c_str(),
				  sButtonName.c_str());
				m_aPresses.clear();
				return false;
			}

			if (bHold)
				m_aPresses.back().m_aButtonsToHold.push_back(gb);
			else if (bNotHold)
				m_aPresses.back().m_aButtonsToNotHold.push_back(gb);
			else
				m_aPresses.back().m_aButtonsToPress.push_back(gb);
		}
	}

	if (m_aPresses.size() == 1)
		m_fMaxSecondsBack = 0.55f;
	else
		m_fMaxSecondsBack = (m_aPresses.size() - 1) * 0.6f;

	// if we make it here, we found all the buttons in the code
	return true;
}
