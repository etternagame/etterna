local focused = false
local t = Def.ActorFrame {
    Name = "TagsPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.tagstabindex then
                self:z(2)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
            else
                self:z(-1)
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
    end,
}

local ratios = {
    EdgeBuffer = 11 / 1920, -- intended buffer from leftmost edge, rightmost edge, and distance away from center of frame
    UpperLipHeight = 43 / 1080,
    LipSeparatorThickness = 2 / 1080,
    
    PageNumberUpperGap = 473 / 1080, -- bottom of upper lip to top of text (this text is align right EdgeBuffer from right edge)

    ItemListUpperGap = 35 / 1080, -- bottom of upper lip to top of topmost item
    ItemAllottedSpace = 435 / 1080, -- top of topmost item to top of bottommost item
    ItemSpacing = 45 / 1080, -- top of item to top of next item
}

local actuals = {
    EdgeBuffer = ratios.EdgeBuffer * SCREEN_WIDTH,
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    LipSeparatorThickness = ratios.LipSeparatorThickness * SCREEN_HEIGHT,
    PageNumberUpperGap = ratios.PageNumberUpperGap * SCREEN_HEIGHT,
    ItemListUpperGap = ratios.ItemListUpperGap * SCREEN_HEIGHT,
    ItemAllottedSpace = ratios.ItemAllottedSpace * SCREEN_HEIGHT,
    ItemSpacing = ratios.ItemSpacing * SCREEN_HEIGHT,
}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

local tagTextSize = 1.2

local pageTextSize = 0.9
local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local textzoomFudge = 5

