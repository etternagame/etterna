--[[ 
	Basically rewriting the c++ code to not be total shit so this can also not be total shit.
]]
	
local jcKeys = tableKeys(colorConfig:get_data().judgment)
local jcT = {}										-- A "T" following a variable name will designate an object of type table.

for i=1, #jcKeys do
	jcT[jcKeys[i]] = byJudgment(jcKeys[i])
end

local jdgT = {										-- Table of judgments for the judgecounter 
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo",
}

local dvCur																	
local jdgCur																-- Note: only for judgments with OFFSETS, might reorganize a bit later
local positive = getMainColor("positive")
local highlight = getMainColor("highlight")
local negative = getMainColor('negative')

-- We can also pull in some localized aliases for workhorse functions for a modest speed increase
local Round = notShit.round
local Floor = notShit.floor
local diffusealpha = Actor.diffusealpha
local diffuse = Actor.diffuse
local finishtweening = Actor.finishtweening
local linear = Actor.linear
local x = Actor.x
local queuecommand = Actor.queuecommand
local playcommand = Actor.queuecommand
local settext = BitmapText.settext
local Broadcast = MessageManager.Broadcast

-- Screenwide params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
isCentered = PREFSMAN:GetPreference("Center1Player")
local CenterX = SCREEN_CENTER_X
local mpOffset = 0
if not isCentered then
	CenterX = THEME:GetMetric("ScreenGameplay",string.format("PlayerP1%sX",ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStyleType())))
	mpOffset = SCREEN_CENTER_X
end
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

-- Those are the X and Y for things that are going to be able to be moved with the listener
local eb     -- Errorbar children
local dt     -- Differential tracker children
local mb     -- Minibar actor frame
local fb     -- Fullbar actor frame
local dp 	 -- Display percent actor frame

local screen 			-- the screen after it is loaded
local messageBox		-- the message box from when you try to move something
local judgeCounter      -- pa counter actor frame

--error bar things
local errorBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ErrorBarX 								
local errorBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ErrorBarY
local errorBarWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ErrorBarWidth         -- felt like this is necessary in order to do stuff
local errorBarHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ErrorBarHeight 								
local errorBarFrameWidth = capWideScale(get43size(errorBarWidth),errorBarWidth)
local wscale = errorBarFrameWidth/180

--percent display things
local displayPercentX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.DisplayPercentX
local displayPercentY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.DisplayPercentY
local displayPercentZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.DisplayPercentZoom

--pa counter things
local judgeCounterX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeCounterX
local judgeCounterY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeCounterY

--differential tracker things
local targetTrackerMode = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTrackerMode
local targetTrackerX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.TargetTrackerX
local targetTrackerY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.TargetTrackerY
local targetTrackerZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.TargetTrackerZoom

--mini progress bar things
local miniProgressBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.MiniProgressBarX
local miniProgressBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.MiniProgressBarY

--full progress bar things
local fullProgressBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.FullProgressBarX
local fullProgressBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.FullProgressBarY
local fullProgressBarWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.FullProgressBarWidth
local fullProgressBarHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.FullProgressBarHeight

--receptor/notefield things
local noteField
local noteColumns
local noteFieldX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldX
local noteFieldY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldY
local noteFieldWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldWidth
local noteFieldHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldHeight

--guess checking if things are enabled before changing them is good for not having a log full of errors
local enabledErrorBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ErrorBar
local enabledMiniBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).MiniProgressBar
local enabledFullBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).FullProgressBar
local enabledTargetTracker = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTracker
local enabledDisplayPercent = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).DisplayPercent
local enabledJudgeCounter = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter

-- restart button
local function froot(loop)
	if loop.DeviceInput.button == "DeviceButton_`" then
		SCREENMAN:GetTopScreen():SetPrevScreenName("ScreenStageInformation"):begin_backing_out()
	end
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
												**Main listener that moves and resizes things**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

]]
local onePressed = false
local twoPressed = false
local threePressed = false
local fourPressed = false
local fivePressed = false
local sixPressed = false
local sevenPressed = false
local eightPressed = false
local ninePressed = false
local zeroPressed = false
local qPressed = false
local wPressed = false
local ePressed = false
local rPressed = false
local tPressed = false
local yPressed = false
local uPressed = false
local iPressed = false
local oPressed = false
local pPressed = false
local changed = false

