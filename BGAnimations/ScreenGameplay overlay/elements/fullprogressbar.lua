-- progress bar that goes across the screen

local width = SCREEN_WIDTH / 2 - 100 / GAMEPLAY_SIZING_RATIO
local height = SCREEN_HEIGHT / 50
local alpha = 0.7
local isReplay = GAMESTATE:GetPlayerState():GetPlayerController() == "PlayerController_Replay"

local progressbarTextSize = 0.45 / GAMEPLAY_SIZING_RATIO

-- ternary logic
-- will become either nothing or a slider
-- used to seek in replays
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
		-- Change to onValueChangeEnd if this lags too much
		onValueChange = function(val)
			SCREENMAN:GetTopScreen():SetSongPosition(val)
		end
	} or
	Def.Actor {Name = "Nothing"}

return Def.ActorFrame {
	Name = "FullProgressBar",
	InitCommand = function(self)
		self:xy(MovableValues.FullProgressBarX, MovableValues.FullProgressBarY)
		self:zoomto(MovableValues.FullProgressBarWidth, MovableValues.FullProgressBarHeight)
	end,

	replaySlider,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:zoomto(width, height)
			self:diffuse(COLORS:getGameplayColor("FullProgressBarBG"))
			self:diffusealpha(alpha)
		end
	},
	Def.SongMeterDisplay {
		Name = "Progress",
		InitCommand = function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth = width,
		Stream = Def.Quad {
			InitCommand = function(self)
				self:zoomy(height)
				self:diffuse(COLORS:getGameplayColor("FullProgressBar"))
				self:diffusealpha(alpha)
			end
		}
	},
	LoadFont("Common Normal") .. {
        Name = "Title",
        InitCommand = function(self)
            self:zoom(progressbarTextSize)
			self:maxwidth((width * 0.8) / progressbarTextSize)
        end,
        BeginCommand = function(self)
            self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
        end,
        DoneLoadingNextSongMessageCommand = function(self)
			self:playcommand("Begin")
		end,
        PracticeModeReloadMessageCommand = function(self)
            self:playcommand("Begin")
        end
    },
	LoadFont("Common Normal") .. {
		Name = "SongLength",
        InitCommand = function(self)
            self:x(width / 2)
			self:halign(1)
			self:zoom(progressbarTextSize)
			self:maxwidth((width * 0.2) / progressbarTextSize)
        end,
        BeginCommand = function(self)
            local ttime = GetPlayableTime()
            self:settext(SecondsToMMSS(ttime))
            self:diffuse(colorByMusicLength(ttime))
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:playcommand("Begin")
        end,
        CurrentRateChangedMessageCommand = function(self)
            self:playcommand("Begin")
        end,
        PracticeModeReloadMessageCommand = function(self)
            self:playcommand("Begin")
        end
    },
}