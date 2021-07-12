-- Everything relating to the gameplay screen is gradually moved to WifeJudgmentSpotting.lua
local inReplay = GAMESTATE:GetPlayerState():GetPlayerController() == "PlayerController_Replay"
local inCustomize = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local isPractice = GAMESTATE:IsPracticeMode()

if not inReplay and not inCustomize and not isPractice then
	Arch.setCursorVisible(false)
end

local t = Def.ActorFrame {}
t[#t + 1] = LoadActor("WifeJudgmentSpotting")
t[#t + 1] = LoadActor("titlesplash")
t[#t + 1] = LoadActor("leaderboard")
if inReplay then
	t[#t + 1] = LoadActor("replayscrolling")
end
if inCustomize then
	t[#t + 1] = LoadActor("messagebox")
end
if inReplay or inCustomize or isPractice then
	t[#t + 1] = LoadActor("../_cursor")
end

local snm = Var("LoadingScreen")
if snm ~= nil and snm == "ScreenGameplaySyncMachine" then
	local status = 0
	local maxstatus = 3 -- when status hits this fade the notefield to 0
	local judgeThreshold = Enum.Reverse(TapNoteScore)[ComboContinue()]
	local notdoingit = true
	local heeby = nil
	t[#t+1] = Def.ActorFrame {
		Name = "spooker",
		JudgmentMessageCommand = function(self, params)
			if params.HoldNoteScore then return end
			if params.TapNoteScore then
				local enum  = Enum.Reverse(TapNoteScore)[params.TapNoteScore]
				if enum < judgeThreshold and enum > 3 then
					-- if a cb
					status = 0
					self:playcommand("nowaitdont")
				else
					-- if not a cb
					status = status + 1
					if status >= maxstatus and notdoingit then
						self:playcommand("doit")
					end
				end
			end
		end,
		DoneLoadingNextSongMessageCommand = function(self)
			self:playcommand("nowaitdont")
		end,
		nowaitdontCommand = function(self)
			notdoingit = true
			if heeby then
				self:stoptweening()
				heeby:diffusealpha(1)
				-- this is the only other line that matters
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Song"):Stealth(0)
			end
		end,
		doitCommand = function(self)
			notdoingit = false
			if heeby then
				self:finishtweening()
				heeby:smooth(1)
				heeby:diffusealpha(0)
				-- this is the only line that matters
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Song"):Stealth(1)
			end
		end,

		Def.ActorProxy {
			Name = "spookest",
			BeginCommand = function(self)
				local nf = SCREENMAN:GetTopScreen():GetChild("PlayerP1"):GetChild("NoteField")
				heeby = self
				if nf then
					local b4 = nf:GetX()
					local b42 = nf:GetY()
					nf:x(SCREEN_WIDTH * 100)
					local yes = 1
					-- the explanation of this line is left as an exercise to the reader
					self:x(-SCREEN_WIDTH * 100 + b4 + SCREEN_CENTER_X):visible(yes):y(0):addy(SCREEN_CENTER_Y + b42 - 0)
					self:SetTarget(nf)
				end
			end,
		},

		LoadFont("Common Large") .. {
			Name = "Instruction",
			InitCommand = function(self)
				self:settext("Keep tapping to the beat")
				self:zoom(0.6)
				self:xy(SCREEN_CENTER_X, SCREEN_HEIGHT/5)
				self:diffusealpha(0)
			end,
			nowaitdontCommand = function(self)
				self:stoptweening()
				self:diffusealpha(0)
			end,
			doitCommand = function(self)
				self:finishtweening()
				self:diffusealpha(0)
				self:smooth(2)
				self:diffusealpha(1)
			end,
		},
		LoadFont("Common Large") .. {
			Name = "Instrucion",
			InitCommand = function(self)
				self:settext("Keep tapping to the beat")
				self:zoom(0.6)
				self:xy(SCREEN_CENTER_X, SCREEN_HEIGHT/5*4)
				self:diffusealpha(0)
			end,
			nowaitdontCommand = function(self)
				self:stoptweening()
				self:diffusealpha(0)
			end,
			doitCommand = function(self)
				self:finishtweening()
				self:diffusealpha(0)
				self:smooth(3)
				self:diffusealpha(1)
			end,
		}
	}
end

return t
