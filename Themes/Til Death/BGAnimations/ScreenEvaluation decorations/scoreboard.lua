local lines = 5 -- number of scores to display
local framex = SCREEN_WIDTH - capWideScale(get43size(230), 230)
local framey = 60
local frameWidth = capWideScale(get43size(220), 220)
local spacing = 34

local song = STATSMAN:GetCurStageStats():GetPlayedSongs()[1]

local profile
local steps
local origTable
local hsTable
local rtTable
local scoreIndex
local score
local usingSSRSort = PREFSMAN:GetPreference("SortBySSRNormPercent")

local player = GAMESTATE:GetEnabledPlayers()[1]

if GAMESTATE:IsPlayerEnabled(player) then
	profile = GetPlayerOrMachineProfile(player)
	steps = STATSMAN:GetCurStageStats():GetPlayerStageStats(player):GetPlayedSteps()[1]
	origTable = getScoresByKey(player)
	score = STATSMAN:GetCurStageStats():GetPlayerStageStats(player):GetHighScore()
	rtTable = getRateTable(origTable)
	if rtTable == nil then
		return
	end
	hsTable = rtTable[getRate(score)] or {score}
	scoreIndex = getHighScoreIndex(hsTable, score)
end

local curPage = 1
local maxPages = math.ceil(#hsTable/lines)

local function movePage(n)
	if n > 0 then 
		curPage = ((curPage+n-1) % maxPages + 1)
	else
		curPage = ((curPage+n+maxPages-1) % maxPages+1)
	end
	MESSAGEMAN:Broadcast("UpdatePage")
end

--Input event for mouse clicks
local function input(event)
	local scoreBoard = SCREENMAN:GetTopScreen():GetChildren().scoreBoard
	if event.DeviceInput.button == "DeviceButton_left mouse button" and scoreBoard then
		if event.type == "InputEventType_Release" then
			for i = 0, math.min(lines, #hsTable) - 1 do
				if isOver(scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("mouseOver")) then
					scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("grade"):visible(
						not scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("grade"):GetVisible()
					)
					scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("judge"):visible(
						not scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("judge"):GetVisible()
					)
					scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("date"):visible(
						not scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("date"):GetVisible()
					)
					scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("option"):visible(
						not scoreBoard:GetChild("scoreItem" .. tostring(i)):GetChild("option"):GetVisible()
					)
				end
			end
		end
	end
	if event.type == "InputEventType_FirstPress" and scoreBoard then
		if event.button == "MenuLeft" then
			movePage(-1)
		end

		if event.button == "MenuRight" then
			movePage(1)
		end

		if event.DeviceInput.button == "DeviceButton_mousewheel up" then
			MESSAGEMAN:Broadcast("WheelUpSlow")
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
			MESSAGEMAN:Broadcast("WheelDownSlow")
		end

	end
	return false
end

local t =
	Def.ActorFrame {
	Name = "scoreBoard",
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end
}

local function scoreitem(pn, index, scoreIndex, drawindex)
	-- first box always displays the number 1 score
	if drawindex == 0 then
		index = 1
	end

	--Whether the score at index is the score that was just played.
	local equals = (index == scoreIndex)

	--
	local t =
		Def.ActorFrame {
		Name = "scoreItem" .. tostring(drawindex),
		ShowCommand = function(self)
			self:playcommand("Begin")
			self:x(100)
			self:diffusealpha(0)
			self:finishtweening()
			self:sleep((drawindex)*0.03)
			self:linear(0.3)
			self:x(0)
			self:diffusealpha(1)
		end,
		HideCommand = function(self)
			self:stoptweening()
			self:linear(0.1)
			self:diffusealpha(0)
			self:x(SCREEN_WIDTH*10)
		end,
		UpdatePageMessageCommand = function(self)
			if index == 1 then return end
			-- this weird math sets the index for every element but the top one
			-- so basically the order for 5 lines on the 2nd page is 1,6,7,8,9 and so on
			index = (curPage - 1) * lines + drawindex+1 + (curPage > 1 and (-1 - (curPage > 2 and curPage-2 or 0)) or 0)
			equals = (index == scoreIndex)
			if hsTable[index] ~= nil then
				self:playcommand("Show")
			else
				self:playcommand("Hide")
			end
		end,
		BeginCommand = function(self)
			if hsTable[index] == nil then 
				self:playcommand("Hide")
			end
		end,
		--The main quad
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(frameWidth, 30):halign(0):valign(0):diffuse(
					color("#333333")
				):diffusealpha(1):diffuserightedge(color("#33333333"))
			end,
			BeginCommand = function(self)
				self:visible(GAMESTATE:IsHumanPlayer(pn))
			end
		},
		--Highlight quad for the current score
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(frameWidth, 30):halign(0):valign(0):diffuse(
					color("#ffffff")
				):diffusealpha(0.3):diffuserightedge(color("#33333300"))
			end,
			BeginCommand = function(self)
				self:visible(GAMESTATE:IsHumanPlayer(pn) and equals)
			end
		},
		--Quad that will act as the bounding box for mouse rollover/click stuff.
		Def.Quad {
			Name = "mouseOver",
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(frameWidth*2, 30):halign(0):valign(0):diffuse(
					getMainColor("highlight")
				):diffusealpha(0)
			end
		},
		--ClearType lamps
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(8, 30):halign(0):valign(0)
			end,
			BeginCommand = function(self)
				if hsTable[index] == nil then return end
				self:visible(GAMESTATE:IsHumanPlayer(pn)):diffuse(
					getClearTypeFromScore(pn, hsTable[index], 2))
			end
		},
		--rank
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:xy(framex - 8, framey + 12 + (drawindex * spacing)):zoom(0.35)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					if #hsTable >= 1 then
						self:settext(index)
						if equals then
							self:diffuse(color("#ffcccc"))
						else
							self:stopeffect()
						end
					end
				end
			},
		-- Wife grade and %score
		LoadFont("Common normal") ..
			{
				Name = "grade",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 11 + (drawindex * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.3)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					local wstring = "Wife"
					if usingSSRSort then
						wstring = "Wife J4"
					end
					if hsTable[index]:GetWifeScore() == 0 then
						self:settextf("NA (%s)", wstring)
					else
						self:settextf("%05.2f%% (%s)", notShit.floor(hsTable[index]:GetWifeScore() * 10000) / 100, wstring)
					end
				end
			},
		--mods
		LoadFont("Common normal") ..
			{
				Name = "option",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 11 + (drawindex * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.35)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					self:settext(getModifierTranslations(hsTable[index]:GetModifiers()))
					self:visible(false)
				end
			},
		--grade text
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:xy(framex + 130 + capWideScale(get43size(0), 50), framey + 2 + (drawindex * spacing)):zoom(0.35):halign(0.5):maxwidth(
						(frameWidth - 15) / 0.35
					)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					if #hsTable >= 1 and index >= 1 then
						self:settext(getGradeStrings(hsTable[index]:GetWifeGrade()))
						self:diffuse(getGradeColor(hsTable[index]:GetWifeGrade()))
					end
				end
			},
		--cleartype text
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:xy(framex + 130 + capWideScale(get43size(0), 50), framey + 12 + (drawindex * spacing)):zoom(0.35):halign(0.5):maxwidth(
						(frameWidth - 15) / 0.35
					)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					if #hsTable >= 1 and index >= 1 then
						self:settext(getClearTypeFromScore(pn, hsTable[index], 0))
						self:diffuse(getClearTypeFromScore(pn, hsTable[index], 2))
					end
				end
			},
		--max combo
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:xy(framex + 130 + capWideScale(get43size(0), 50), framey + 22 + (drawindex * spacing)):zoom(0.35):halign(0.5):maxwidth(
						(frameWidth - 15) / 0.35
					)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					if #hsTable >= 1 and index >= 1 then
						self:settextf("%sx", hsTable[index]:GetMaxCombo())
					end
				end
			},
		--judgment
		LoadFont("Common normal") ..
			{
				Name = "judge",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 20 + (drawindex * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.35)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					if #hsTable >= 1 and index >= 1 then
						self:settextf(
							"%d / %d / %d / %d / %d / %d",
							hsTable[index]:GetTapNoteScore("TapNoteScore_W1"),
							hsTable[index]:GetTapNoteScore("TapNoteScore_W2"),
							hsTable[index]:GetTapNoteScore("TapNoteScore_W3"),
							hsTable[index]:GetTapNoteScore("TapNoteScore_W4"),
							hsTable[index]:GetTapNoteScore("TapNoteScore_W5"),
							hsTable[index]:GetTapNoteScore("TapNoteScore_Miss")
						)
					end
				end
			},
		--date
		LoadFont("Common normal") ..
			{
				Name = "date",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 20 + (drawindex * spacing)):zoom(0.35):halign(0)
				end,
				BeginCommand = function(self)
					if hsTable[index] == nil then return end
					if #hsTable >= 1 and index >= 1 then
						self:settext(hsTable[index]:GetDate())
					end
					self:visible(false)
				end
			}
	}
	return t
