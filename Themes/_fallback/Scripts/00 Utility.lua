--- Random helper functions that don't really belong anywhere else.
-- @module 00_Utility

--- Scale a number to 4:3 resolution
-- @tparam number size4_3 the size to scale
-- @treturn number 4:3 scaled value
function get43size(size4_3)
	return 640 * (size4_3 / 854)
end

function getMousePosition()
	return {x = INPUTFILTER:GetMouseX(), y = INPUTFILTER:GetMouseY()}
end

function capWideScale(AR4_3, AR16_9)
	if AR4_3 < AR16_9 then
		return clamp(WideScale(AR4_3, AR16_9), AR4_3, AR16_9)
	else
		return clamp(WideScale(AR4_3, AR16_9), AR16_9, AR4_3)
	end
end

--- Returns current autoplay type. returns a integer between 0~2 corresponding to
-- human, autoplay and autoplay cpu respectively.
-- @treturn number
function getAutoplay()
	return Enum.Reverse(PlayerController)[tostring(PREFSMAN:GetPreference("AutoPlay"))]
end

--- Returns true if windowed mode is on
-- @treturn boolean
function isWindowed()
	return PREFSMAN:GetPreference("Windowed")
end

--- Get the actor's real X (Not relative to the parent like self:GetX()) by recursively grabbing the parents' position
-- @tparam actor element the actor
-- @treturn number
function getTrueX(element)
	element:GetTrueX()
end

--- Get the actor's real Y (Not relative to the parent like self:GetY()) by recursively grabbing the parents' position
-- @tparam actor element the actor
-- @treturn number
function getTrueY(element)
	element:GetTrueY()
end

--- Checks whether the mouse is over an actor
-- @tparam actor element the actor
-- @treturn bool true if the mouse is over the actor
function isOver(element)
	local mouse = getMousePosition()
	return element:IsOver(mouse.x, mouse.y)
end

--- returns true if the table contains the key.
-- @tab table
-- @tparam any key
-- @treturn bool
function tableContains(table, key)
	return (table[key] ~= nil)
end

--- for non-array tables.
-- @tab table
-- @treturn int
function getTableSize(table)
	local i = 0
	for k, v in pairs(table) do
		i = i + 1
	end
	return i
end

--- returns the hexadecimal representaion of the MD5 hash.
-- @string sPath
-- @treturn string|0 0 if invalid
function MD5FileHex(sPath)
	local text = {}
	local MD5 = CRYPTMAN:MD5File(sPath)
	for i = 1, #MD5 do
		text[i] = string.format("%02X", string.byte(MD5, i) or 0)
	end
	if #text == 16 then
		return table.concat(text)
	else
		return 0 --invalid
	end
end

--- Returns the hexadecimal representation of the SHA-1 hash for the file.
-- @string sPath
-- @treturn string|0 0 if invalid
function SHA1FileHex(sPath)
	local text = {}
	local SHA1 = CRYPTMAN:SHA1File(sPath)
	for i = 1, #SHA1 do
		text[i] = string.format("%02X", string.byte(SHA1, i) or 0)
	end
	if #text == 20 then
		return table.concat(text)
	else
		return 0 --invalid
	end
end

--- Returns the hexadecimal representaion of the MD5 hash for the string.
-- @string str
-- @treturn string|0 0 if invalid
function MD5StringHex(str)
	local text = {}
	local MD5 = CRYPTMAN:MD5String(str)
	for i = 1, #MD5 do
		text[i] = string.format("%02X", string.byte(MD5, i) or 0)
	end
	if #text == 16 then
		return table.concat(text)
	else
		return 0 --invalid
	end
end

--- Returns the hexadecimal representation of the SHA-1 hash for the string
-- @string str
-- @treturn string|0 0 if invalid
function SHA1StringHex(str)
	local text = {}
	local SHA1 = CRYPTMAN:SHA1String(str)
	for i = 1, #SHA1 do
		text[i] = string.format("%02X", string.byte(SHA1, i) or 0)
	end
	if #text == 20 then
		return table.concat(text)
	else
		return 0 --invalid
	end
end

--- Given a table of file paths/names in a fileList
-- filters out files from fileList that aren't in the type in fileTypes
-- both function parameters are passed on as tables
-- @tparam {string} fileList
-- @tparam {string} fileTypes
function filterFileList(fileList, fileTypes)
	local t = {}
	for i = 1, #fileList do
		local add = false
		local s
		for _, v in pairs(fileTypes) do
			s = fileList[i]
			s = s:sub(-v:len())
			if s == v then
				add = true
			end
		end
		if add then
			table.insert(t, fileList[i])
		end
	end
	return t
end

--- Just something to get rid of scores where the player quit out early.
-- dead because chord cohesion removal -mina
-- always returns true now
function isScoreValid(pn, steps, score)
	return true
end

--- Should make the currently played sscore not save
-- Doesn't work for some reason rip-
function disqualifyScore()
	local so = GAMESTATE:GetSongOptionsObject("ModsLevel_Song")
	if so:SaveScore() then
		so:SaveScore(false)
		SCREENMAN:SystemMessage("SaveScore set to false")
	else
		SCREENMAN:SystemMessage("SaveScore already set to false")
	end
end

