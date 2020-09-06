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

-- the page names in the order they go
local choiceNames = {
    "General",
    "Scores",
    "Tags",
    "Goals",
    "Playlists",
    "Profile",
}

local choiceTextSize = 0.8
local buttonHoverAlpha = 0.6
local textzoomFudge = 5

local function createChoices()
    local function createChoice(i)
        return UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "ButtonTab_"..choiceNames[i],
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")

                -- this position is the center of the text
                -- divides the space into slots for the choices then places them half way into them
                -- should work for any count of choices
                -- and the maxwidth will make sure they stay nonoverlapping
                self:x((actuals.Width / #choiceNames) * (i-1) + (actuals.Width / #choiceNames / 2))
                txt:zoom(choiceTextSize)
                txt:maxwidth(actuals.Width / #choiceNames / choiceTextSize - textzoomFudge)
                txt:settext(choiceNames[i])
                bg:zoomto(actuals.Width / #choiceNames, actuals.UpperLipHeight)
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    MESSAGEMAN:Broadcast("GeneralTabSet", {tab = i})
                end
            end,
            RolloverUpdateCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "in" then
                    self:diffusealpha(buttonHoverAlpha)
                else
                    self:diffusealpha(1)
                end
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
            self:diffuse(color("#111111"))
            self:diffusealpha(0.6)
        end
    },
    Def.Quad {
        Name = "UpperLip",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.UpperLipHeight)
            self:diffuse(color("#111111"))
            self:diffusealpha(0.6)
        end
    },
    createChoices(),
    LoadActorWithParams("generalPages/general.lua", {ratios = ratios, actuals = actuals}) .. {
        BeginCommand = function(self)
            -- this will cause the tab to become visible
            self:playcommand("GeneralTabSet", {tab = 1})
            -- skip animation
            self:finishtweening()
        end
    },
    LoadActorWithParams("generalPages/scores.lua", {ratios = ratios, actuals = actuals}),
}

return t