end

--can't have more lines than the # of scores huehuehu
if lines > #hsTable then
	lines = #hsTable
	curPage = 1
else
	curPage = math.ceil(scoreIndex / (lines-1))
end

-- weird math explanation can be found above somewhere
local drawindex = 0
local startind = (curPage-1) * lines + 1 + (curPage > 1 and (-1 - (curPage > 2 and curPage-2 or 0)) or 0)
while drawindex < lines do
	t[#t+1] = scoreitem(player,startind,scoreIndex,drawindex)
	startind = startind+1
	drawindex  = drawindex+1
end

--Update function for showing mouse rollovers
local function Update(self)
	t.InitCommand = function(self)
		self:SetUpdateFunction(Update)
	end
	for i = 0, drawindex - 1 do
		if isOver(self:GetChild("scoreItem" .. tostring(i)):GetChild("mouseOver")) then
			self:GetChild("scoreItem" .. tostring(i)):GetChild("mouseOver"):diffusealpha(0.2)
		else
			self:GetChild("scoreItem" .. tostring(i)):GetChild("mouseOver"):diffusealpha(0)
			self:GetChild("scoreItem" .. tostring(i)):GetChild("grade"):visible(true)
			self:GetChild("scoreItem" .. tostring(i)):GetChild("judge"):visible(true)
			self:GetChild("scoreItem" .. tostring(i)):GetChild("date"):visible(false)
			self:GetChild("scoreItem" .. tostring(i)):GetChild("option"):visible(false)
		end
	end
end
t.InitCommand = function(self)
	self:SetUpdateFunction(Update)
end

t[#t+1] = Def.Quad {
	InitCommand = function(self)
		self:xy(framex - 20,framey - 20)
		self:valign(0):halign(0)
		self:diffusealpha(0)
		self:zoomto(20 + frameWidth, 20 + (30) * lines + lines * (5))
	end,
	WheelUpSlowMessageCommand = function(self)
		if isOver(self) then
			movePage(-1)
		end
	end,
	WheelDownSlowMessageCommand = function(self)
		if isOver(self) then
			movePage(1)
		end
	end

}

return t
