-- the bpm display. it displays the bpm

-- reset the update function and stuff
-- optimization: dont update for files with 1 bpm because the bpm doesnt change
local function initbpm(self)
	local r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
	local a = GAMESTATE:GetPlayerState():GetSongPosition()
	local GetBPS = SongPosition.GetCurBPS
	if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then
		self:SetUpdateFunction(function(self)
			local bpm = GetBPS(a) * r
			self:GetChild("BPM"):settext(notShit.round(bpm, 2))
		end)
		self:SetUpdateRate(0.5)
	else
		self:SetUpdateFunction(nil)
		self:GetChild("BPM"):settextf("%5.2f", GetBPS(a) * r)
	end
end
-------

local bpmTextSize = 0.4 / GAMEPLAY_SIZING_RATIO

return Def.ActorFrame {
	Name = "BPMText",
	InitCommand = function(self)
		self:xy(MovableValues.BPMTextX, MovableValues.BPMTextY)
		self:zoom(MovableValues.BPMTextZoom)
		initbpm(self)
	end,
	CurrentRateChangedMessageCommand = function(self)
		initbpm(self)
	end,
	PracticeModeReloadMessageCommand = function(self)
		self:playcommand("CurrentRateChanged")
	end,
	DoneLoadingNextSongMessageCommand = function(self)
		self:queuecommand("Init")
	end,

	LoadFont("Common Normal") .. {
        Name = "BPM",
        InitCommand = function(self)
            self:halign(0.5):zoom(bpmTextSize)
        end,
    },
}