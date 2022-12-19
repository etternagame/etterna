local lines = 5 -- number of scores to display
local framex = SCREEN_WIDTH - capWideScale(get43size(230), 230)
local framey = 60
local frameWidth = capWideScale(get43size(220), 220)
local spacing = 34

local song = STATSMAN:GetCurStageStats():GetPlayedSongs()[1]

local steps = STATSMAN:GetCurStageStats():GetPlayerStageStats():GetPlayedSteps()[1]
local origTable = getScoresByKey(player)
local score = SCOREMAN:GetMostRecentScore()
local rtTable = getRateTable(origTable) or {}
local hsTable = rtTable[getRate(score)] or {score}
local scoreIndex = getHighScoreIndex(hsTable, score)

local usingSSRSort = PREFSMAN:GetPreference("SortBySSRNormPercent")

if rtTable == nil then
	return {}
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

local t = Def.ActorFrame {
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
	local t = Def.ActorFrame {
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

			-- we won't have a score selected upon changing the page so make sure the highlights go away at first
			self:GetParent():GetParent():playcommand("HahaThisCodeINeedHelp", {doot = nil})
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
				self:visible(GAMESTATE:IsHumanPlayer())
			end
		},
		--Highlight quad for the current score
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(frameWidth, 30):halign(0):valign(0):diffuse(
					color("#ffffff")
				):diffusealpha(0.3):diffuserightedge(color("#33333300"))
			end,
			HahaThisCodeINeedHelpCommand = function(self, params)
				local equis = params.doot == index
				self:visible(GAMESTATE:IsHumanPlayer() and equis)
			end,
			BeginCommand = function(self)
				self:visible(GAMESTATE:IsHumanPlayer() and equals)

				-- it was once asked if anything had been hacked so hard as some thing that had been hacked really hard.. but yes.. this is
				-- hackered... even hardered.... force the offset plot to update if the index in the scoreboard list matches the currently
				-- displayed score.. this is because the offset plot was previously using pss to get its info and the way the current system
				-- is setup this is the most direct way to actually get the pointer to the score being displayed
				if equals then
					self:GetParent():GetParent():GetParent():GetChild("OffsetPlot"):playcommand("SetFromScore", {score =  hsTable[index]})
				end
			end
		},
		--Quad that will act as the bounding box for mouse rollover/click stuff.
		UIElements.QuadButton(1, 1) .. {
			Name = "mouseOver",
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(frameWidth*2, 30):halign(0):valign(0):diffuse(
					getMainColor("highlight")
				):diffusealpha(0)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" then
					local p = self:GetParent()
					local grade = p:GetChild("grade")
					local judge = p:GetChild("judge")
					local date = p:GetChild("date")
					local option = p:GetChild("option")
					local cleartype = p:GetChild("ClearType")
					
					grade:visible(not grade:GetVisible())
					judge:visible(not judge:GetVisible())
					date:visible(not date:GetVisible())
					option:visible(not option:GetVisible())
					cleartype:visible(not cleartype:GetVisible())

					local score = hsTable[index]
					if score ~= nil then
						if not score:HasReplayData() then return end
						newindex = getHighScoreIndex(hsTable, hsTable[index])
						self:GetParent():GetParent():playcommand("HahaThisCodeINeedHelp", {doot = newindex})
						self:GetParent():GetParent():GetParent():GetChild("ScoreDisplay"):playcommand("ChangeScore", {score =  hsTable[index]})
						self:GetParent():GetParent():GetParent():GetChild("OffsetPlot"):playcommand("SetFromScore", {score =  hsTable[index]})
					end
				end
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(0.2)
			end,
			MouseOutCommand = function(self)
				local p = self:GetParent()
				local grade = p:GetChild("grade")
				local judge = p:GetChild("judge")
				local date = p:GetChild("date")
				local option = p:GetChild("option")
				local cleartype = p:GetChild("ClearType")
				self:diffusealpha(0)
				grade:visible(true)
				judge:visible(true)
				date:visible(false)
				cleartype:visible(true)
				option:visible(false)
			end,
		},
		--ClearType lamps
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + (drawindex * spacing) - 4):zoomto(8, 30):halign(0):valign(0)
			end,
			BeginCommand = function(self)
				if hsTable[index] == nil then return end
				self:visible(GAMESTATE:IsHumanPlayer()):diffuse(
					getClearTypeFromScore(pn, hsTable[index], 2))
			end
		},
		--rank
		LoadFont("Common normal") .. {
			InitCommand = function(self)
				self:xy(framex - 8, framey + 12 + (drawindex * spacing)):zoom(0.35)
			end,
			HahaThisCodeINeedHelpCommand = function(self, params)
				if params.doot == index then
					self:diffuse(color("#ffcccc"))
				else
					self:diffuse(color("ffffff"))
				end
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
		LoadFont("Common normal") .. {
			Name = "grade",
			InitCommand = function(self)
				self:xy(framex + 10, framey + 11 + (drawindex * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.3)
			end,
			BeginCommand = function(self)
				if hsTable[index] == nil then return end
				local wv = hsTable[index]:GetWifeVers()
				local wstring = "Wife" .. wv
				if usingSSRSort then
					wstring = "Wife" .. wv .. " J4"
				end
				if hsTable[index]:GetWifeScore() == 0 then
					self:settextf("NA (%s)", wstring)
				else
					local perc = hsTable[index]:GetWifeScore() * 100
					if perc > 99.65 then
						self:settextf("%05.4f%% (%s)", notShit.floor(perc, 4), wstring)
					else
						self:settextf("%05.2f%% (%s)", notShit.floor(perc, 2), wstring)
					end
				end
			end
		},
		--mods
		LoadFont("Common normal") .. {
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
		LoadFont("Common normal") .. {
			Name = "Grade",
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
		LoadFont("Common normal") .. {
			Name = "ClearType",
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
		LoadFont("Common normal") .. {
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
		LoadFont("Common normal") .. {
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
		LoadFont("Common normal") .. {
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

curPage = 1
--can't have more lines than the # of scores huehuehu
if lines > #hsTable then
	lines = #hsTable
else
	-- linear search for the score index to set the proper curPage
	if scoreIndex > lines then
		curPage = curPage + 1
		local j = 0
		for i = lines+1, #hsTable do
			j = j + 1
			if j == lines - 1 then
				j = 0
				curPage = curPage + 1
			end
			if i == scoreIndex then
				break
			end
		end
	end
end

-- weird math explanation can be found above somewhere
local drawindex = 0
local startind = (curPage-1) * lines + 1 + (curPage > 1 and (-1 - (curPage > 2 and curPage-2 or 0)) or 0)
while drawindex < lines do
	t[#t+1] = scoreitem(player,startind,scoreIndex,drawindex)
	startind = startind+1
	drawindex  = drawindex+1
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
	end,
}

return t
