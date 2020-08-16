local t = Def.ActorFrame {
    Name = "MainDisplayFile",
    OnCommand = function(self)
        local score = SCOREMAN:GetMostRecentScore()
        if not score then
            score = SCOREMAN:GetTempReplayScore()
        end

        --- propagate set command through children with the song
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), steps = GAMESTATE:GetCurrentSteps(), score = score})
    end
}

local ratios = {
    LeftGap = 78 / 1920,
    UpperGap = 135 / 1080, -- from top edge of screen to edge of bg
    Width = 1765 / 1920,
    Height = 863 / 1080,
    LipLeftGap = 758 / 1920, -- the lip starts at the end of the banner
    LipHeight = 50 / 1080,
    LipLength = 1007 / 1920, -- runs to the right end of the frame

    GraphLeftGap = 18 / 1920,
    GraphWidth = 739 / 1920, -- this must be the same as in metrics [GraphDisplay/ComboGraph]
    GraphBannerGap = 16 / 1080, -- from bottom of banner to top of graph
    BannerLeftGap = 18 / 1920,
    BannerHeight = 228 / 1080,
    BannerWidth = 739 / 1920,
    LifeGraphHeight = 71 / 1080, -- this must be the same as in metrics [GraphDisplay]
    ComboGraphHeight = 16 / 1080, -- this must be the same as in metrics [ComboGraph]

    DividerThickness = 2 / 1080,
    LeftDividerLeftGap = 18 / 1920,
    LeftDividerLength = 739 / 1920,

    LeftDivider1UpperGap = 338 / 1080,
    LeftDivider2UpperGap = 399 / 1080,

    ModTextLeftGap = 19 / 1920,
    -- modtext Y pos is half between the 2 dividers.

    JudgmentBarLeftGap = 18 / 1920, -- edge of frame to left of bar
    JudgmentBarUpperGap = 408 / 1080, -- top edge of from to top edge of top bar
    JudgmentBarHeight = 44 / 1080,
    JudgmentBarAllottedSpace = 264 / 1080, -- top of top bar to top of bottom bar (valign 0)
    JudgmentBarLength = 739 / 1920,
    JudgmentBarSpacing = 7 / 1080, -- the emptiness between judgments
    JudgmentNameLeftGap = 25 / 1920, -- from left edge of bar to left edge of text
    JudgmentCountRightGap = 95 / 1920, -- from right edge of bar to left edge of percentage, right edge of count
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    UpperGap = ratios.UpperGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    LipLeftGap = ratios.LipLeftGap * SCREEN_WIDTH,
    LipHeight = ratios.LipHeight * SCREEN_HEIGHT,
    LipLength = ratios.LipLength * SCREEN_WIDTH,
    GraphLeftGap = ratios.GraphLeftGap * SCREEN_WIDTH,
    GraphWidth = ratios.GraphWidth * SCREEN_WIDTH,
    GraphBannerGap = ratios.GraphBannerGap * SCREEN_HEIGHT,
    BannerLeftGap = ratios.BannerLeftGap * SCREEN_WIDTH,
    BannerHeight = ratios.BannerHeight * SCREEN_HEIGHT,
    BannerWidth = ratios.BannerWidth * SCREEN_WIDTH,
    LifeGraphHeight = ratios.LifeGraphHeight * SCREEN_HEIGHT,
    ComboGraphHeight = ratios.ComboGraphHeight * SCREEN_HEIGHT,
    DividerThickness = ratios.DividerThickness * SCREEN_HEIGHT,
    LeftDividerLeftGap = ratios.LeftDividerLeftGap * SCREEN_WIDTH,
    LeftDividerLength = ratios.LeftDividerLength * SCREEN_WIDTH,
    LeftDivider1UpperGap = ratios.LeftDivider1UpperGap * SCREEN_HEIGHT,
    LeftDivider2UpperGap = ratios.LeftDivider2UpperGap * SCREEN_HEIGHT,
    ModTextLeftGap = ratios.ModTextLeftGap * SCREEN_WIDTH,
    JudgmentBarLeftGap = ratios.JudgmentBarLeftGap * SCREEN_WIDTH,
    JudgmentBarUpperGap = ratios.JudgmentBarUpperGap * SCREEN_HEIGHT,
    JudgmentBarHeight = ratios.JudgmentBarHeight * SCREEN_HEIGHT,
    JudgmentBarAllottedSpace = ratios.JudgmentBarAllottedSpace * SCREEN_HEIGHT,
    JudgmentBarLength = ratios.JudgmentBarLength * SCREEN_WIDTH,
    JudgmentBarSpacing = ratios.JudgmentBarSpacing * SCREEN_HEIGHT,
    JudgmentNameLeftGap = ratios.JudgmentNameLeftGap * SCREEN_WIDTH,
    JudgmentCountRightGap = ratios.JudgmentCountRightGap * SCREEN_WIDTH
}