local function firstHalfInput(event)
	if getAutoplay() ~= 0 then
		-- this is starting to not look pretty, might rework on this piece of code to make it look smaller / lol i had to split in two functions because it told me there were more than 60 values
		if event.DeviceInput.button == "DeviceButton_1" then
			onePressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_2" then
			twoPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_3" then
			threePressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_4" then
			fourPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_5" then
			fivePressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_6" then
			sixPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_7" then
			sevenPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_8" then
			eightPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_9" then
			ninePressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_0" then
			zeroPressed = not (event.type == "InputEventType_Release")
		end
		messageBox:GetChild("judgmentPosText"):visible(onePressed):playcommand("Update")
		messageBox:GetChild("judgmentSizeText"):visible(twoPressed):playcommand("Update")
		messageBox:GetChild("comboPosText"):visible(threePressed):playcommand("Update")
		messageBox:GetChild("comboSizeText"):visible(fourPressed):playcommand("Update")
		messageBox:GetChild("errorBarPosText"):visible(fivePressed):playcommand("Update")
		messageBox:GetChild("errorBarSizeText"):visible(sixPressed):playcommand("Update")
		messageBox:GetChild("targetTrackerPosText"):visible(sevenPressed):playcommand("Update")
		messageBox:GetChild("targetTrackerSizeText"):visible(eightPressed):playcommand("Update")
		messageBox:GetChild("fullProgressBarPosText"):visible(ninePressed):playcommand("Update")
		messageBox:GetChild("fullProgressBarSizeText"):visible(zeroPressed):playcommand("Update")
		-- changes errorbar x/y
		if fivePressed and enabledErrorBar and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				errorBarY = errorBarY - 5
				eb.Center:y(errorBarY)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ErrorBarY = errorBarY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				errorBarY = errorBarY + 5
				eb.Center:y(errorBarY)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ErrorBarY = errorBarY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				errorBarX = errorBarX - 5
				eb.Center:x(errorBarX)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ErrorBarX = errorBarX
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				errorBarX = errorBarX + 5
				eb.Center:x(errorBarX)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ErrorBarX = errorBarX
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes errorbar size
		if sixPressed and enabledErrorBar and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				errorBarHeight = errorBarHeight + 1
				eb.Center:zoomtoheight(errorBarHeight)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ErrorBarHeight = errorBarHeight
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				errorBarHeight = errorBarHeight - 1
				eb.Center:zoomtoheight(errorBarHeight)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ErrorBarHeight = errorBarHeight
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				errorBarWidth = errorBarWidth - 10
				errorBarFrameWidth = capWideScale(get43size(errorBarWidth),errorBarWidth)
				wscale = errorBarFrameWidth/180
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ErrorBarWidth = errorBarWidth
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				errorBarWidth = errorBarWidth + 10
				errorBarFrameWidth = capWideScale(get43size(errorBarWidth),errorBarWidth)
				wscale = errorBarFrameWidth/180
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ErrorBarWidth = errorBarWidth
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes differential tracker x/y
		if sevenPressed and enabledTargetTracker and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				targetTrackerY = targetTrackerY - 5
				if targetTrackerMode == 0 then
					dt.PercentDifferential:y(targetTrackerY)
				else
					dt.PBDifferential:y(targetTrackerY)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.TargetTrackerY = targetTrackerY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				targetTrackerY = targetTrackerY + 5
				if targetTrackerMode == 0 then
					dt.PercentDifferential:y(targetTrackerY)
				else
					dt.PBDifferential:y(targetTrackerY)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.TargetTrackerY = targetTrackerY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				targetTrackerX = targetTrackerX - 5
				if targetTrackerMode == 0 then
					dt.PercentDifferential:x(targetTrackerX)
				else
					dt.PBDifferential:x(targetTrackerX)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.TargetTrackerX = targetTrackerX
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				targetTrackerX = targetTrackerX + 5
				if targetTrackerMode == 0 then
					dt.PercentDifferential:x(targetTrackerX)
				else
					dt.PBDifferential:x(targetTrackerX)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.TargetTrackerX = targetTrackerX
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes differential tracker size
		if eightPressed and enabledTargetTracker and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				targetTrackerZoom = targetTrackerZoom + 0.01
				if targetTrackerMode == 0 then
					dt.PercentDifferential:zoom(targetTrackerZoom)
				else
					dt.PBDifferential:zoom(targetTrackerZoom)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.TargetTrackerZoom = targetTrackerZoom
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				targetTrackerZoom = targetTrackerZoom - 0.01
				if targetTrackerMode == 0 then
					dt.PercentDifferential:zoom(targetTrackerZoom)
				else
					dt.PBDifferential:zoom(targetTrackerZoom)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.TargetTrackerZoom = targetTrackerZoom
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes full progress bar x/y
		if ninePressed and enabledFullBar and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				fullProgressBarY = fullProgressBarY - 3
				fb:y(fullProgressBarY)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.FullProgressBarY = fullProgressBarY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				fullProgressBarY = fullProgressBarY + 3
				fb:y(fullProgressBarY)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.FullProgressBarY = fullProgressBarY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				fullProgressBarX = fullProgressBarX - 5
				fb:x(fullProgressBarX)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.FullProgressBarX = fullProgressBarX
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				fullProgressBarX = fullProgressBarX + 5
				fb:x(fullProgressBarX)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.FullProgressBarX = fullProgressBarX
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes full progress bar width/height
		if zeroPressed and enabledFullBar and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				fullProgressBarHeight = fullProgressBarHeight + 0.1
				fb:zoomtoheight(fullProgressBarHeight)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.FullProgressBarHeight = fullProgressBarHeight
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				fullProgressBarHeight = fullProgressBarHeight - 0.1
				fb:zoomtoheight(fullProgressBarHeight)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.FullProgressBarHeight = fullProgressBarHeight
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				fullProgressBarWidth = fullProgressBarWidth - 0.01
				fb:zoomtowidth(fullProgressBarWidth)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.FullProgressBarWidth = fullProgressBarWidth
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				fullProgressBarWidth = fullProgressBarWidth + 0.01
				fb:zoomtowidth(fullProgressBarWidth)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.FullProgressBarWidth = fullProgressBarWidth
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		
	end
	return false
