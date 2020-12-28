local focused = false
local t = Def.ActorFrame {
    Name = "PlaylistsPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.playliststabindex then
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
                self:playcommand("UpdatePlaylistsTab")
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
    ItemAllottedSpace = 405 / 1080, -- top of topmost item to top of bottommost item
    ItemLowerLineUpperGap = 30 / 1080, -- top of top line to top of bottom line
    ItemDividerThickness = 3 / 1080, -- you know what it is (i hope) (ok its based on height so things are consistent-ish)
    ItemDividerLength = 26 / 1080,

    ItemIndexLeftGap = 11 / 1920, -- left edge of frame to left edge of number
    ItemIndexWidth = 38 / 1920, -- left edge of number to uhh nothing
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
    ItemLowerLineUpperGap = ratios.ItemLowerLineUpperGap * SCREEN_HEIGHT,
    ItemDividerThickness = ratios.ItemDividerThickness * SCREEN_HEIGHT,
    ItemDividerLength = ratios.ItemDividerLength * SCREEN_HEIGHT,
    ItemIndexLeftGap = ratios.ItemIndexLeftGap * SCREEN_WIDTH,
    ItemIndexWidth = ratios.ItemIndexWidth * SCREEN_WIDTH,
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

local itemLine1TextSize = 0.85
local itemLine2TextSize = 0.75
local pageTextSize = 0.9

-- our fontpage SUCKS so this should make things look better
-- undo this if the fontpage doesnt SUCK
local dividerUpwardBump = 1

local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local buttonActiveStrokeColor = color("0.85,0.85,0.85,0.8")
local textzoomFudge = 5

local itemListAnimationSeconds = 0.05

-- the entire playlist display ActorFrame
local function playlistList()
    -- modifiable parameters
    local itemCount = 7

    -- internal var storage
    local page = 1
    local maxPage = 1
    local playlistListFrame = nil
    local playlistTable = {}

    local function updatePlaylists()
        playlistTable = SONGMAN:GetPlaylists()
        maxPage = math.ceil(#playlistTable / itemCount)
    end

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

        if playlistListFrame then
            playlistListFrame:playcommand("UpdateItemList")
        end
    end

    local function playlistItem(i)
        local index = i
        local playlist = nil

        -- theres a lot going on here i just wanted to write down vars representing math so its a little clearer for everyone
        -- i should have done this kind of thing in more places but ...
        local itemWidth = actuals.Width * 0.84
        local indX = actuals.ItemIndexLeftGap
        local indW = actuals.ItemIndexWidth
        local remainingWidth = itemWidth - indW - indX
        local nameX = indX + indW -- halign 0
        local deleteX = itemWidth - indX -- halign 1
        local playX = deleteX - actuals.IconWidth - actuals.IconWidth/2 -- halign 1
        local nameW = remainingWidth - (actuals.IconWidth * 2.5) - indX -- area between index and leftmost icon

        return Def.ActorFrame {
            Name = "PlaylistItemFrame_"..i,
            InitCommand = function(self)
                self:y((actuals.ItemAllottedSpace / (itemCount - 1)) * (i-1) + actuals.ItemListUpperGap + actuals.UpperLipHeight)
            end,
            UpdateItemListCommand = function(self)
                index = (page - 1) * itemCount + i
                playlist = playlistTable[index]
                self:finishtweening()
                self:diffusealpha(0)
                if playlist ~= nil then
                    self:playcommand("UpdateText")
                    self:smooth(itemListAnimationSeconds * i)
                    self:diffusealpha(1)
                end
            end,
        
            LoadFont("Common Normal") .. {
                Name = "Index",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(indX)
                    self:zoom(itemLine1TextSize)
                    self:maxwidth(indW / itemLine1TextSize - textzoomFudge)
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settextf("%d.", index)
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(nameX)
                    self:zoom(itemLine1TextSize)
                    self:maxwidth(nameW / itemLine1TextSize - textzoomFudge)
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settext(playlist:GetName())
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if playlist == nil then return end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(1)
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showReplay")) .. {
                Name = "PlayCourse",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(playX)
                    self:zoomto(actuals.IconWidth, actuals.IconHeight)
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then
                        self:diffusealpha(0)
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText("Play As Course")
                            TOOLTIP:Show()
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if playlist == nil then return end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:SetText("Play As Course")
                    TOOLTIP:Show()
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:Hide()
                    self:diffusealpha(1)
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "deleteGoal")) .. {
                Name = "DeletePlaylist",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(deleteX)
                    self:zoomto(actuals.IconWidth, actuals.IconHeight)
                end,
                UpdateTextCommand = function(self)
                    -- dont allow deleting the Favorites playlist
                    -- this breaks so many things
                    if playlist == nil or playlist:GetName() == "Favorites" then
                        self:diffusealpha(0)
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText("Delete Playlist")
                            TOOLTIP:Show()
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if playlist == nil then return end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:SetText("Delete Playlist")
                    TOOLTIP:Show()
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:Hide()
                    self:diffusealpha(1)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Count",
                InitCommand = function(self)
                    self:valign(0):halign(0)
                    self:x(nameX)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(itemLine2TextSize)
                    self:maxwidth((itemWidth - nameX) / 2 / itemLine2TextSize - textzoomFudge)
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settextf("Number of charts: %d", playlist:GetNumCharts())
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Average",
                InitCommand = function(self)
                    self:valign(0):halign(1)
                    self:x(itemWidth - indX)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(itemLine2TextSize)
                    self:maxwidth((itemWidth - nameX) / 2 / itemLine2TextSize - textzoomFudge)
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settextf("(Average MSD %5.2f)", playlist:GetAverageRating())
                end
            }
        }
    end

    local function tabChoices()
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
            {   -- Make a new playlist or add the current chart to the opened playlist
                Name = "newentry",
                Type = "Exclusive",
                Display = {"New Playlist", "Add Current Chart"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function() end,
            },
            {   -- Exit the page that lets you see inside a playlist
                Name = "back",
                Type = "Exclusive",
                Display = {"Back"},
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
                    local bg = self:GetChild("BG")
                    -- update index
                    displayIndex = definition.IndexGetter()

                    -- update visibility by condition
                    if definition.Condition() then
                        if isOver(bg) then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
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
                        self:GetParent():GetParent():playcommand("UpdateItemList")
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
        Name = "PlaylistListFrame",
        BeginCommand = function(self)
            playlistListFrame = self
            updatePlaylists()
            self:playcommand("UpdateItemList")
            self:playcommand("UpdateText")
        end,
        UpdatePlaylistsTabCommand = function(self)
            page = 1
            self:playcommand("UpdateItemList")
            self:playcommand("UpdateText")
        end,
        UpdateItemListCommand = function(self)
            -- in case tooltip gets stuck
            TOOLTIP:Hide()
        end,
        
        tabChoices(),
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
                    self:GetParent():playcommand("UpdateItemList")
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "PageText",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:xy(actuals.Width - actuals.PageTextRightGap, actuals.PageNumberUpperGap)
                self:zoom(pageTextSize)
                -- oddly precise max width but this should fit with the original size
                self:maxwidth(actuals.Width * 0.14 / pageTextSize - textzoomFudge)
            end,
            UpdateItemListCommand = function(self)
                local lb = (page-1) * (itemCount) + 1
                if lb > #playlistTable then
                    lb = #playlistTable
                end
                local ub = page * itemCount
                if ub > #playlistTable then
                    ub = #playlistTable
                end
                self:settextf("%d-%d/%d", lb, ub, #playlistTable)
            end
        }
    }

    for i = 1, itemCount do
        t[#t+1] = playlistItem(i)
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

t[#t+1] = playlistList()

return t