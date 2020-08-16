local t = Def.ActorFrame {
    Name = "MainDisplayFile",
    OnCommand = function(self)
        --- propagate set command through children with the song
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), steps = GAMESTATE:GetCurrentSteps()})
    end
}

local ratios = {
    LeftGap = 78 / 1920,
    UpperGap = 135 / 1080, -- from top edge of screen to edge of bg
    Width = 1765 / 1920,
    Height = 863 / 1080,
    LipHeight = 50 / 1080,

    GraphLeftGap = 18 / 1920,
    GraphWidth = 740 / 1920,
    GraphBannerGap = 16 / 1080, -- from bottom of banner to top of graph
    BannerLeftGap = 22 / 1920,
    BannerHeight = 228 / 1080,
    BannerWidth = 731 / 1920,
    LifeGraphHeight = 71 / 1080,
    ComboGraphHeight = 16 / 1080,

    DividerThickness = 2 / 1080,
    LeftDividerLeftGap = 18 / 1920,
    LeftDividerLength = 740 / 1920,

    LeftDivider1UpperGap = 338 / 1080,
    LeftDivider2UpperGap = 399 / 1080,

    ModTextLeftGap = 19 / 1920,
    -- modtext Y pos is half between the 2 dividers.

    JudgmentBarLeftGap = 20 / 1920, -- edge of frame to left of bar
    JudgmentBarHeight = 44 / 1080,
    JudgmentBarAllottedSpace = 264 / 1080, -- top of top bar to top of bottom bar (valign 0)
    JudgmentBarLength = 739 / 1920,
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    UpperGap = ratios.UpperGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    LipHeight = ratios.LipHeight * SCREEN_HEIGHT,
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
    JudgmentBarHeight = ratios.JudgmentBarHeight * SCREEN_HEIGHT,
    JudgmentBarAllottedSpace = ratios.JudgmentBarAllottedSpace * SCREEN_HEIGHT,
    JudgmentBarLength = ratios.JudgmentBarLength * SCREEN_WIDTH,
}

local modTextZoom = 1
local textzoomFudge = 5


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
            self:zoomto(actuals.Width, actuals.LipHeight)
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
            -- self:zoomto(actuals.GraphWidth, actuals.LifeGraphHeight)
            self:Load("GraphDisplay")

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

    }

}

return t