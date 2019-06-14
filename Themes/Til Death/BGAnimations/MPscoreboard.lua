local lines = 10 -- number of scores to display
local framex = SCREEN_WIDTH - capWideScale(get43size(230), 230)
local framey = 60
local frameWidth = capWideScale(get43size(220), 220)
local spacing = 34
local multiscores

-- Takes an index into multiscores
local function SetActivePlayer(locaIndex)
	local scoreBoard = SCREENMAN:GetTopScreen():GetChildren().scoreBoard
	local item = scoreBoard:GetChild(locaIndex)
	SCREENMAN:GetTopScreen():SetCurrentPlayerByName(multiscores[locaIndex].user)
	item:GetChild("grade"):visible(not item:GetChild("grade"):GetVisible())
	item:GetChild("clear"):visible(not item:GetChild("clear"):GetVisible())
	item:GetChild("wife"):visible(not item:GetChild("wife"):GetVisible())
	item:GetChild("combo"):visible(not item:GetChild("combo"):GetVisible())
	item:GetChild("judge"):visible(not item:GetChild("judge"):GetVisible())
	item:GetChild("option"):visible(not item:GetChild("option"):GetVisible())
end
--Input event for mouse clicks
local function input(event)
	local scoreBoard = SCREENMAN:GetTopScreen():GetChildren().scoreBoard
	if event.DeviceInput.button == "DeviceButton_left mouse button" and scoreBoard then
		if event.type == "InputEventType_Release" then
			local scoreboard_items = scoreBoard:GetChildren()
			for i = 1, #multiscores do
				if isOver(scoreboard_items[tostring(i)]:GetChild("mouseOver")) then
					SetActivePlayer(i)
				end
			end
		end
	end
	return false
end

local function Update(self)
	for i = 1, lines do
		local item = self:GetChild(i)
		local mouseOver = item:GetChild("mouseOver")
		if isOver(mouseOver) then
			mouseOver:diffusealpha(0.2)
		else
			mouseOver:diffusealpha(0)
			item:GetChild("grade"):visible(true)
			item:GetChild("wife"):visible(true)
			item:GetChild("combo"):visible(true)
			item:GetChild("judge"):visible(true)
			item:GetChild("clear"):visible(true)
			item:GetChild("option"):visible(false)
		end
	end
end

local sortFunction = function(first, second)
	return first.highscore:GetWifeScore() > second.highscore:GetWifeScore()
end

local function updateScoreBoard(self)
	local selectedUserName = false
	local selectedIndex = false
	local screen = SCREENMAN:GetTopScreen()
	if multiscores then
		local cur = screen:GetCurrentPlayer()
		for i = 1, #multiscores do
			if cur == multiscores[i].idx then
				selectedUserName = multiscores[i].user
			end
		end
	end
	selectedUserName = selectedUserName or NSMAN:GetLoggedInUsername()

	multiscores = NSMAN:GetEvalScores()
	-- Since we sort we need to store the originall indexes
	for i = 1, #multiscores do
		multiscores[i].idx = i
	end
	table.sort(multiscores, sortFunction)
	for i = 1, #multiscores do
		self:GetChild(i):queuecommand("UpdateNetScore")
		if selectedUserName and multiscores[i].user == selectedUserName then
			selectedIndex = i
		end
	end

	if #multiscores > 0 and type(selectedIndex) == "number" then
		SetActivePlayer(selectedIndex)
	end
end
local t =
	Def.ActorFrame {
	Name = "scoreBoard",
	InitCommand = function(self)
		self:SetUpdateFunction(Update)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	NewMultiScoreMessageCommand = updateScoreBoard
}

