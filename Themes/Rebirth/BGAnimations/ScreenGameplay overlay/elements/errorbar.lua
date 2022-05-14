-- Number of bars. Older bars will refresh if judgments/barDuration exceeds this value.
local barcount = playerConfig:get_data().ErrorBarCount
-- Width of the bars.
local barWidth = GAMEPLAY:getItemWidth("errorBarBarWidth")
-- Time duration in seconds before the ticks fade out. Doesn't need to be higher than 1. Maybe if you have 300 bars I guess.
local barDuration = 0.75

if barcount > 50 then barDuration = barcount / 50 end -- just procedurally set the duration if we pass 50 bars

-- regular bars
local currentbar = 1 -- so we know which error bar we need to update
local ingots = {} -- references to the error bars

-- ewma vars
local alpha = 0.07 -- this is not opacity. this is a math number thing
local avg
local lastAvg

-- to relate an offset to position within the bar
-- max offset is 180ms
local wscale = MovableValues.ErrorBarWidth / 180

local earlylateTextSize = GAMEPLAY:getItemHeight("errorBarText")

-- the error bar can either load as Regular or Exponential Weighted Moving Average
-- Regular loads a certain number of bars and places them continuously
-- EWMA loads 1 bar and places it according to the EWMA of the previous n taps
local errorbarType = playerConfig:get_data().ErrorBar == 1 and "Regular" or "EWMA"

local translations = {
    ErrorLate = THEME:GetString("ScreenGameplay", "ErrorBarLate"),
    ErrorEarly = THEME:GetString("ScreenGameplay", "ErrorBarEarly"),
}

-- procedurally generated error bars
local function smeltErrorBar(index)
    return Def.Quad {
        Name = index,
        InitCommand = function(self)
            self:zoomto(barWidth, MovableValues.ErrorBarHeight)
            self:diffusealpha(0)
        end,
        UpdateErrorBarCommand = function(self, params)
            if not params or params.judgeCurrent == nil or params.judgeOffset == nil then return end
            self:finishtweening()
            self:diffusealpha(1)
            -- now make it the color for this new judgment
            self:diffuse(colorByJudgment(params.judgeCurrent))
            -- and set up the position
            if MovableValues and MovableValues.ErrorBarX then
                self:x(params.judgeOffset * wscale)
                self:zoomtoheight(MovableValues.ErrorBarHeight)
            end
            self:linear(barDuration)
            self:diffusealpha(0)
        end,
        PracticeModeResetMessageCommand = function(self)
            self:diffusealpha(0)
        end
    }
end

local t = Def.ActorFrame {
    Name = "ErrorBar",
    InitCommand = function(self)
        if errorbarType == "Regular" then
            for i = 1, barcount do
                ingots[#ingots + 1] = self:GetChild(i)
            end
        else
            avg = 0
            lastAvg = 0
        end
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {5,1},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY)
        wscale = MovableValues.ErrorBarWidth / 180
    end,
    SpottedOffsetCommand = function(self, params)
        if errorbarType == "Regular" then
            if params and params.judgeOffset ~= nil then
                currentbar = ((currentbar) % barcount) + 1
                ingots[currentbar]:playcommand("UpdateErrorBar", params) -- Update the next bar in the queue
            end
        end
    end,
    DootCommand = function(self)
        self:RemoveChild("DestroyMe")
        self:RemoveChild("DestroyMe2")
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            --registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
            self:diffusealpha(0)
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:zoomto(MovableValues.ErrorBarWidth, MovableValues.ErrorBarHeight)
        end,
    },
    Def.Quad {
        Name = "Center",
        InitCommand = function(self)
            self:diffuse(COLORS:getGameplayColor("ErrorBarCenter"))
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:zoomto(2, MovableValues.ErrorBarHeight)
        end,
    },

    -- Indicates which side is which (early/late) These should be destroyed after the song starts.
    LoadFont("Common Normal") .. {
        Name = "DestroyMe",
        InitCommand = function(self)
            self:zoom(earlylateTextSize)
        end,
        BeginCommand = function(self)
            self:settext(translations["ErrorLate"])
            self:diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0)
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:x(MovableValues.ErrorBarWidth / 4)
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "DestroyMe2",
        InitCommand = function(self)
            self:zoom(earlylateTextSize)
        end,
        BeginCommand = function(self)
            self:settext(translations["ErrorEarly"])
            self:diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0):queuecommand("Doot")
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:x(-MovableValues.ErrorBarWidth / 4)
        end,
        DootCommand = function(self)
            self:GetParent():queuecommand("Doot")
        end
    },
}

if errorbarType == "EWMA" then
    t[#t+1] = Def.Quad {
        Name = "WeightedBar",
        InitCommand = function(self)
            self:diffuse(COLORS:getGameplayColor("ErrorBarEWMABar"))
            self:diffusealpha(1)
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:zoomto(barWidth, MovableValues.ErrorBarHeight)
        end,
        SpottedOffsetCommand = function(self, params)
            if params and params.judgeOffset ~= nil then
                avg = alpha * params.judgeOffset + (1 - alpha) * lastAvg
                lastAvg = avg
                self:x(avg * wscale)
            end
        end
    }
end

if errorbarType == "Regular" then
    for i = 1, barcount do
        t[#t+1] = smeltErrorBar(i)
    end
end

return t