--- Values based on ArrowEffects.cpp
-- Gets the note scale from the mini mod being used.
-- Do not use pn_old_deprecated
function getNoteFieldScale(pn_old_deprecated)
	if (pn_old_deprecated == PLAYER_2) then
		return 0
	end
	local pn = PLAYER_1
	local po = GAMESTATE:GetPlayerState(pn):GetPlayerOptions("ModsLevel_Preferred")
	local val, as = po:Mini()
	local zoom = 1
	zoom = 1 - (val * 0.5)
	if math.abs(zoom) < 0.01 then
		zoom = 0.01
	end
	return zoom
end

--- Gets the width of the note assuming the base width is 64.
-- Do not use pn_old_deprecated
function getNoteFieldWidth(pn_old_deprecated)
	if (pn_old_deprecated == PLAYER_2) then
		return 0
	end
	local pn = PLAYER_1
	local baseWidth = 64 -- is there a way to grab a noteskin width..?
	local style = GAMESTATE:GetCurrentStyle()
	local cols = style:ColumnsPerPlayer()
	return cols * baseWidth * getNoteFieldScale(pn)
end

--- Gets the center X position of the notefield.
-- Do not use pn_old_deprecated
function getNoteFieldPos(pn_old_deprecated)
	if (pn_old_deprecated == PLAYER_2) then
		return -1000
	end
	local pn = PLAYER_1
	local pNum = (pn == PLAYER_1) and 1 or 2
	local style = GAMESTATE:GetCurrentStyle()
	local cols = style:ColumnsPerPlayer()
	local styleType = ToEnumShortString(style:GetStyleType())
	local centered = ((cols >= 6) or PREFSMAN:GetPreference("Center1Player"))

	if centered then
		return SCREEN_CENTER_X
	else
		return THEME:GetMetric("ScreenGameplay", string.format("PlayerP%i%sX", pNum, styleType))
	end
end

--- Get the most common BPM in a list of BPMChanges
-- Should check for correctness later.
-- @tparam {{number}} bpmChanges table of {row, bpm}
-- @tparam int lastBeat
-- @treturn int maxBPM
function getCommonBPM(bpmChanges, lastBeat)
	local BPMtable = {}
	local curBPM = math.round(bpmChanges[1][2])
	local curBeat = bpmChanges[1][1]
	for _, v in ipairs(bpmChanges) do
		if BPMtable[tostring(curBPM)] == nil then
			BPMtable[tostring(curBPM)] = (v[1] - curBeat) / curBPM
		else
			BPMtable[tostring(curBPM)] = BPMtable[tostring(curBPM)] + (v[1] - curBeat) / curBPM
		end
		curBPM = math.round(v[2])
		curBeat = v[1]
	end

	if BPMtable[tostring(curBPM)] == nil then
		BPMtable[tostring(curBPM)] = (lastBeat - curBeat) / curBPM
	else
		BPMtable[tostring(curBPM)] = BPMtable[tostring(curBPM)] + (lastBeat - curBeat) / curBPM
	end

	local maxBPM = 0
	local maxDur = 0
	for k, v in pairs(BPMtable) do
		if v > maxDur then
			maxDur = v
			maxBPM = tonumber(k)
		end
	end
	return maxBPM
end

--- Get number of bpm changes in a bpm changes array
-- DDreamStudio adds small, indetectable bpm changes which make
-- #bpmChanges not accurate
-- @tparam {{number}} bpmChanges table of {row, bpm}
-- @treturn int BPMChangeCount
function getBPMChangeCount(bpmChanges)
	local count = 0
	local threshhold = 5 -- get rid of ddreamspeed changes
	local curBPM = bpmChanges[1][2]
	for k, v in ipairs(bpmChanges) do
		if math.abs(curBPM - v[2]) > threshhold then
			count = count + 1
		end
		curBPM = v[2]
	end

	return count
end
--- Returns a string of the form "(KeyCount)K" like "4K"
-- Uses GAMESTATE:GetCurrentSteps(PLAYER_1):GetStepsType()
-- @treturn string keymode
function getCurrentKeyMode()
	local keys = {
		StepsType_Dance_Threepanel = "3K",
		StepsType_Dance_Single = "4K",
		StepsType_Pump_Single = "5K",
		StepsType_Pump_Halfdouble = "6K",
		StepsType_Bm_Single5 = "6K",
		StepsType_Dance_Solo = "6K",
		StepsType_Kb7_Single = "7K",
		StepsType_Bm_Single7 = "8K",
		StepsType_Dance_Double = "8K",
		StepsType_Pump_Double = "10K",
		StepsType_Bm_Double5 = "12K",
		StepsType_Bm_Double7 = "16K",
	}
	local stepstype = GAMESTATE:GetCurrentSteps(PLAYER_1):GetStepsType()
	return keys[stepstype]
end

--- Returns translated modifiers given a raw string of modifiers
-- @tparam {string} raw string of modifiers
-- @treturn {string} translated string of modifiers
function getModifierTranslations(source)
	local translated = {}
	for mod in string.gmatch(source, "[^,%s]+") do
		table.insert(translated, THEME:HasString("OptionNames", mod) and THEME:GetString("OptionNames", mod) or mod)
	end
	return table.concat(translated, ", ")
end