end

local function secondHalfInput(event)
	if getAutoplay() ~= 0 then
		if event.DeviceInput.button == "DeviceButton_q" then
			qPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_w" then
			wPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_e" then
			ePressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_r" then
			rPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_t" then
			tPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_y" then
			yPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_u" then
			uPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_i" then
			iPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_o" then
			oPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_p" then
			pPressed = not (event.type == "InputEventType_Release")
		end
		messageBox:GetChild("miniProgressBarPosText"):visible(qPressed):playcommand("Update")
		messageBox:GetChild("displayPercentPosText"):visible(wPressed):playcommand("Update")
		messageBox:GetChild("displayPercentSizeText"):visible(ePressed):playcommand("Update")
		messageBox:GetChild("noteFieldPosText"):visible(rPressed):playcommand("Update")
		messageBox:GetChild("noteFieldSizeText"):visible(tPressed):playcommand("Update")
		messageBox:GetChild("npsDisplayPosText"):visible(yPressed):playcommand("Update")
		messageBox:GetChild("npsDisplaySizeText"):visible(uPressed):playcommand("Update")
		messageBox:GetChild("npsGraphPosText"):visible(iPressed):playcommand("Update")
		messageBox:GetChild("npsGraphSizeText"):visible(oPressed):playcommand("Update")
		messageBox:GetChild("judgeCounterPosText"):visible(pPressed):playcommand("Update")
		-- changes mini progress bar x/y
		if qPressed and enabledMiniBar and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				miniProgressBarY = miniProgressBarY - 5
				mb:y(miniProgressBarY)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.MiniProgressBarY = miniProgressBarY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				miniProgressBarY = miniProgressBarY + 5
				mb:y(miniProgressBarY)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.MiniProgressBarY = miniProgressBarY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				miniProgressBarX = miniProgressBarX - 5
				mb:x(miniProgressBarX)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.MiniProgressBarX = miniProgressBarX
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				miniProgressBarX = miniProgressBarX + 5
				mb:x(miniProgressBarX)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.MiniProgressBarX = miniProgressBarX
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes display percent x/y
		if wPressed and enabledDisplayPercent and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				displayPercentY = displayPercentY - 5
				dp:addy(-5)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.DisplayPercentY = displayPercentY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				displayPercentY = displayPercentY + 5
				dp:addy(5)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.DisplayPercentY = displayPercentY
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				displayPercentX = displayPercentX - 5
				dp:addx(-5)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.DisplayPercentX = displayPercentX
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				displayPercentX = displayPercentX + 5
				dp:addx(5)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.DisplayPercentX = displayPercentX
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes display percent size
		if ePressed and enabledDisplayPercent and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				displayPercentZoom = displayPercentZoom + 0.01
				dp:zoom(displayPercentZoom)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.DisplayPercentZoom = displayPercentZoom
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				displayPercentZoom = displayPercentZoom - 0.01
				dp:zoom(displayPercentZoom)
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.DisplayPercentZoom = displayPercentZoom
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes the noteField/receptor x/y
		if rPressed and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				noteFieldY = noteFieldY - 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldY = noteFieldY
				noteField:addy(-3)
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				noteFieldY = noteFieldY + 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldY = noteFieldY
				noteField:addy(3)
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				noteFieldX = noteFieldX - 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldX = noteFieldX
				noteField:addx(-3)
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				noteFieldX = noteFieldX + 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldX = noteFieldX
				noteField:addx(3)
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes the noteField/receptor width/height
		if tPressed and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				noteFieldHeight = noteFieldHeight + 0.01
				for i, actor in ipairs(noteColumns) do
					actor:zoomtoheight(noteFieldHeight)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldHeight = noteFieldHeight
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				noteFieldHeight = noteFieldHeight - 0.01
				for i, actor in ipairs(noteColumns) do
					actor:zoomtoheight(noteFieldHeight)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldHeight = noteFieldHeight
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				noteFieldWidth = noteFieldWidth - 0.01
				for i, actor in ipairs(noteColumns) do
					actor:zoomtowidth(noteFieldWidth)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldWidth = noteFieldWidth
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				noteFieldWidth = noteFieldWidth + 0.01
				for i, actor in ipairs(noteColumns) do
					actor:zoomtowidth(noteFieldWidth)
				end
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldWidth = noteFieldWidth
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
		-- changes pa counter x/y
		if pPressed and enabledJudgeCounter and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" then
				judgeCounterY = judgeCounterY - 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeCounterY = judgeCounterY
				judgeCounter:addy(-3)
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_down" then
				judgeCounterY = judgeCounterY + 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeCounterY = judgeCounterY
				judgeCounter:addy(3)
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_left" then
				judgeCounterX = judgeCounterX - 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeCounterX = judgeCounterX
				judgeCounter:addx(-3)
				changed = true
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				judgeCounterX = judgeCounterX + 3
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeCounterX = judgeCounterX
				judgeCounter:addx(3)
				changed = true
			end
			if changed then
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				changed = false
			end
		end
	end
	return false