-- list of judgments to display the bar/counts for
local judgmentsChosen = {
    "TapNoteScore_W1", -- marvelous
    "TapNoteScore_W2", -- perfect
    "TapNoteScore_W3", -- great
    "TapNoteScore_W4", -- good
    "TapNoteScore_W5", -- bad
    "TapNoteScore_Miss", -- miss
}

local modTextZoom = 1
local judgmentTextZoom = 0.95
local judgmentCountZoom = 0.95
local judgmentPercentZoom = 0.6
local judgmentCountPercentBump = 1 -- a bump in position added to the Count and Percent for spacing
local textzoomFudge = 5

local function judgmentBars()
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

        return Def.ActorFrame {
            Name = "Judgment_"..i,
            InitCommand = function(self)
                -- finds the top of every bar given the requested spacing and the height of each bar within the allotted space
                self:y((((i-1) * actuals.JudgmentBarHeight + (i-1) * actuals.JudgmentBarSpacing) / actuals.JudgmentBarAllottedSpace) * actuals.JudgmentBarAllottedSpace)
            end,

            Def.Quad {
                Name = "BG",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoomto(actuals.JudgmentBarLength, actuals.JudgmentBarHeight)
                    self:diffuse(byJudgment(jdg))
                    self:diffusealpha(0.7)
                end
            },
            Def.Quad {
                Name = "Fill",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoomto(0, actuals.JudgmentBarHeight)
                    self:diffuse(byJudgment(jdg))
                end,
                SetCommand = function(self, params)
                    local percent = params.score:GetTapNoteScore(jdg) / totalTaps
                    self:zoomx(actuals.JudgmentBarLength * percent)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0)
                    self:xy(actuals.JudgmentNameLeftGap, actuals.JudgmentBarHeight / 2)
                    self:zoom(judgmentTextZoom)
                    --self:maxwidth()
                    self:settext(ms.JudgeCount[i])
                end
            },
            Def.RollingNumbers {
                Name = "Count",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:Load("RollingNumbersNoLead")
                    self:halign(1)
                    self:xy(actuals.JudgmentBarLength - actuals.JudgmentCountRightGap - judgmentCountPercentBump, actuals.JudgmentBarHeight / 2)
                    self:zoom(judgmentCountZoom)
                    self:targetnumber(0)
                end,
                SetCommand = function(self, params)
                    local count = params.score:GetTapNoteScore(jdg)
                    self:targetnumber(count)
                end
            },
            Def.RollingNumbers {
                Name = "Percentage",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:Load("RollingNumbersJudgmentPercentage")
                    self:halign(0)
                    self:xy(actuals.JudgmentBarLength - actuals.JudgmentCountRightGap + judgmentCountPercentBump, actuals.JudgmentBarHeight / 2)
                    self:zoom(judgmentPercentZoom)
                    self:targetnumber(0)
                end,
                SetCommand = function(self, params)
                    local percent = params.score:GetTapNoteScore(jdg) / totalTaps * 100
                    self:targetnumber(percent)
                end
            }
        }
    end
    for i = 1, #judgmentsChosen do
        t[#t+1] = makeJudgment(i)
    end

    return t
end

t[#t+1] = Def.ActorFrame {
    Name = "OwnerFrame",
    InitCommand = function(self)
        self:xy(actuals.LeftGap, actuals.UpperGap)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.Width, actuals.Height)
            self:diffuse(color("#111111"))
            self:diffusealpha(0.75)
        end
    },
    Def.Quad {
        Name = "BGLip",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:x(actuals.LipLeftGap)
            self:zoomto(actuals.LipLength, actuals.LipHeight)
            self:diffuse(color("#111111"))
        end
    },
    Def.Quad {
        Name = "LeftUpperDivider",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.LeftDividerLength, actuals.DividerThickness)
            self:xy(actuals.LeftDividerLeftGap, actuals.LeftDivider1UpperGap)
        end
    },
    Def.Quad {
        Name = "LeftLowerDivider",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.LeftDividerLength, actuals.DividerThickness)
            self:xy(actuals.LeftDividerLeftGap, actuals.LeftDivider2UpperGap)
        end
    },


    Def.Sprite {
        Name = "Banner",
        InitCommand = function(self)
            self:x(actuals.BannerLeftGap)
            self:valign(0):halign(0)
            self:scaletoclipped(actuals.BannerWidth, actuals.BannerHeight)
        end,
        SetCommand = function(self, params)
            self:finishtweening()
            self:smooth(0.05)
            self:diffusealpha(1)
            if params.song then
                local bnpath = params.song:GetBannerPath()
                if not bnpath then
                    bnpath = THEME:GetPathG("Common", "fallback banner")
                end
                self:LoadBackground(bnpath)
            else
                local bnpath = THEME:GetPathG("Common", "fallback banner")
                self:LoadBackground(bnpath)
            end
        end
    },
    Def.GraphDisplay {
        Name = "LifeGraph",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:xy(actuals.GraphLeftGap, actuals.GraphBannerGap + actuals.BannerHeight)
            -- due to reasons, the sizing for this is in metrics [GraphDisplay]
            -- we override them with the following zoomto
            -- so the ones in metrics can be anything....
            -- i don't like that
            self:Load("GraphDisplay")
            self:zoomto(actuals.GraphWidth, actuals.LifeGraphHeight)

            -- hide the max life line and its dots (why does this exist)
            self:GetChild("Line"):diffusealpha(0)
        end,
        SetCommand = function(self, params)
            local ss = SCREENMAN:GetTopScreen():GetStageStats()
            self:Set(ss, ss:GetPlayerStageStats(PLAYER_1))
        end
    },
    Def.ComboGraph {
        Name = "ComboGraph",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:xy(actuals.GraphLeftGap, actuals.GraphBannerGap + actuals.BannerHeight + actuals.LifeGraphHeight)
            -- due to reasons, the sizing for this is in metrics [ComboGraph]
            -- self:zoomto(actuals.GraphWidth, actuals.ComboGraphHeight)
        end,
        SetCommand = function(self, params)
            -- we have to destroy and reload all children of the ComboGraph when setting it
            -- this crashes really easily if you do it wrong
            if #(self:GetChildren()) > 0 then
                self:Clear()
            end
            self:Load("ComboGraph")
            local ss = SCREENMAN:GetTopScreen():GetStageStats()
            self:Set(ss, ss:GetPlayerStageStats(PLAYER_1))
        end
    },
    LoadFont("Common Normal") .. {
        Name = "ModString",
        InitCommand = function(self)
            -- should be the upper divider + half the space between (accounting for the width of the top divider)
            local yPos = actuals.LeftDivider1UpperGap + (actuals.LeftDivider2UpperGap - actuals.LeftDivider1UpperGap) / 2 + actuals.DividerThickness
            self:xy(actuals.ModTextLeftGap, yPos)
            self:halign(0)
            self:zoom(modTextZoom)
            self:maxwidth(actuals.BannerWidth / modTextZoom - textzoomFudge)
        end,
        SetCommand = function(self, params)
            local mstr = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Current")
            local ss = SCREENMAN:GetTopScreen():GetStageStats()
            if not ss:GetLivePlay() then
                mstr = SCREENMAN:GetTopScreen():GetReplayModifiers()
            end
            self:settext(mstr)
        end
    },
    judgmentBars() .. {
        InitCommand = function(self)
            self:xy(actuals.JudgmentBarLeftGap, actuals.JudgmentBarUpperGap)
        end
    },


}

return t