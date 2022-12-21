-- for portability
-- we use this file in evaluation and in the local section of the scores tab in selectmusic
-- requires sizing to be provided on init as tables
local sizing = Var("sizing")
if sizing == nil then sizing = {} end
local textSizeMultiplier = Var("textSizeMultiplier") or 1

-- list of judgments to display the bar/counts for
local judgmentsChosen = {
    "TapNoteScore_W1", -- marvelous
    "TapNoteScore_W2", -- perfect
    "TapNoteScore_W3", -- great
    "TapNoteScore_W4", -- good
    "TapNoteScore_W5", -- bad
    "TapNoteScore_Miss", -- miss
}

local judgmentTextZoom = 0.63 * textSizeMultiplier
local judgmentCountZoom = 0.63 * textSizeMultiplier
local judgmentPercentZoom = 0.325 * textSizeMultiplier
local judgmentCountPercentBump = 1 -- a bump in position added to the Count and Percent for spacing

local textzoomFudge = 5
-- this number should stay the same as ApproachSeconds under metrics.ini [RollingNumbersJudgmentPercentage]
-- (or the associated RollingNumbers classes used in this file)
local animationSeconds = 0.5

-- make this not invisible to ... you know
local textEmbossColor = color("0,0,0,0")

local totalTaps = 0
local t = Def.ActorFrame {
    Name = "JudgmentBarParentFrame",
    SetCommand = function(self, params)
        totalTaps = 0
        for i, j in ipairs(judgmentsChosen) do
            totalTaps = totalTaps + params.score:GetTapNoteScore(j)
        end
    end
}
local function makeJudgment(i)
    local jdg = judgmentsChosen[i]
    local count = 0

    return Def.ActorFrame {
        Name = "Judgment_"..i,
        InitCommand = function(self)
            -- finds the top of every bar given the requested spacing and the height of each bar within the allotted space
            self:y((((i-1) * sizing.JudgmentBarHeight + (i-1) * sizing.JudgmentBarSpacing) / sizing.JudgmentBarAllottedSpace) * sizing.JudgmentBarAllottedSpace)
        end,
        SetCommand = function(self, params)
            if params.usingCustomWindows then
                local lastSnapshot = REPLAYS:GetActiveReplay():GetLastReplaySnapshot()
                count = lastSnapshot:GetJudgments()[jdg:gsub("TapNoteScore_", "")]
                return
            end

            if params.score ~= nil then
                if params.judgeSetting ~= nil and params.score:HasReplayData() then
                    count = getRescoredJudge(params.score:GetOffsetVector(), params.judgeSetting, i)
                else
                    count = params.score:GetTapNoteScore(jdg)
                end
            else
                count = 0
            end
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(sizing.JudgmentBarLength, sizing.JudgmentBarHeight)
                self:diffusealpha(0.5)
                registerActorToColorConfigElement(self, "judgment", jdg)
            end
        },
        Def.Quad {
            Name = "Fill",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(0, sizing.JudgmentBarHeight)
                self:diffusealpha(0.5)
                registerActorToColorConfigElement(self, "judgment", jdg)
            end,
            SetCommand = function(self, params)
                self:finishtweening()
                self:smooth(animationSeconds)
                if params.score == nil then
                    self:zoomx(0)
                    return
                end
                local percent = count / totalTaps
                self:zoomx(sizing.JudgmentBarLength * percent)
            end
        },
        LoadFont("Common Large") .. {
            Name = "Name",
            InitCommand = function(self)
                self:halign(0)
                self:xy(sizing.JudgmentNameLeftGap, sizing.JudgmentBarHeight / 2)
                self:zoom(judgmentTextZoom)
                self:strokecolor(textEmbossColor)
                -- allow 3/4 of the judgment area between the number alignment and the name alignment
                self:maxwidth((sizing.JudgmentBarLength - sizing.JudgmentNameLeftGap - sizing.JudgmentCountRightGap - judgmentCountPercentBump) / 4 * 3 / judgmentTextZoom)
                self:settext(getJudgeStrings(ms.JudgeCount[i]))
                registerActorToColorConfigElement(self, "judgment", "TextOverBars")
            end,
            SetCommand = function(self, params)
                if params and params.usingCustomWindows then
                    self:settext(getCustomWindowConfigJudgmentName(jdg))
                else
                    self:settext(getJudgeStrings(ms.JudgeCount[i]))
                end
            end
        },
        Def.RollingNumbers {
            Name = "Count",
            Font = "Common Large",
            InitCommand = function(self)
                self:Load("RollingNumbersJudgmentNoLead")
                self:halign(1)
                self:xy(sizing.JudgmentBarLength - sizing.JudgmentCountRightGap - judgmentCountPercentBump, sizing.JudgmentBarHeight / 2)
                self:zoom(judgmentCountZoom)
                self:strokecolor(textEmbossColor)
                -- allow 1/4 of the judgment area between the number alignment and the name alignment
                self:maxwidth((sizing.JudgmentBarLength - sizing.JudgmentNameLeftGap - sizing.JudgmentCountRightGap - judgmentCountPercentBump) / 4 / judgmentTextZoom)
                self:targetnumber(0)
                registerActorToColorConfigElement(self, "judgment", "TextOverBars")
            end,
            SetCommand = function(self, params)
                if params.score == nil then
                    self:targetnumber(0)
                    return
                end
                self:targetnumber(count)
            end
        },
        Def.RollingNumbers {
            Name = "Percentage",
            Font = "Common Large",
            InitCommand = function(self)
                self:Load("RollingNumbersJudgmentPercentage")
                self:halign(0)
                self:xy(sizing.JudgmentBarLength - sizing.JudgmentCountRightGap + judgmentCountPercentBump, sizing.JudgmentBarHeight / 2)
                self:zoom(judgmentPercentZoom)
                self:maxwidth((sizing.JudgmentCountRightGap - judgmentCountPercentBump) / judgmentPercentZoom - textzoomFudge)
                self:strokecolor(textEmbossColor)
                self:targetnumber(0)
                registerActorToColorConfigElement(self, "judgment", "TextOverBars")
            end,
            SetCommand = function(self, params)
                if params.score == nil then
                    self:targetnumber(0)
                    return
                end
                if totalTaps == 0 then
                    self:targetnumber(100)
                else
                    local percent = count / totalTaps * 100
                    self:targetnumber(percent)
                end
            end
        }
    }
end
for i = 1, #judgmentsChosen do
    t[#t+1] = makeJudgment(i)
end

return t