end


--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								     **Wife deviance tracker. Basically half the point of the theme.**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	For every doot there is an equal and opposite scoot.
]]

local t = Def.ActorFrame{										
	Name = "WifePerch",
	OnCommand=function()
		SCREENMAN:GetTopScreen():AddInputCallback(froot)
		if(playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay) then
			SCREENMAN:GetTopScreen():AddInputCallback(firstHalfInput)
			SCREENMAN:GetTopScreen():AddInputCallback(secondHalfInput)
		end
		screen = SCREENMAN:GetTopScreen()
		noteField = screen:GetChild("PlayerP1"):GetChild("NoteField")
		noteField:addx(noteFieldX)
		noteField:addy(noteFieldY)
		noteColumns = noteField:get_column_actors()
		for i, actor in ipairs(noteColumns) do
			actor:zoomtowidth(noteFieldWidth)
			actor:zoomtoheight(noteFieldHeight)
		end
	end,
	JudgmentMessageCommand=function(self, msg)
		if msg.Offset ~= nil then
			dvCur = msg.Offset 
			jdgCur = msg.Judgment
			Broadcast(MESSAGEMAN, "SpottedOffset")
		end
	end,
}

-- Stuff you probably shouldn't turn off, music rate string display
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_BOTTOM-10;zoom,0.35;settext,getCurRateDisplayString())}




--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
																	**LaneCover**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Old scwh lanecover back for now. Equivalent to "screencutting" on ffr; essentially hides notes for a fixed distance before they appear
