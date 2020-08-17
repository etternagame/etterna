local sizing = Var("sizing")
if sizing == nil then sizing = {} end
--[[
    We are expecting the sizing table to be provided on file load.
    It should contain these attributes:
    Width
    Height
]]
-- all elements are placed relative to default valign - 0 halign
-- this means relatively to center vertically and relative to the left end horizontally

local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())
local timingScale = ms.JudgeScalers[judgeSetting]

-- cap the graph to this
local maxOffset = 180
local lineThickness = 2
local lineAlpha = 0.2
local textPadding = 5
local textSize = 0.65

-- judgment windows to display on the plot
local barJudgments = {
    "TapNoteScore_W2",
    "TapNoteScore_W3",
    "TapNoteScore_W4",
    "TapNoteScore_W5",
}

-- convert number to another number out of a given width
-- relative to left side of the graph
local function fitX(x, maxX)
    return x / maxX * sizing.Width
end

-- convert millisecond values to a y position in the graph
-- relative to vertical center
local function fitY(y, maxY)
    return -1 * y / maxY * sizing.Height / 2 + sizing.Height / 2
end


local t = Def.ActorFrame {
    Name = "OffsetPlotFile",
    UpdateSizingCommand = function(self, params)
        sizing = params.sizing
    end
}

t[#t+1] = Def.Quad {
    Name = "BG",
    InitCommand = function(self)
        self:halign(0)
        self:diffuse(color("0,0,0,0.8"))
        self:playcommand("UpdateSizing")
    end,
    UpdateSizingCommand = function(self)
        self:y(sizing.Height / 2)
        self:zoomto(sizing.Width, sizing.Height)
    end
}

t[#t+1] = Def.Quad {
    Name = "CenterLine",
    InitCommand = function(self)
        self:halign(0)
        self:diffuse(byJudgment("TapNoteScore_W1"))
        self:diffusealpha(lineAlpha)
        self:playcommand("UpdateSizing")
    end,
    UpdateSizingCommand = function(self)
        self:y(sizing.Height / 2)
        self:zoomto(sizing.Width, lineThickness)
    end
}

for i, j in ipairs(barJudgments) do
    t[#t+1] = Def.Quad {
        Name = j.."_Late",
        InitCommand = function(self)
            self:halign(0)
            self:diffuse(byJudgment(j))
            self:diffusealpha(lineAlpha)
            self:playcommand("UpdateSizing")
        end,
        UpdateSizingCommand = function(self)
            local window = ms.getLowerWindowForJudgment(j, timingScale)
            self:y(fitY(window, maxOffset))
            self:zoomto(sizing.Width, lineThickness)
        end
    }
    t[#t+1] = Def.Quad {
        Name = j.."_Early",
        InitCommand = function(self)
            self:halign(0)
            self:diffuse(byJudgment(j))
            self:diffusealpha(lineAlpha)
            self:playcommand("UpdateSizing")
        end,
        UpdateSizingCommand = function(self)
            local window = ms.getLowerWindowForJudgment(j, timingScale)
            self:y(fitY(-window, maxOffset))
            self:zoomto(sizing.Width, lineThickness)
        end
    }
end

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "LateText",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoom(textSize)
        self:playcommand("UpdateSizing")
    end,
    UpdateSizingCommand = function(self)
        local bound = ms.getUpperWindowForJudgment(barJudgments[#barJudgments], timingScale)
        self:xy(textPadding, textPadding)
        self:settextf("Late (+%dms)", bound)
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "EarlyText",
    InitCommand = function(self)
        self:halign(0):valign(1)
        self:zoom(textSize)
        self:playcommand("UpdateSizing")
    end,
    UpdateSizingCommand = function(self)
        local bound = ms.getUpperWindowForJudgment(barJudgments[#barJudgments], timingScale)
        self:xy(textPadding, sizing.Height - textPadding)
        self:settextf("Early (-%dms)", bound)
    end
}



return t