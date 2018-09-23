--TODO: refactor this mess

t = Def.ActorFrame {}

local barXP1 = (capWideScale(get43size(384), 384)) - 153
local barYP1 = 310 + capWideScale(get43size(120), 120)

local playerDistY = 95

local barXP2 = 157
local barYP2 = barYP1 + playerDistY

local barWidth = capWideScale(get43size(333), 333) - (barXP1 - capWideScale(get43size(barXP1), barXP1))
local barHeight = 4
local animationDelay = 0
local animationLength = 1

local hsTableP1
local hsTableP2
local topScoreP1
local topScoreP2

t[#t + 1] =
	Def.Actor {
	BeginCommand = function(self)
		self:playcommand("Set")
	end,
	SetCommand = function(self)
		song = GAMESTATE:GetCurrentSong()
		if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
			hsTableP1 = getScoreList(PLAYER_1)
			if hsTableP1 ~= nil then
				topScoreP1 = getScoreFromTable(hsTableP1, 1)
			end
		end
		if GAMESTATE:IsPlayerEnabled(PLAYER_2) then
			hsTableP2 = getScoreList(PLAYER_2)
			if hsTableP2 ~= nil then
				topScoreP2 = getScoreFromTable(hsTableP2, 1)
			end
		end
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:playcommand("Set")
	end,
	CurrentStepsP1ChangedMessageCommand = function(self)
		self:playcommand("Set")
	end
}

t[#t + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(barXP1, barYP1)
	end,
	BeginCommand = function(self)
		if GAMESTATE:IsHumanPlayer(PLAYER_1) then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	PlayerJoinedMessageCommand = function(self, params)
		if params.Player == PLAYER_1 then
			self:visible(true)
		end
	end,
	PlayerUnjoinedMessageCommand = function(self, params)
		if params.Player == PLAYER_1 then
			self:visible(false)
		end
	end,
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:x(-2):zoom(0.30):halign(1)
			end,
			BeginCommand = function(self)
				self:settext("Judge:")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0):diffuse(color("#000000"))
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_1)
			local judge =
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP1, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W3") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W4") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W5") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_Miss")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_Miss"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_1)
			local judge =
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP1, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W3") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W4") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W5")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W5"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_1)
			local judge =
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP1, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W3") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W4")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W4"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_1)
			local judge =
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP1, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W3")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W3"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_1)
			local judge =
				getScoreTapNoteScore(topScoreP1, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP1, "TapNoteScore_W2")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W2"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_1)
			local judge = getScoreTapNoteScore(topScoreP1, "TapNoteScore_W1")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(getMainColor("highlight"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	}
}

t[#t + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(barXP2, barYP2)
	end,
	BeginCommand = function(self)
		if GAMESTATE:IsHumanPlayer(PLAYER_2) then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	PlayerJoinedMessageCommand = function(self, params)
		if params.Player == PLAYER_2 then
			self:visible(true)
		end
	end,
	PlayerUnjoinedMessageCommand = function(self, params)
		if params.Player == PLAYER_2 then
			self:visible(false)
		end
	end,
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:x(-2):zoom(0.30):halign(1)
			end,
			BeginCommand = function(self)
				self:settext("Judge:")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0):diffuse(color("#000000"))
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_2)
			local judge =
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP2, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W3") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W4") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W5") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_Miss")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_Miss"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_2)
			local judge =
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP2, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W3") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W4") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W5")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W5"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_2)
			local judge =
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP2, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W3") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W4")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W4"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_2)
			local judge =
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP2, "TapNoteScore_W2") +
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W3")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W3"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_2)
			local judge =
				getScoreTapNoteScore(topScoreP2, "TapNoteScore_W1") + getScoreTapNoteScore(topScoreP2, "TapNoteScore_W2")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(TapNoteScoreToColor("TapNoteScore_W2"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(barWidth, barHeight):halign(0)
		end,
		BeginCommand = function(self)
			self:glowshift():effectcolor1(color("1,1,1,0.2")):effectcolor2(color("1,1,1,0.5")):queuecommand("Set")
		end,
		SetCommand = function(self)
			local notes = getMaxNotes(PLAYER_2)
			local judge = getScoreTapNoteScore(topScoreP2, "TapNoteScore_W1")
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end
			--self:zoomx(0)
			self:sleep(animationDelay)
			self:smooth(animationLength)
			self:diffuse(getMainColor("highlight"))
			self:zoomx((judge / notes) * barWidth)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:stoptweening()
			self:queuecommand("Set")
		end
	}
}

return t
