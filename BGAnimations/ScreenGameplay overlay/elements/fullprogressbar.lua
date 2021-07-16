local width = SCREEN_WIDTH / 2 - 100
local height = 10
local alpha = 0.7

local replaySlider = isReplay and
	Widg.SliderBase {
		width = width,
		height = height,
		min = GAMESTATE:GetCurrentSteps():GetFirstSecond(),
		visible = true,
		max = GAMESTATE:GetCurrentSteps():GetLastSecond(),
		onInit = function(slider)
			slider.actor:diffusealpha(0)
		end,
		-- Change to onValueChangeEnd if this
		-- lags too much
		onValueChange = function(val)
			SCREENMAN:GetTopScreen():SetSongPosition(val)
		end
	} or
	Def.Actor {Name = "Nothing!"}

local t = Def.ActorFrame {
	Name = "FullProgressBar",
	InitCommand = function(self)
		self:xy(MovableValues.FullProgressBarX, MovableValues.FullProgressBarY)
		self:zoomto(MovableValues.FullProgressBarWidth, MovableValues.FullProgressBarHeight)
	end,
	replaySlider,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(width, height):diffuse(color("#666666")):diffusealpha(alpha)
		end
	},
	Def.SongMeterDisplay {
		InitCommand = function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth = width,
		Stream = Def.Quad {
			InitCommand = function(self)
				self:zoomy(height):diffuse(getMainColor("highlight"))
			end
		}
	},
	LoadFont("Common Normal") .. {
        -- title
        InitCommand = function(self)
            self:zoom(0.35):maxwidth(width * 2)
        end,
        BeginCommand = function(self)
            self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
        end,
        PracticeModeReloadMessageCommand = function(self)
            self:playcommand("Begin")
        end
    },
	LoadFont("Common Normal") .. {
        -- total time
        InitCommand = function(self)
            self:x(width / 2):zoom(0.35):maxwidth(width * 2):halign(1)
        end,
        BeginCommand = function(self)
            local ttime = GetPlayableTime()
            self:settext(SecondsToMMSS(ttime))
            self:diffuse(byMusicLength(ttime))
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            local ttime = GetPlayableTime()
            self:settext(SecondsToMMSS(ttime))
            self:diffuse(byMusicLength(ttime))
        end,
        CurrentRateChangedMessageCommand = function(self)
            local ttime = GetPlayableTime()
            self:settext(SecondsToMMSS(ttime))
            self:diffuse(byMusicLength(ttime))
        end,
        PracticeModeReloadMessageCommand = function(self)
            self:playcommand("CurrentRateChanged")
        end
    },
}

return t