on screen so you can adjust the time arrows display on screen without modifying their spacing from each other. 
]]	
	
t[#t+1] = LoadActor("lanecover")



	
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    	**Player Target Differential: Ghost target rewrite, average score gone for now**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Point differential to AA.
]]

-- Mostly clientside now. We set our desired target goal and listen to the results rather than calculating ourselves.
local target = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetGoal
GAMESTATE:GetPlayerState(PLAYER_1):SetTargetGoal(target/100)

-- We can save space by wrapping the personal best and set percent trackers into one function, however
-- this would make the actor needlessly cumbersome and unnecessarily punish those who don't use the
-- personal best tracker (although everything is efficient enough now it probably wouldn't matter)

-- moved it for better manipulation
local d = Def.ActorFrame{
	InitCommand = function(self)
		dt = self:GetChildren()
	end,
}

if targetTrackerMode == 0 then
	d[#d+1] = LoadFont("Common Normal")..{
		Name = "PercentDifferential",
		InitCommand=cmd(xy,targetTrackerX,targetTrackerY;zoom,targetTrackerZoom;halign,0;valign,1),
		JudgmentMessageCommand=function(self,msg)
			local tDiff = msg.WifeDifferential
			if tDiff >= 0 then 											
				diffuse(self,positive)
			else
				diffuse(self,negative)
			end
			self:settextf("%5.2f (%5.2f%%)", tDiff, target)
		end
	}
	else
	d[#d+1] = LoadFont("Common Normal")..{
		Name = "PBDifferential",
		InitCommand=cmd(xy,targetTrackerX,targetTrackerY;zoom,targetTrackerZoom;halign,0;valign,1),
		JudgmentMessageCommand=function(self,msg)
			local tDiff = msg.WifePBDifferential
			if tDiff then
				local pbtarget = msg.WifePBGoal
				if tDiff >= 0 then
					diffuse(self,color("#00ff00"))
				else
					diffuse(self,negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, pbtarget*100)
			else
				tDiff = msg.WifeDifferential
				if tDiff >= 0 then 											
					diffuse(self,positive)
				else
					diffuse(self,negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, target)
			end
		end
	}
end

if enabledTargetTracker then
	t[#t+1] = d
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    									**Display Percent**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Displays the current percent for the score.
]]

local cp = Def.ActorFrame{
	InitCommand = function(self)
		dp = self
		self:zoom(displayPercentZoom):addx(displayPercentX):addy(displayPercentY)
	end,
	Def.Quad{
		InitCommand=cmd(xy,60 + mpOffset,(SCREEN_HEIGHT*0.62)-90;;zoomto, 60, 13;diffuse,color("0,0,0,0.4");horizalign,left;vertalign,top)
	},
	-- Displays your current percentage score
	LoadFont("Common Large")..{											
		Name = "DisplayPercent",
		InitCommand=cmd(xy,115 + mpOffset,220;zoom,0.3;halign,1;valign,1),
		OnCommand=function(self)
			self:settextf("%05.2f%%", 0)
		end,
		JudgmentMessageCommand=function(self,msg)
			self:settextf("%05.2f%%", Floor(msg.WifePercent*100)/100)
		end
	},
}

if enabledDisplayPercent then
	t[#t+1] = cp
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											    	**Player judgment counter (aka pa counter)**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Counts judgments.
--]]

