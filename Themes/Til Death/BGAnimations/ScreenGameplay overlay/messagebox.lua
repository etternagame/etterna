local settext = BitmapText.settext
local isPractice = GAMESTATE:IsPracticeMode()

local function highlight(self)
	self:queuecommand("Highlight")
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.2)
	else
		self:diffusealpha(1)
	end
end

local function transStr(line)
	return THEME:GetString("CustomizeGameplay", line)
end

return Def.ActorFrame {
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(MovableInput)
		self:SetUpdateFunction(highlight)
	end,
	OffCommand = function(self)
		-- save CustomizeGameplay changes when leaving the screen
		playerConfig:save(pn_to_profile_slot(PLAYER_1))
	end,
	Def.BitmapText {
		Name = "message",
		Font = "Common Normal",
		InitCommand = function(self)
			Movable.message = self
			self:horizalign(left):vertalign(top):shadowlength(2):xy(10, 20):zoom(.5):visible(false)
		end
	},
	Def.BitmapText {
		Name = "Instructions",
		Font = "Common Normal",
		InitCommand = function(self)
			self:horizalign(left):vertalign(top):xy(SCREEN_WIDTH - 240, 20):zoom(.4):visible(true)
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end,
		OnCommand = function(self)
			local text = {
				transStr("InstructionAutoplay"),
				transStr("InstructionPressKeys"),
				transStr("InstructionCancel"),
				"1: "..transStr("JudgmentPosition"),
				"2: "..transStr("JudgmentSize"),
				"3: "..transStr("ComboPosition"),
				"4: "..transStr("ComboSize"),
				"5: "..transStr("ErrorBarPosition"),
				"6: "..transStr("ErrorBarSize"),
				"7: "..transStr("TargetTrackerPosition"),
				"8: "..transStr("TargetTrackerSize"),
				"9: "..transStr("FullProgressBarPosition"),
				"0: "..transStr("FullProgressBarSize"),
				"q: "..transStr("MiniProgressBarPosition"),
				"w: "..transStr("DisplayPercentPosition"),
				"e: "..transStr("DisplayPercentSize"),
				"r: "..transStr("NotefieldPosition"),
				"t: "..transStr("NotefieldSize"),
				"y: "..transStr("NPSDisplayPosition"),
				"u: "..transStr("NPSDisplaySize"),
				"i: "..transStr("NPSGraphPosition"),
				"o: "..transStr("NPSGraphSize"),
				"p: "..transStr("JudgeCounterPosition"),
				"a: "..transStr("LeaderboardPosition"),
				"s: "..transStr("LeaderboardSize"),
				"d: "..transStr("LeaderboardSpacing"),
				"f: "..transStr("ReplayButtonPosition"),
				--"g: Replay Buttons Size",
				"h: "..transStr("ReplayButtonSpacing"),
				"j: "..transStr("LifebarPosition"),
				"k: "..transStr("LifebarSize"),
				"l: "..transStr("LifebarRotation"),
				"x: "..transStr("BPMPosition"),
				"c: "..transStr("BPMSize"),
				"v: "..transStr("RatePosition"),
				"b: "..transStr("RateSize")
			}
			if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCover ~= 0 then
				local selectStr = THEME:GetString("GameButton", "Select")
				table.insert(text, selectStr..": "..transStr("LaneCoverHeight"))
			end
			if isPractice then
				table.insert(text, "z: "..transStr("DensityGraphPosition"))
			end
			self:settext(table.concat(text, "\n"))
		end
	}
}
