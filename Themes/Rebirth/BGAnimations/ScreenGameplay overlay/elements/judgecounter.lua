-- the judge counter. counts judges

-- judgments to display and the order for them
local jdgT = {
    "TapNoteScore_W1",
    "TapNoteScore_W2",
    "TapNoteScore_W3",
    "TapNoteScore_W4",
    "TapNoteScore_W5",
    "TapNoteScore_Miss",
    "HoldNoteScore_Held",
    "HoldNoteScore_LetGo",
}

local spacing = GAMEPLAY:getItemHeight("judgeDisplayVerticalSpacing") -- Spacing between the judgetypes
local frameWidth = GAMEPLAY:getItemWidth("judgeDisplay") -- Width of the Frame
local frameHeight = ((#jdgT + 1) * spacing) -- Height of the Frame
local judgeFontSize = GAMEPLAY:getItemHeight("judgeDisplayJudgeText")
local countFontSize = GAMEPLAY:getItemHeight("judgeDisplayCountText")

local function recalcSizing()
    spacing = GAMEPLAY:getItemHeight("judgeDisplayVerticalSpacing") * MovableValues.JudgeCounterSpacing
    frameWidth = GAMEPLAY:getItemWidth("judgeDisplay")
    frameHeight = ((#jdgT + 1) * spacing)
end

-- the text actors for each judge count
local judgeCounts = {}

local t = Def.ActorFrame {
    Name = "JudgeCounter",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
            spacingInc = {0.1,0.05},
        })
    end,
    BeginCommand = function(self)
        for _, j in ipairs(jdgT) do
            judgeCounts[j] = self:GetChild(j .. "count")
        end
    end,
    SetUpMovableValuesMessageCommand = function(self)
        recalcSizing()

        self:xy(MovableValues.JudgeCounterX, MovableValues.JudgeCounterY)
        self:zoomto(MovableValues.JudgeCounterWidth, MovableValues.JudgeCounterHeight)
        self:playcommand("FinishSetUpMovableValues")
    end,
    SpottedOffsetCommand = function(self, params)
        if params == nil then return end
        local cur = params.judgeCurrent
        if cur and judgeCounts[cur] ~= nil then
            judgeCounts[cur]:settext(params.judgeCount)
        end
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:diffuse(COLORS:getGameplayColor("PrimaryBackground"))
            self:diffusealpha(0.4)
        end,
        FinishSetUpMovableValuesMessageCommand = function(self)
            self:zoomto(frameWidth, frameHeight)
        end,
    },
}

local function makeJudgeText(judge, index)
    return LoadFont("Common normal") .. {
        Name = judge .. "text",
        InitCommand = function(self)
            self:halign(0)
            self:settext(getShortJudgeStrings(judge))
            self:diffuse(COLORS:colorByJudgment(judge))
            self:diffusealpha(1)
        end,
        FinishSetUpMovableValuesMessageCommand = function(self)
            self:xy(-frameWidth / 2 + 5, -frameHeight / 2 + (index * spacing))
            self:zoom(judgeFontSize)
        end,
    }
end

local function makeJudgeCount(judge, index)
    return LoadFont("Common Normal") .. {
        Name = judge .. "count",
        InitCommand = function(self)
            self:halign(1)
            self:settext(0)
            self:diffuse(COLORS:getGameplayColor("PrimaryText"))
            self:diffusealpha(1)
        end,
        PracticeModeResetMessageCommand = function(self)
            self:settext(0)
        end,
        FinishSetUpMovableValuesMessageCommand = function(self)
            self:xy(frameWidth / 2 - 5, -frameHeight / 2 + (index * spacing))
            self:zoom(countFontSize)
        end,
    }
end

for i, j in ipairs(jdgT) do
    t[#t+1] = makeJudgeText(j, i)
    t[#t+1] = makeJudgeCount(j, i)
end

return t
