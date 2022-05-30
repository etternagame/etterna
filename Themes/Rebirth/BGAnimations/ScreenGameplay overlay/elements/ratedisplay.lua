-- music rate. its the rate of the music

local rateTextSize = GAMEPLAY:getItemHeight("rateDisplayText")

return Def.ActorFrame {
    Name = "MusicRate",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.MusicRateX, MovableValues.MusicRateY)
        self:zoom(MovableValues.MusicRateZoom)
    end,

    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:zoom(rateTextSize)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
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
