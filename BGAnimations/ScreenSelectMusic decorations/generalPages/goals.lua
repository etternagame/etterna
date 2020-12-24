local focused = false
local t = Def.ActorFrame {
    Name = "GoalsPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.goalstabindex then
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
                self:playcommand("UpdateGoalsTab")
            else
                self:z(-100)
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
    end,
    WheelSettledMessageCommand = function(self, params)
        if not focused then return end
    end,
    ChangedStepsMessageCommand = function(self, params)
        if not focused then return end
    end
}

local ratios = {
    UpperLipHeight = 43 / 1080,
    LipSeparatorThickness = 2 / 1080,
    
    PageTextRightGap = 33 / 1920, -- right of frame, right of text
    PageNumberUpperGap = 525 / 1080, -- bottom of upper lip to top of text

    ItemListUpperGap = 35 / 1080, -- bottom of upper lip to top of topmost item
    ItemAllottedSpace = 397 / 1080, -- top of topmost item to top of bottommost item
    ItemSpacing = 100 / 1080, -- top of item to top of next item
    ItemLowerLineUpperGap = 40 / 1080, -- top of top line to top of bottom line
    ItemDividerThickness = 3 / 1080, -- you know what it is (i hope) (ok its based on height so things are consistent-ish)
    ItemDividerLength = 26 / 1080,

    ItemPriorityLeftGap = 11 / 1920, -- left edge of frame to left edge of number
    ItemPriorityWidth = 38 / 1920, -- left edge of number to uhh nothing
    IconWidth = 18 / 1920, -- for the trash thing
    IconHeight = 21 / 1080,
}

