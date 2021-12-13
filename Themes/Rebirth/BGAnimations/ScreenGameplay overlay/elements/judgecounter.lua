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

-- the text actors for each judge count
local judgeCounts = {}

local t = Def.ActorFrame {
    Name = "JudgeCounter",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
        })
    end,
    BeginCommand = function(self)
        for _, j in ipairs(jdgT) do
            judgeCounts[j] = self:GetChild(j .. "count")
        end
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.JudgeCounterX, MovableValues.JudgeCounterY)
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
            self:zoomto(frameWidth, frameHeight)
            self:diffuse(COLORS:getGameplayColor("PrimaryBackground"))
            self:diffusealpha(0.4)
        end,
    },
}

local function makeJudgeText(judge, index)
    return LoadFont("Common normal") .. {
        Name = judge .. "text",
        InitCommand = function(self)
            self:xy(-frameWidth / 2 + 5, -frameHeight / 2 + (index * spacing))
            self:halign(0)
            self:zoom(judgeFontSize)
            self:settext(getShortJudgeStrings(judge))
            self:diffuse(COLORS:colorByJudgment(judge))
            self:diffusealpha(1)
        end,
    }
end

local function makeJudgeCount(judge, index)
    return LoadFont("Common Normal") .. {
        Name = judge .. "count",
        InitCommand = function(self)
            self:halign(1)
            self:xy(frameWidth / 2 - 5, -frameHeight / 2 + (index * spacing))
            self:zoom(countFontSize)
            self:settext(0)
            self:diffuse(COLORS:getGameplayColor("PrimaryText"))
            self:diffusealpha(1)
        end,
        PracticeModeResetMessageCommand = function(self)
            self:settext(0)
        end,
    }
end

for i, j in ipairs(jdgT) do
    t[#t+1] = makeJudgeText(j, i)
    t[#t+1] = makeJudgeCount(j, i)
end

return t
