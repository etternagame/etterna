-- music rate. its the rate of the music

local rateTextSize = GAMEPLAY:getItemHeight("rateDisplayText")

return Def.ActorFrame {
    Name = "MusicRate",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI(self)
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.MusicRateX, MovableValues.MusicRateY)
        self:zoom(MovableValues.MusicRateZoom)
    end,

    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:zoom(rateTextSize)
            self:playcommand("SetRate")
        end,
        SetRateCommand = function(self)
            self:settext(getCurRateDisplayString())
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:playcommand("SetRate")
        end,
        CurrentRateChangedMessageCommand = function(self)
            self:playcommand("SetRate")
        end
    },
}