-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local frameX = 60 + mpOffset						 -- X position of the frame
local frameY = (SCREEN_HEIGHT*0.62)-90 				 -- Y Position of the frame
local spacing = 10									 -- Spacing between the judgetypes
local frameWidth = 60								 -- Width of the Frame
local frameHeight = ((#jdgT-1)*spacing)	- 8			 -- Height of the Frame
local judgeFontSize = 0.40							 -- Font sizes for different text elements 
local countFontSize = 0.35
local gradeFontSize = 0.45
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local jdgCounts = {}								 -- Child references for the judge counter

local j = Def.ActorFrame{
	InitCommand=function(self)
		judgeCounter = self
		self:addx(judgeCounterX):addy(judgeCounterY)
	end,
	OnCommand=function(self)
		for i=1,#jdgT do
			jdgCounts[jdgT[i]] = self:GetChild(jdgT[i])
		end
	end,
	JudgmentMessageCommand=function(self, msg)
		if jdgCounts[msg.Judgment] then
			settext(jdgCounts[msg.Judgment],msg.Val)
		end
	end																		
}																					

 local function makeJudgeText(judge,index)		-- Makes text
 	return LoadFont("Common normal")..{
 		InitCommand=cmd(xy,frameX+5,frameY+7+(index*spacing);zoom,judgeFontSize;halign,0),
 		OnCommand=function(self)
 			settext(self,getShortJudgeStrings(judge))
 			diffuse(self,jcT[judge])
 		end
 	}
 end
 
 local function makeJudgeCount(judge,index)		-- Makes county things for taps....
 	return LoadFont("Common Normal")..{
 		Name = judge,
		InitCommand=cmd(xy,frameWidth+frameX-5,frameY+7+(index*spacing);zoom,countFontSize;horizalign,right;settext,0) 	}
 end


-- Background
j[#j+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY+13;zoomto,frameWidth,frameHeight+18;diffuse,color("0,0,0,0.4");horizalign,left;vertalign,top)}

-- Build judgeboard
for i=1,#jdgT do
	j[#j+1] = makeJudgeText(jdgT[i],i)
	j[#j+1] = makeJudgeCount(jdgT[i],i)
end

-- Now add the completed judgment table to the primary actor frame t if enabled
if enabledJudgeCounter then
	t[#t+1] = j
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**Player ErrorBar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Visual display of deviance values. 
--]]

-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local barcount = 30 									-- Number of bars. Older bars will refresh if judgments/barDuration exceeds this value. You don't need more than 40.
local barWidth = 2										-- Width of the ticks.
local barDuration = 0.75 								-- Time duration in seconds before the ticks fade out. Doesn't need to be higher than 1. Maybe if you have 300 bars I guess.
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local currentbar = 1 									-- so we know which error bar we need to update
local ingots = {}										-- references to the error bars

-- Makes the error bars. They position themselves relative to the center of the screen based on your dv and diffuse to your judgement value before disappating or refreshing
-- Should eventually be handled by the game itself to optimize performance
function smeltErrorBar(index)
	return Def.Quad{
		Name = index,
		InitCommand=cmd(xy,errorBarX,errorBarY;zoomto,barWidth,errorBarHeight;diffusealpha,0),
		UpdateErrorBarCommand=function(self)						-- probably a more efficient way to achieve this effect, should test stuff later
			finishtweening(self)									-- note: it really looks like shit without the fade out 
			diffusealpha(self,1)
			diffuse(self,jcT[jdgCur])
			x(self,errorBarX+dvCur*wscale)
			self:y(errorBarY)  -- i dont know why man it doenst work the other way ( y(self,errorBarY) )
			self:zoomtoheight(errorBarHeight)
			linear(self,barDuration)
			diffusealpha(self,0)
		end
	}
end

local e = Def.ActorFrame{										
	InitCommand = function(self)
		eb = self:GetChildren()
		for i=1,barcount do											-- basically the equivalent of using GetChildren() if it returned unnamed children numerically indexed
			ingots[#ingots+1] = self:GetChild(i)
		end
	end,
	SpottedOffsetMessageCommand=function(self)				
		currentbar = ((currentbar)%barcount) + 1
		playcommand(ingots[currentbar],"UpdateErrorBar")			-- Update the next bar in the queue
	end,
	DootCommand=function(self)
		self:RemoveChild("DestroyMe")
		self:RemoveChild("DestroyMe2")
	end,

	Def.Quad {
		Name = "Center",
		InitCommand=cmd(diffuse,getMainColor('highlight');xy,errorBarX,errorBarY;zoomto,2,errorBarHeight)
	},
	-- Indicates which side is which (early/late) These should be destroyed after the song starts.
	LoadFont("Common Normal") .. {
		Name = "DestroyMe",
		InitCommand=cmd(xy,errorBarX+errorBarFrameWidth/4,errorBarY;zoom,0.35),
		BeginCommand=cmd(settext,"Late";diffusealpha,0;smooth,0.5;diffusealpha,0.5;sleep,1.5;smooth,0.5;diffusealpha,0),
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe2",
		InitCommand=cmd(xy,errorBarX-errorBarFrameWidth/4,errorBarY;zoom,0.35),
		BeginCommand=cmd(settext,"Early";diffusealpha,0;smooth,0.5;diffusealpha,0.5;sleep,1.5;smooth,0.5;diffusealpha,0;queuecommand,"Doot"),
		DootCommand=function(self)
			self:GetParent():queuecommand("Doot")
		end
	}
}

-- Initialize bars
for i=1,barcount do
	e[#e+1] = smeltErrorBar(i)
end

-- Add the completed errorbar frame to the primary actor frame t if enabled
if enabledErrorBar then
	t[#t+1] = e
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															   **Player Info**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Avatar and such, now you can turn it off. Planning to have player mods etc exported similarly to the nowplaying, and an avatar only option
]]
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PlayerInfo then
	t[#t+1] = LoadActor("playerinfo")
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														       **Full Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither. 
]]
 
-- User params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local width = SCREEN_WIDTH/2-100
local height = 10
local alpha = 0.7
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local p = Def.ActorFrame{
	InitCommand = function(self)
		self:xy(fullProgressBarX,fullProgressBarY)
		self:zoomto(fullProgressBarWidth,fullProgressBarHeight)
		fb = self
	end,
	Def.Quad{InitCommand=cmd(zoomto,width,height;diffuse,color("#666666");diffusealpha,alpha)},			-- background
	Def.SongMeterDisplay{
		InitCommand=function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth=width,
		Stream=Def.Quad{InitCommand=cmd(zoomy,height;diffuse,getMainColor("highlight"))}
	},
	LoadFont("Common Normal")..{																		-- title
		InitCommand=cmd(zoom,0.35;maxwidth,width*2),
		BeginCommand=cmd(settext,GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
	},
	LoadFont("Common Normal")..{																		-- total time
		InitCommand=cmd(x,width/2;zoom,0.35;maxwidth,width*2;halign,1),
		BeginCommand=function(self)
		local ttime = GetPlayableTime()
			settext(self,SecondsToMMSS(ttime))
			diffuse(self, ByMusicLength(ttime))
		end
	}
}

if enabledFullBar then
	t[#t+1] = p
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														      **Mini Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither. 
]]

local width = 34
local height = 4
local alpha = 0.3

mb = Def.ActorFrame{
	InitCommand = function(self)
		self:xy(miniProgressBarX,miniProgressBarY)
		mb = self
	end,
	Def.Quad{InitCommand=cmd(zoomto,width,height;diffuse,color("#666666");diffusealpha,alpha)}, 	-- background
	Def.Quad{InitCommand=cmd(x,1+width/2;zoomto,1,height;diffuse,color("#555555"))},				-- ending indicator
	Def.SongMeterDisplay{
		InitCommand=function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth=width,
		Stream=Def.Quad{InitCommand=cmd(zoomy,height;diffuse,getMainColor("highlight"))}
	}
}

if enabledMiniBar then
	t[#t+1] = mb
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**BPM Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Better optimized frame update bpm display. 
]]

local BPM
local a = GAMESTATE:GetPlayerState(PLAYER_1):GetSongPosition()	
local r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
local GetBPS = SongPosition.GetCurBPS

local function UpdateBPM(self)
	local bpm = GetBPS(a) * r
	settext(BPM,Round(bpm,2))
end

t[#t+1] = Def.ActorFrame{
	InitCommand=function(self)
		BPM = self:GetChild("BPM")
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then			-- dont bother updating for single bpm files
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			settext(BPM,Round(GetBPS(a) * r,2))
		end
	end,
	LoadFont("Common Normal")..{
		Name="BPM",
		InitCommand=cmd(x,SCREEN_CENTER_X;y,SCREEN_BOTTOM-20;halign,0.5;zoom,0.40)
	}
}



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															**Combo Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

]]

local x = 0
local y = 60

--This just initializes the initial point or not idk not needed to mess with this any more
function ComboTransformCommand( self, params )
	self:x( x )
	self:y( y )
end




--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														  **Judgment Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	moving here eventually
]]



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															 **NPS Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	re-enabling the old nps calc/graph for now 
]]