local function tagList()
    -- modifiable parameters
    local tagsPerColumn = 10
    local columns = 2

    -- internal var storage
    local storedTags = {} -- exact tag list, keys are tags, values are {chartkeys : 1} or {chartkey : nil}
    local tagNameList = {} -- just a list of all tags so it can be indexed in a consistent order
    local page = 1
    local maxPage = 1

    local function movePage(n)
        if maxPage <= 1 then
            return
        end

        -- math to make pages loop both directions
        local nn = (page + n) % (maxPage + 1)
        if nn == 0 then
            nn = n > 0 and 1 or maxPage
        end
        page = nn
    end

    local function tagListItem(i)
        local column = math.floor((i-1) / tagsPerColumn)
        local allowedWidth = (actuals.Width / columns - actuals.EdgeBuffer * 2)
        local index = i
        local tag = ""

        return UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "TagButton_"..i,
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                txt:halign(0):valign(0)
                bg:halign(0)

                -- this should make it so that the left column (0) is at EdgeBuffer and the right column (1) is in the middle-ish
                -- if the column count is changed, it should adjust accordingly
                self:x(actuals.EdgeBuffer + column * actuals.Width / columns)
                self:y(actuals.UpperLipHeight + actuals.ItemListUpperGap + actuals.ItemAllottedSpace / tagsPerColumn * (i-1 - column * tagsPerColumn))
                txt:zoom(tagTextSize)
                txt:maxwidth(allowedWidth / tagTextSize - textzoomFudge)
                txt:settext(" ")
                bg:zoomto(allowedWidth, actuals.UpperLipHeight)
                bg:y(txt:GetZoomedHeight() / 2)
            end,
            UpdateTagListCommand = function(self)
                local txt = self:GetChild("Text")
                index = (page-1) * columns * tagsPerColumn + i
                tag = tagNameList[index]
                if tag ~= nil and tag ~= "" then
                    self:diffusealpha(1)
                    txt:settext(tag)
                else
                    self:diffusealpha(0)
                end
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    --
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

    local function tagChoices()
        
        -- keeping track of which choices are on at any moment (a list of indices)
        local activeChoices = {}

        -- identify each choice using this table
        --  Name: The name of the choice (NOT SHOWN TO THE USER)
        --  Type: Toggle/Exclusive/Tap
        --      Toggle - This choice can be clicked multiple times to scroll through choices
        --      Exclusive - This choice is one out of a set of Exclusive choices. Only one Exclusive choice can be picked at once
        --      Tap - This choice can only be pressed (if visible by Condition) and will run TapFunction at that time
        --  Display: The string the user sees. One option for each choice must be given if it is a Toggle choice
        --  Condition: A function that returns true or false. Determines if the choice should be visible or not
        --  TapFunction: A function that runs when the button is pressed. This is only used by the Tap Type
        local choiceDefinitions = {
            {   -- Button to assign tags to charts
                Name = "assign",
                Type = "Exclusive",
                Display = {"Assign"},
                Condition = function() return true end,
            },
            {   -- Button to filter charts by tags (Require charts have these tags)
                Name = "filter",
                Type = "Exclusive",
                Display = {"Require Tag"},
                Condition = function() return true end,
            },
            {   -- Button to filter charts by tags (Hide charts with these tags)
                Name = "hide",
                Type = "Exclusive",
                Display = {"Hide Tag"},
                Condition = function() return true end,
            },
            {   -- Button to change filter mode AND/OR
                Name = "filtermode",
                Type = "Toggle",
                Display = {"Mode: AND", "Mode: OR"},
                Condition = function() return activeChoices[2] or activeChoices[3] end,
            },
            {   -- Button to delete tags
                Name = "delete",
                Type = "Exclusive",
                Display = {"Delete"},
                Condition = function() return #tagNameList > 0 end,
            },
            {   -- Button to create tags
                Name = "new",
                Type = "Tap",
                Display = {"New"},
                Condition = function() return true end,
                TapFunction = function() ms.ok("new tag") end,
            },
        }

        local function createChoice(i)
            local definition = choiceDefinitions[i]
            local displayIndex = 1

            return UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "ChoiceButton_" ..i,
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")

                    -- this position is the center of the text
                    -- divides the space into slots for the choices then places them half way into them
                    -- should work for any count of choices
                    -- and the maxwidth will make sure they stay nonoverlapping
                    self:x((actuals.Width / #choiceDefinitions) * (i-1) + (actuals.Width / #choiceDefinitions / 2))
                    txt:zoom(choiceTextSize)
                    txt:maxwidth(actuals.Width / #choiceDefinitions / choiceTextSize - textzoomFudge)
                    bg:zoomto(actuals.Width / #choiceDefinitions, actuals.UpperLipHeight)
                    self:playcommand("UpdateText")
                end,
                UpdateTextCommand = function(self)
                    local txt = self:GetChild("Text")
                    txt:settext(definition.Display[displayIndex])
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        self:playcommand("UpdateText")
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
                self:y(actuals.UpperLipHeight / 2)
            end
        }

        for i = 1, #choiceDefinitions do
            t[#t+1] = createChoice(i)
        end

        return t
    end

    local t = Def.ActorFrame {
        Name = "TagListFrame",
        InitCommand = function(self)
            --
        end,
        BeginCommand = function(self)
            self:playcommand("UpdateTagList")
        end,
        UpdateTagListCommand = function(self)
            storedTags = TAGMAN:get_data().playerTags
            tagNameList = {}
            for k, _ in pairs(storedTags) do
                tagNameList[#tagNameList+1] = k
            end
        end,
        
        tagChoices(),
        Def.Quad {
            Name = "MouseWheelRegion",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:diffusealpha(0)
                self:zoomto(actuals.Width, actuals.Height)
            end,
            MouseScrollMessageCommand = function(self, params)
                if isOver(self) and focused then
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "PageText",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:xy(actuals.Width - actuals.EdgeBuffer, actuals.PageNumberUpperGap)
                self:zoom(pageTextSize)
                self:maxwidth(actuals.Width / pageTextSize - textzoomFudge)
            end,
            UpdateTagListCommand = function(self)
                local lb = (page-1) * (columns * tagsPerColumn) + 1
                if lb > #tagNameList then
                    lb = #tagNameList
                end
                local ub = page * columns * tagsPerColumn
                if ub > #tagNameList then
                    ub = #tagNameList
                end
                self:settextf("%d-%d/%d", lb, ub, #tagNameList)
            end
        }
    }

    for i = 1, tagsPerColumn * columns do
        t[#t+1] = tagListItem(i)
    end

    return t
end

t[#t+1] = Def.Quad {
    Name = "UpperLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.UpperLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = Def.Quad {
    Name = "LipTop",
    InitCommand = function(self)
        self:halign(0)
        self:zoomto(actuals.Width, actuals.LipSeparatorThickness)
        self:diffuse(color(".4,.4,.4,.7"))
    end
}

t[#t+1] = tagList()

return t