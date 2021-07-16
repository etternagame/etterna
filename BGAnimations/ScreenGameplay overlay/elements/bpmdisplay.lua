local a = GAMESTATE:GetPlayerState():GetSongPosition()
local r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
local GetBPS = SongPosition.GetCurBPS

-- less copypasta
local function UpdateBPM(self)
	local bpm = GetBPS(a) * r
	self:GetChild("BPM"):settext(notShit.round(bpm, 2))
end

t[#t + 1] =
	Def.ActorFrame {
	Name = "BPMText",
	InitCommand = function(self)
		self:x(MovableValues.BPMTextX):y(MovableValues.BPMTextY):zoom(MovableValues.BPMTextZoom)
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then
            -- dont bother updating for single bpm files
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			self:GetChild("BPM"):settextf("%5.2f", GetBPS(a) * r)
		end
	end,
	DoneLoadingNextSongMessageCommand = function(self)
		self:queuecommand("Init")
	end,
	CurrentRateChangedMessageCommand = function(self)
		r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then
            -- dont bother updating for single bpm files
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			self:GetChild("BPM"):settextf("%5.2f", GetBPS(a) * r)
		end
	end,
	PracticeModeReloadMessageCommand = function(self)
		self:playcommand("CurrentRateChanged")
	end,

	LoadFont("Common Normal") .. {
        Name = "BPM",
        InitCommand = function(self)
            self:halign(0.5):zoom(0.40)
        end,
    },
}