t[#t+1] = LoadActor("npscalc")



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															  **NPS graph**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ditto
]]

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
													**Message boxes for moving things**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	offset window esque boxes so its more intuitive to use the moving feature
]]
if(playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay) then
t[#t+1] = Def.ActorFrame{
	InitCommand=function(self)
		messageBox = self
	end,
	Def.BitmapText{
		Name= "errorBarPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Error Bar Position:",
				"X: " .. errorBarX,
				"Y: " .. errorBarY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "errorBarSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Error Bar Size:",
				"Width: " .. errorBarWidth,
				"Height: " .. errorBarHeight,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "targetTrackerPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Goal Tracker Position:",
				"X: " .. targetTrackerX,
				"Y: " .. targetTrackerY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "targetTrackerSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Goal Tracker Size:",
				"Zoom: " .. targetTrackerZoom,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "fullProgressBarPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Full Progress Bar Position:",
				"X: " .. fullProgressBarX,
				"Y: " .. fullProgressBarY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "fullProgressBarSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Full Progress Bar Size:",
				"Width: " .. fullProgressBarWidth,
				"Height: " .. fullProgressBarHeight,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "miniProgressBarPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Mini Progress Bar Position:",
				"X: " .. miniProgressBarX,
				"Y: " .. miniProgressBarY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "displayPercentPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Current Percent Position:",
				"X: " .. displayPercentX,
				"Y: " .. displayPercentY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "displayPercentSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Current Percent Size:",
				"Zoom: " .. displayPercentZoom,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "noteFieldPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Notefield Position:",
				"X: " .. noteFieldX,
				"Y: " .. noteFieldY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "noteFieldSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Notefield Size:",
				"Width: " .. noteFieldWidth,
				"Height: " .. noteFieldHeight,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "judgeCounterPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local text= {
				"Judge Counter Position:",
				"X: " .. judgeCounterX,
				"Y: " .. judgeCounterY,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	-- had to throw this here because it was getting x/y fucked up
	Def.BitmapText{
		Name= "judgmentPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local x = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeX
			local y = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.JudgeY
			local text= {
				"Judgment Label Position:",
				"X: " .. x,
				"Y: " .. y,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "judgmentSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local zoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.JudgeZoom
			local text= {
				"Judgment Label Size:",
				"Zoom: " .. zoom,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "comboPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local x = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ComboX
			local y = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.ComboY
			local text= {
				"Combo Position:",
				"X: " .. x,
				"Y: " .. y,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "comboSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local zoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.ComboZoom
			local text= {
				"Combo Size:",
				"Zoom: " .. zoom,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "npsDisplayPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local x = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NPSDisplayX
			local y = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NPSDisplayY
			local text= {
				"NPS Display Position:",
				"X: " .. x,
				"Y: " .. y,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "npsDisplaySizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local zoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NPSDisplayZoom
			local text= {
				"NPS Display Size:",
				"Zoom: " .. zoom,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "npsGraphPosText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local x = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NPSGraphX
			local y = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NPSGraphY
			local text= {
				"NPS Graph Position:",
				"X: " .. x,
				"Y: " .. y,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "npsGraphSizeText", Font= "Common Normal", 
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end,
		UpdateCommand=function(self)
			local width = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NPSGraphWidth
			local height = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NPSGraphHeight
			local text= {
				"NPS Display Size:",
				"Width: " .. width,
				"Height: " .. height,
			}
			self:settext(table.concat(text, "\n"))
		end,
	},
	Def.BitmapText{
		Name= "Instructions", Font= "Common Normal",
		InitCommand= function(self)
			self:horizalign(left):vertalign(top)
				:xy(SCREEN_WIDTH - 240, 110):zoom(.5):visible(true)
		end,
		OnCommand=function(self)
			local text= {
				"Hold the following and press the arrow",
				"keys to alter the associated element\n",
				"1: Judgement Text Position",
				"2: Judgement Text Size",
				"3: Combo Text Position",
				"4: Combo Text Size",
				"5: Error Bar Text Position",
				"6: Error Bar Text Size",
				"7: Target Tracker Text Position",
				"8: Target Tracker Text Size",
				"9: Full Progress Bar Position",
				"0: Full Progress Bar Size",
				"q: Mini Progress Bar Position",
				"w: Display Percent Text Position",
				"e: Display Percent Text Size",
				"r: Notefield Position",
				"t: Notefield Size",
				"y: NPS Display Text Position",
				"u: NPS Display Text Size",
				"i: NPS Graph Position",
				"o: NPS Graph Size",
				"p: Judge Counter Position",
			}
			self:settext(table.concat(text, "\n"))
		end
	},
}
end

return t