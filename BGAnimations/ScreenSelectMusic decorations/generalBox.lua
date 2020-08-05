local t = Def.ActorFrame {Name = "GeneralBoxFile"}

local ratios = {
    LeftGap = 1056 / 1920, -- left side of screen to left edge of frame
    TopGap = 497 / 1080, -- top of screen to top of frame
    Width = 776 / 1920,
    Height = 571 / 1080,
    UpperLipHeight = 43 / 1080,
    PageTextTopGap = 24 / 1080, -- from top edge to center of text
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    TopGap = ratios.TopGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    PageTextTopGap = ratios.PageTextTopGap * SCREEN_HEIGHT,
}

-- preferably, these match an actorframe name below for organization purposes
local choiceNames = {
    "General",
    "Scores",
    "Tags",
    "Goals",
    "Preview",
    "",
    "",
}

local choiceTextSize = 0.8
local textzoomFudge = 5

local function createChoices()
    local function createChoice(i)
        return LoadFont("Common Normal") .. {
            Name = choiceNames[i],
            InitCommand = function(self)
                -- this position is the center of the text
                -- divides the space into slots for the choices then places them half way into them
                -- should work for any count of choices
                -- and the maxwidth will make sure they stay nonoverlapping
                self:x((actuals.Width / #choiceNames) * (i-1) + (actuals.Width / #choiceNames / 2))
                self:zoom(choiceTextSize)
                self:maxwidth(actuals.Width / #choiceNames / choiceTextSize - textzoomFudge)
                self:settext(choiceNames[i])
            end
        }
    end
    local t = Def.ActorFrame {
        Name = "Choices",
        InitCommand = function(self)
            self:y(actuals.PageTextTopGap)
        end
    }
    for i = 1, #choiceNames do
        t[#t+1] = createChoice(i)
    end
    return t
end

t[#t+1] = Def.ActorFrame {
    Name = "Container",
    InitCommand = function(self)
        self:xy(actuals.LeftGap, actuals.TopGap)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.Height)
            self:diffuse(color("0,0,0,0.6"))
        end
    },
    Def.Quad {
        Name = "UpperLip",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.UpperLipHeight)
            self:diffuse(color("0,0,0,0.6"))
        end
    },
    createChoices(),
    LoadActorWithParams("generalPages/general.lua", {ratios = ratios, actuals = actuals})
}

return t