local actuals = {
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    LipSeparatorThickness = ratios.LipSeparatorThickness * SCREEN_HEIGHT,
    PageTextRightGap = ratios.PageTextRightGap * SCREEN_WIDTH,
    PageNumberUpperGap = ratios.PageNumberUpperGap * SCREEN_HEIGHT,
    ItemListUpperGap = ratios.ItemListUpperGap * SCREEN_HEIGHT,
    ItemAllottedSpace = ratios.ItemAllottedSpace * SCREEN_HEIGHT,
    ItemSpacing = ratios.ItemSpacing * SCREEN_HEIGHT,
    ItemLowerLineUpperGap = ratios.ItemLowerLineUpperGap * SCREEN_HEIGHT,
    ItemDividerThickness = ratios.ItemDividerThickness * SCREEN_HEIGHT,
    ItemDividerLength = ratios.ItemDividerLength * SCREEN_HEIGHT,
    ItemPriorityLeftGap = ratios.ItemPriorityLeftGap * SCREEN_WIDTH,
    ItemPriorityWidth = ratios.ItemPriorityWidth * SCREEN_WIDTH,
    IconWidth = ratios.IconWidth * SCREEN_WIDTH,
    IconHeight = ratios.IconHeight * SCREEN_HEIGHT,
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

local goalLine1TextSize = 1
local goalLine2TextSize = 1
local pageTextSize = 0.9

-- our fontpage SUCKS so this should make things look better
-- undo this if the fontpage doesnt SUCK
local dividerUpwardBump = 1

local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local buttonActiveStrokeColor = color("0.85,0.85,0.85,0.8")
local textzoomFudge = 5

local goalListAnimationSeconds = 0.05

local function goalList()
    -- modifiable parameters
    local goalItemCount = 5

    -- internal var storage
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

    local function goalListItem(i)
        local index = i

        -- theres a lot going on here i just wanted to write down vars representing math so its a little clearer for everyone
        -- i should have done this kind of thing in more places but ...
        local itemWidth = actuals.Width
        local prioX = actuals.ItemPriorityLeftGap
        local prioW = actuals.ItemPriorityWidth
        local remainingWidth = itemWidth - prioW - prioX
        local diffW = remainingWidth / 60 * 13 -- keep this in line with the other divisions below (combined at around 1/1) -- 13/60
        local diffX = prioX + prioW + diffW/2
        local div1X = prioX + prioW + diffW
        local rateW = remainingWidth / 60 * 13 -- above comment -- 13/60
        local rateX = div1X + rateW/2
        local div2X = div1X + rateW
        local percentW = remainingWidth / 60 * 21 -- above comment -- 21/60
        local percentX = div2X + percentW/2
        local div3X = div2X + percentW
        local msdW = remainingWidth / 60 * 13 -- above comment -- 13/60
        local msdX = div3X + msdW/2
        local trashX = div3X + msdW
        local line2AllowedWidth = div3X - prioX -- note: bottom line intersects trash and page number; this area is full width up to the 3rd divider

        return Def.ActorFrame {
            Name = "GoalItemFrame_"..i,
            InitCommand = function(self)
                self:y((actuals.ItemAllottedSpace / (goalItemCount - 1)) * (i-1) + actuals.ItemListUpperGap + actuals.UpperLipHeight)
            end,
        
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Priority",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0):valign(0)
                    bg:halign(0)
                    bg:diffusealpha(0.2)

                    self:x(prioX)
                    txt:zoom(goalLine1TextSize)
                    txt:maxwidth(prioW / goalLine1TextSize - textzoomFudge)
                    txt:settext("1.")
                    bg:zoomto(prioW, txt:GetZoomedHeight())
                    bg:y(txt:GetZoomedHeight() / 2)
                end,
                UpdateGoalListCommand = function(self)
                    local txt = self:GetChild("Text")
                    index = (page-1) * goalItemCount + i
                    self:finishtweening()
                    --self:diffusealpha(0)
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
            },
            LoadFont("Common Normal") .. {
                Name = "Difficulty",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(diffX)
                    self:zoom(goalLine1TextSize)
                    self:maxwidth(diffW / goalLine1TextSize - textzoomFudge)
                    self:settext("IN")
                end,
            },
            Def.Quad {
                Name = "Div1",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(div1X)
                    self:y(-dividerUpwardBump)
                    self:zoomto(actuals.ItemDividerThickness, actuals.ItemDividerLength)
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Rate",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:valign(0)
                    bg:diffusealpha(0.2)

                    self:x(rateX)
                    txt:zoom(goalLine1TextSize)
                    txt:maxwidth(rateW / goalLine1TextSize - textzoomFudge)
                    txt:settext("1.0x")
                    bg:zoomto(rateW, txt:GetZoomedHeight())
                    bg:y(txt:GetZoomedHeight() / 2)
                end,
                UpdateGoalListCommand = function(self)
                    local txt = self:GetChild("Text")
                    index = (page-1) * goalItemCount + i
                    self:finishtweening()
                    --self:diffusealpha(0)
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
            },
            Def.Quad {
                Name = "Div2",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(div2X)
                    self:y(-dividerUpwardBump)
                    self:zoomto(actuals.ItemDividerThickness, actuals.ItemDividerLength)
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Percentage",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:valign(0)
                    bg:diffusealpha(0.2)

                    self:x(percentX)
                    txt:zoom(goalLine1TextSize)
                    txt:maxwidth(percentW / goalLine1TextSize - textzoomFudge)
                    txt:settext("93% (93.33%)")
                    bg:zoomto(percentW, txt:GetZoomedHeight())
                    bg:y(txt:GetZoomedHeight() / 2)
                end,
                UpdateGoalListCommand = function(self)
                    local txt = self:GetChild("Text")
                    index = (page-1) * goalItemCount + i
                    self:finishtweening()
                    --self:diffusealpha(0)
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
            },
            Def.Quad {
                Name = "Div3",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(div3X)
                    self:y(-dividerUpwardBump)
                    self:zoomto(actuals.ItemDividerThickness, actuals.ItemDividerLength)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "MSD",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(msdX)
                    self:zoom(goalLine1TextSize)
                    self:maxwidth(msdW / goalLine1TextSize - textzoomFudge)
                    self:settext("11.11")
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "deleteGoal")) .. {
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    -- trash will be aligned under the last divider
                    self:x(div3X)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoomto(actuals.IconWidth, actuals.IconHeight)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "NameDate",
                InitCommand = function(self)
                    self:valign(0):halign(0)
                    self:x(prioX)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(goalLine2TextSize)
                    self:maxwidth(line2AllowedWidth / goalLine2TextSize - textzoomFudge)
                    self:settext("Song Name Song Name (Achieved 11/11/11)")
                end
            }
        }
    end

    local function goalChoices()
        -- keeping track of which choices are on at any moment (keys are indices, values are true/false/nil)
        local activeChoices = {}

        -- identify each choice using this table
        --  Name: The name of the choice (NOT SHOWN TO THE USER)
        --  Type: Toggle/Exclusive/Tap
        --      Toggle - This choice can be clicked multiple times to scroll through choices
        --      Exclusive - This choice is one out of a set of Exclusive choices. Only one Exclusive choice can be picked at once
        --      Tap - This choice can only be pressed (if visible by Condition) and will only run TapFunction at that time
        --  Display: The string the user sees. One option for each choice must be given if it is a Toggle choice
        --  Condition: A function that returns true or false. Determines if the choice should be visible or not
        --  IndexGetter: A function that returns an index for its status, according to the Displays set
        --  TapFunction: A function that runs when the button is pressed
        local choiceDefinitions = {
            {   -- Sort by Priority
                Name = "assign",
                Type = "Exclusive",
                Display = {"Priority"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- Sort by Rate
                Name = "assign",
                Type = "Exclusive",
                Display = {"Rate"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- Sort by Difficulty
                Name = "assign",
                Type = "Exclusive",
                Display = {"Difficulty"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- Sort by Song (Song + ChartKey)
                Name = "assign",
                Type = "Exclusive",
                Display = {"Name"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- Sort by Set Date
                Name = "assign",
                Type = "Exclusive",
                Display = {"Date"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- New Goal on current Chart
                Name = "assign",
                Type = "Exclusive",
                Display = {"New Goal"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- Toggle between Show All Goals and Show Goals On This Song
                Name = "assign",
                Type = "Exclusive",
                Display = {"Showing All", "Song Only"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
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
                    bg:diffusealpha(0.1)

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
                    -- update index
                    displayIndex = definition.IndexGetter()

                    -- update visibility by condition
                    if definition.Condition() then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end

                    if activeChoices[i] then
                        txt:strokecolor(buttonActiveStrokeColor)
                    else
                        txt:strokecolor(color("0,0,0,0"))
                    end

                    -- update display
                    txt:settext(definition.Display[displayIndex])
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        -- exclusive choices cause activechoices to be forced to this one
                        if definition.Type == "Exclusive" then
                            activeChoices = {[i]=true}
                        else
                            -- uhh i didnt implement any other type that would ... be used for.. this
                        end

                        -- run the tap function
                        if definition.TapFunction ~= nil then
                            definition.TapFunction()
                        end
                        self:GetParent():GetParent():playcommand("UpdateGoalList")
                        self:GetParent():playcommand("UpdateText")
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
            end,
        }

        for i = 1, #choiceDefinitions do
            t[#t+1] = createChoice(i)
        end

        return t
    end

    local t = Def.ActorFrame {
        Name = "GoalListFrame",
        BeginCommand = function(self)
            self:playcommand("UpdateGoalList")
            self:playcommand("UpdateText")
        end,
        UpdateGoalsTabCommand = function(self)
            page = 1
            self:playcommand("UpdateGoalList")
            self:playcommand("UpdateText")
        end,
        UpdateGoalListCommand = function(self)
            -- this sets all the data things over and over and over
            goalList = {}
        end,
        
        goalChoices(),
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
                    self:GetParent():playcommand("UpdateGoalList")
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "PageText",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:xy(actuals.Width - actuals.PageTextRightGap, actuals.PageNumberUpperGap)
                self:zoom(pageTextSize)
                self:maxwidth(actuals.Width / pageTextSize - textzoomFudge)
            end,
            UpdateGoalListCommand = function(self)
                local lb = (page-1) * (goalItemCount) + 1
                if lb > #goalList then
                    lb = #goalList
                end
                local ub = page * goalItemCount
                if ub > #goalList then
                    ub = #goalList
                end
                self:settextf("%d-%d/%d", lb, ub, #goalList)
            end
        }
    }

    for i = 1, goalItemCount do
        t[#t+1] = goalListItem(i)
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

t[#t+1] = goalList()

return t