local function scoreitem(pn, i)
	local t =
		Def.ActorFrame {
		Name = tostring(i),
		InitCommand = function(self)
			self:visible(false)
		end,
		UpdateNetScoreCommand = function(self)
			self:visible(i <= #multiscores)
		end,
		--The main quad
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + ((i - 1) * spacing) - 4):zoomto(frameWidth, 30):halign(0):valign(0):diffuse(
					color("#333333")
				):diffusealpha(1):diffuserightedge(color("#33333333"))
			end
		},
		--Highlight quad for the current score
		Def.Quad {
			InitCommand = function(self)
				self:xy(framex, framey + ((i - 1) * spacing) - 4):zoomto(frameWidth, 30):halign(0):valign(0):diffuse(
					color("#ffffff")
				):diffusealpha(0.3):diffuserightedge(color("#33333300"))
			end,
			UpdateNetScoreCommand = function(self)
				self:visible(multiscores[i] and SCREENMAN:GetTopScreen():GetCurrentPlayer() == multiscores[i].idx or false)
			end,
			UpdateNetEvalStatsMessageCommand = function(self)
				self:visible(multiscores[i] and SCREENMAN:GetTopScreen():GetCurrentPlayer() == multiscores[i].idx or false)
			end
		},
		--Quad that will act as the bounding box for mouse rollover/click stuff.
		Def.Quad {
			Name = "mouseOver",
			UpdateNetScoreCommand = function(self)
				self:xy(framex, framey + ((i - 1) * spacing) - 4):zoomto(frameWidth * 2, 30):halign(0):valign(0):diffuse(
					getMainColor("highlight")
				):diffusealpha(0)
			end
		},
		--ClearType lamps
		Def.Quad {
			UpdateNetScoreCommand = function(self)
				self:xy(framex, framey + ((i - 1) * spacing) - 4):zoomto(8, 30):halign(0):valign(0):diffuse(
					getClearTypeFromScore(pn, hs, 2)
				)
			end
		},
		--rank
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:xy(framex - 8, framey + 12 + ((i - 1) * spacing)):zoom(0.35)
				end,
				UpdateNetScoreCommand = function(self)
					self:settext(i)
					if multiscores[i] and SCREENMAN:GetTopScreen():GetCurrentPlayer() == multiscores[i].idx then
						self:diffuse(color("#ffcccc"))
					else
						self:diffuse(color("#FFFFFF"))
					end
				end,
				UpdateNetEvalStatsMessageCommand = function(self)
					self:playcommand("UpdateNetScore")
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "wife",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 11 + ((i - 1) * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.3)
				end,
				UpdateNetScoreCommand = function(self)
					self:settextf(
						"%05.2f%% (%s) - %sx",
						notShit.floor(multiscores[i].highscore:GetWifeScore() * 10000) / 100,
						"Wife",
						string.format("%.2f", multiscores[i].highscore:GetMusicRate()):gsub("%.?0+$", "")
					)
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "user",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 1 + ((i - 1) * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.3)
				end,
				UpdateNetScoreCommand = function(self)
					self:settextf(multiscores[i].user)
					if Grade:Reverse()[GetGradeFromPercent(multiscores[i].highscore:GetWifeScore())] < 2 then -- this seeems right -mina
						self:rainbowscroll(true)
					end
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "option",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 11 + ((i - 1) * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.35)
				end,
				UpdateNetScoreCommand = function(self)
					self:settext(multiscores[i].highscore:GetModifiers())
					self:visible(false)
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "grade",
				InitCommand = function(self)
					self:xy(framex + 130 + capWideScale(get43size(0), 50), framey + 2 + ((i - 1) * spacing)):zoom(0.35):halign(0.5):maxwidth(
						(frameWidth - 15) / 0.35
					)
				end,
				UpdateNetScoreCommand = function(self)
					self:settext(getGradeStrings(multiscores[i].highscore:GetWifeGrade()))
					self:diffuse(getGradeColor(multiscores[i].highscore:GetWifeGrade()))
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "clear",
				InitCommand = function(self)
					self:xy(framex + 130 + capWideScale(get43size(0), 50), framey + 12 + ((i - 1) * spacing)):zoom(0.35):halign(0.5):maxwidth(
						(frameWidth - 15) / 0.35
					)
				end,
				UpdateNetScoreCommand = function(self)
					self:settext(getClearTypeFromScore(pn, multiscores[i].highscore, 0))
					self:diffuse(getClearTypeFromScore(pn, multiscores[i].highscore, 2))
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "combo",
				InitCommand = function(self)
					self:xy(framex + 130 + capWideScale(get43size(0), 50), framey + 22 + ((i - 1) * spacing)):zoom(0.35):halign(0.5):maxwidth(
						(frameWidth - 15) / 0.35
					)
				end,
				UpdateNetScoreCommand = function(self)
					self:settextf("%sx", multiscores[i].highscore:GetMaxCombo())
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "judge",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 20 + ((i - 1) * spacing)):zoom(0.35):halign(0):maxwidth((frameWidth - 15) / 0.35)
				end,
				UpdateNetScoreCommand = function(self)
					self:settextf(
						"%d / %d / %d / %d / %d / %d",
						multiscores[i].highscore:GetTapNoteScore("TapNoteScore_W1"),
						multiscores[i].highscore:GetTapNoteScore("TapNoteScore_W2"),
						multiscores[i].highscore:GetTapNoteScore("TapNoteScore_W3"),
						multiscores[i].highscore:GetTapNoteScore("TapNoteScore_W4"),
						multiscores[i].highscore:GetTapNoteScore("TapNoteScore_W5"),
						multiscores[i].highscore:GetTapNoteScore("TapNoteScore_Miss")
					)
				end
			}
		--[[ -- this doesnt do anything useful, usually just displays january 1900, why do we need it
		LoadFont("Common normal") ..
			{
				Name = "date",
				InitCommand = function(self)
					self:xy(framex + 10, framey + 20 + ((i - 1) * spacing)):zoom(0.35):halign(0)
				end,
				UpdateNetScoreCommand = function(self)
					self:settext(multiscores[i].highscore:GetDate())
					self:visible(false)
				end
			}
		]]
	}
	return t
end

for i = 1, lines do
	t[#t + 1] = scoreitem(1, i)
end

return t
