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
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
                self:playcommand("UpdateTagsTab")
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
        self:playcommand("UpdateTagsTab")
    end,
    ChangedStepsMessageCommand = function(self, params)
        if not focused then return end
        self:playcommand("UpdateTagsTab")
    end
}

local ratios = {
    EdgeBuffer = 11 / 1920, -- intended buffer from leftmost edge, rightmost edge, and distance away from center of frame
    UpperLipHeight = 43 / 1080,
    LipSeparatorThickness = 2 / 1080,
    
    PageTextRightGap = 33 / 1920, -- right of frame, right of text
    PageNumberUpperGap = 48 / 1080, -- bottom of upper lip to top of text

    ItemListUpperGap = 35 / 1080, -- bottom of upper lip to top of topmost item
    ItemAllottedSpace = 435 / 1080, -- top of topmost item to top of bottommost item
    ItemSpacing = 45 / 1080, -- top of item to top of next item
}

local actuals = {
    EdgeBuffer = ratios.EdgeBuffer * SCREEN_WIDTH,
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    LipSeparatorThickness = ratios.LipSeparatorThickness * SCREEN_HEIGHT,
    PageTextRightGap = ratios.PageTextRightGap * SCREEN_WIDTH,
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

local translations = {
    AssignTag = THEME:GetString("ScreenSelectMusic Tags", "AssignTag"),
    RequireTag = THEME:GetString("ScreenSelectMusic Tags", "RequireTag"),
    HideTag = THEME:GetString("ScreenSelectMusic Tags", "HideTag"),
    ModeALL = THEME:GetString("ScreenSelectMusic Tags", "ModeALL"),
    ModeANY = THEME:GetString("ScreenSelectMusic Tags", "ModeANY"),
    DeleteTag = THEME:GetString("ScreenSelectMusic Tags", "DeleteTag"),
    TagDeleteInProgress = THEME:GetString("ScreenSelectMusic Tags", "TagDeleteInProgress"),
    NewTag = THEME:GetString("ScreenSelectMusic Tags", "NewTag"),
    NewTagQuestion = THEME:GetString("ScreenSelectMusic Tags", "NewTagQuestion"),
    ApplyFilter = THEME:GetString("ScreenSelectMusic Tags", "ApplyFilter"),
    ResetFilter = THEME:GetString("ScreenSelectMusic Tags", "ResetFilter"),
}

local tagTextSize = 1.2
local pageTextSize = 0.7

local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local textzoomFudge = 5

local tagListAnimationSeconds = 0.03

local function tagList()
    -- modifiable parameters
    local tagsPerColumn = 10
    local columns = 2

    -- internal var storage
    local storedTags = {} -- exact tag list, keys are tags, values are {chartkeys : 1} or {chartkey : nil}
    local tagNameList = {} -- just a list of all tags so it can be indexed in a consistent order
    local excludedTags = {} -- a list but instead of being indexed, the keys are the tags
    local requiredTags = {} -- same as above
    local page = 1
    local maxPage = 1

    -- determines the mode of the tag list thing
    -- accepts "Assign", "Require", "Exclude", "Delete"
    local tagListMode = "Assign"

    -- just a list of the modes that will allow for mouse hover highlighting
    -- this isnt necessary at the moment, but just in case i guess
    local tagListModeForClicking = {
        Assign = true,
        Require = true,
        Exclude = true,
        Delete = true,
    }

    local tagsAssignedToCurrentChart = {}

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
                -- and if the column count is changed, it should adjust accordingly
                self:x(actuals.EdgeBuffer + column * actuals.Width / columns)
                self:y(actuals.UpperLipHeight + actuals.ItemListUpperGap + actuals.ItemAllottedSpace / tagsPerColumn * (i-1 - column * tagsPerColumn))
                txt:zoom(tagTextSize)
                txt:maxwidth(allowedWidth / tagTextSize - textzoomFudge)
                txt:settext(" ")
                bg:zoomto(allowedWidth, actuals.UpperLipHeight)
                bg:y(txt:GetZoomedHeight() / 2)
            end,
            ColorConfigUpdatedMessageCommand = function(self)
                self:playcommand("UpdateTagList")
            end,
            UpdateTagListCommand = function(self)
                local txt = self:GetChild("Text")
                index = (page-1) * columns * tagsPerColumn + i
                tag = tagNameList[index]
                self:finishtweening()
                self:diffusealpha(0)
                if tag ~= nil and tag ~= "" then
                    self:smooth(tagListAnimationSeconds * i)
                    self:diffuse(COLORS:getMainColor("PrimaryText"))
                    txt:settext(tag)

                    if tagListMode == "Assign" then
                        -- color if assigned on this chart
                        local chart = GAMESTATE:GetCurrentSteps()
                        if chart ~= nil then
                            local ck = chart:GetChartKey()
                            if storedTags[tag][ck] then
                                self:diffuse(COLORS:getColor("generalBox", "AssignedTag"))
                            end
                        end
                    elseif tagListMode == "Require" then
                        -- color if required
                        if requiredTags[tag] then
                            self:diffuse(COLORS:getColor("generalBox", "RequiredTag"))
                        end
                    elseif tagListMode == "Exclude" then
                        -- color if excluded
                        if excludedTags[tag] then
                            self:diffuse(COLORS:getColor("generalBox", "FilteredTag"))
                        end
                    end
                    self:diffusealpha(1)
                end
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    if tagListModeForClicking[tagListMode] == nil then return end

                    if tagListMode == "Assign" then
                        -- you cant assign a tag to nothing
                        local chart = GAMESTATE:GetCurrentSteps()
                        if chart ~= nil then
                            local ck = chart:GetChartKey()
                            if storedTags[tag][ck] then
                                TAGMAN:get_data().playerTags[tag][ck] = nil
                            else
                                TAGMAN:get_data().playerTags[tag][ck] = 1
                            end
                            TAGMAN:set_dirty()
                            TAGMAN:save()
                            self:GetParent():playcommand("UpdateTagList")
                            MESSAGEMAN:Broadcast("ReassignedTags")
                        end

                    elseif tagListMode == "Require" then
                        if requiredTags[tag] == nil then
                            requiredTags[tag] = true
                        else
                            requiredTags[tag] = nil
                        end
                        local newtable = {}
                        for tagname, _ in pairs(requiredTags) do
                            newtable[#newtable+1] = tagname
                        end
                        WHEELDATA:SetRequiredTags(newtable)
                        self:GetParent():playcommand("UpdateTagList")

                    elseif tagListMode == "Exclude" then
                        if excludedTags[tag] == nil then
                            excludedTags[tag] = true
                        else
                            excludedTags[tag] = nil
                        end
                        local newtable = {}
                        for tagname, _ in pairs(excludedTags) do
                            newtable[#newtable+1] = tagname
                        end
                        WHEELDATA:SetExcludedTags(newtable)
                        self:GetParent():playcommand("UpdateTagList")

                    elseif tagListMode == "Delete" then
                        if excludedTags[tag] then excludedTags[tag] = nil end
                        if requiredTags[tag] then requiredTags[tag] = nil end

                        -- delete tag record
                        TAGMAN:get_data().playerTags[tag] = nil
                        TAGMAN:set_dirty()
                        TAGMAN:save()

                        self:GetParent():playcommand("DeletedTag")
                    end
                end
            end,
            RolloverUpdateCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "in" then
                    if tagListModeForClicking[tagListMode] ~= nil then
                        self:diffusealpha(buttonHoverAlpha)
                    end
                else
                    self:diffusealpha(1)
                end
            end
        }
    end

    local function tagChoices()
        -- keeping track of which choices are on at any moment (a list of indices)
        -- setting 1 to be true initially because tagListMode is set to Assign and there needs to be consistency there
        local activeChoices = {[1]=true}

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
            {   -- Button to assign tags to charts
                Name = "assign",
                Type = "Exclusive",
                Display = {translations["AssignTag"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    tagListMode = "Assign"
                end,
            },
            {   -- Button to filter charts by tags (Require charts have these tags)
                Name = "filter",
                Type = "Exclusive",
                Display = {translations["RequireTag"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    tagListMode = "Require"
                end,
            },
            {   -- Button to filter charts by tags (Hide charts with these tags)
                Name = "hide",
                Type = "Exclusive",
                Display = {translations["HideTag"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    tagListMode = "Exclude"
                end,
            },
            {   -- Button to change filter mode AND/OR
                Name = "filtermode",
                Type = "Toggle",
                Display = {translations["ModeALL"], translations["ModeANY"]},
                IndexGetter = function()
                    if tagListMode == "Require" then
                        return WHEELDATA:GetRequiredTagMode() and 1 or 2
                    elseif tagListMode == "Exclude" then
                        return WHEELDATA:GetExcludedTagMode() and 1 or 2
                    else
                        return 1 -- dont care
                    end
                end,
                Condition = function() return tagListMode == "Exclude" or tagListMode == "Require" end,
                TapFunction = function()
                    if tagListMode == "Require" then
                        WHEELDATA:SetRequiredTagMode()
                    elseif tagListMode == "Exclude" then
                        WHEELDATA:SetExcludedTagMode()
                    else
                        -- nothing
                    end
                end,
            },
            {   -- Button to delete tags
                Name = "delete",
                Type = "Exclusive",
                Display = {translations["DeleteTag"], translations["TagDeleteInProgress"]},
                IndexGetter = function()
                    if tagListMode == "Delete" then
                        return 2
                    else
                        return 1
                    end
                end,
                Condition = function() return #tagNameList > 0 end,
                TapFunction = function()
                    if tagListMode == "Delete" then
                        tagListMode = "Assign"
                    else
                        tagListMode = "Delete"
                    end
                end,
            },
            {   -- Button to create tags
                Name = "new",
                Type = "Tap",
                Display = {translations["NewTag"]},
                Condition = function() return true end,
                IndexGetter = function() return 1 end,
                TapFunction = function()
                    local redir = SCREENMAN:get_input_redirected(PLAYER_1)
                    local function off()
                        if redir then
                            SCREENMAN:set_input_redirected(PLAYER_1, false)
                        end
                    end
                    local function on()
                        if redir then
                            SCREENMAN:set_input_redirected(PLAYER_1, true)
                        end
                    end
                    off()
                    -- input redirects are controlled here because we want to be careful not to break any prior redirects
                    askForInputStringWithFunction(
                        translations["NewTagQuestion"],
                        128,
                        false,
                        function(answer)
                            -- success if the answer isnt blank
                            if answer:gsub("^%s*(.-)%s*$", "%1") ~= "" then
                                MESSAGEMAN:Broadcast("CreateNewTag", {name = answer})
                            else
                                on()
                            end
                        end,
                        function() return true, "" end,
                        function()
                            on()
                        end
                    )
                end,
            },
            {   -- Button to apply tag filter changes to music wheel
                Name = "apply",
                Type = "Tap",
                Display = {translations["ApplyFilter"]},
                Condition = function() return true end,
                IndexGetter = function() return 1 end,
                TapFunction = function()
                    -- really all this does is trigger a search
                    -- since the filter is always set to what you visually see, you just have to reload the wheel
                    local scr = SCREENMAN:GetTopScreen()
                    local w = scr:GetChild("WheelFile")
                    if w ~= nil then
                        w:sleep(0.01):queuecommand("ApplyFilter")
                    end
                    -- but we dont change the input context to keep it from being too jarring
                end,
            },
            {   -- Button to reset tag filters
            Name = "reset",
            Type = "Tap",
            Display = {translations["ResetFilter"]},
            Condition = function() return true end,
            IndexGetter = function() return 1 end,
            TapFunction = function()
                -- resets tags and triggers a search
                local scr = SCREENMAN:GetTopScreen()
                local w = scr:GetChild("WheelFile")
                if w ~= nil then
                    WHEELDATA:ResetExcludedTags()
                    WHEELDATA:ResetRequiredTags()
                    w:sleep(0.01):queuecommand("ApplyFilter")
                end
                -- but we dont change the input context to keep it from being too jarring
            end,
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
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
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
                        txt:strokecolor(Brightness(COLORS:getMainColor("PrimaryText"), 0.75))
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
                        self:GetParent():GetParent():playcommand("UpdateTagList")
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
            DeletedTagCommand = function(self)
                -- reset choice to Assign
                activeChoices = {[1]=true}
                self:playcommand("UpdateText")
            end,
        }

        for i = 1, #choiceDefinitions do
            t[#t+1] = createChoice(i)
        end

        return t
    end

    local t = Def.ActorFrame {
        Name = "TagListFrame",
        BeginCommand = function(self)
            self:playcommand("UpdateTagList")
            self:playcommand("UpdateText")
        end,
        UpdateTagsTabCommand = function(self)
            page = 1
            self:playcommand("UpdateTagList")
            self:playcommand("UpdateText")
        end,
        UpdateTagListCommand = function(self)
            -- this sets all the data things over and over and over
            -- but its all in one place and is only called once every time you touch the tag list stuff
            -- ... so its probably slow but only as slow as it needs to be
            -- (make sure you dont let this get called if you arent looking at the tag tab because thats a waste)
            -- (but if you then decide to look at the tag tab you should probably run this)
            storedTags = TAGMAN:get_data().playerTags
            tagNameList = {}
            for k, _ in pairs(storedTags) do
                tagNameList[#tagNameList+1] = k
            end
            table.sort(
                tagNameList,
                function(a,b) return a:lower() < b:lower() end
            )
            maxPage = math.ceil(#tagNameList / (columns * tagsPerColumn))

            requiredTags = {}
            excludedTags = {}
            for _, t in ipairs(WHEELDATA:GetRequiredTags()) do
                requiredTags[t] = true
            end
            for _, t in ipairs(WHEELDATA:GetExcludedTags()) do
                excludedTags[t] = true
            end
        end,
        CreateNewTagMessageCommand = function(self, params)
            -- only create tag if the name isnt blank
            if params ~= nil and params.name ~= nil and params.name ~= "" then
                -- and dont make duplicate tags
                -- (but allow alternate capitalization ...)
                if storedTags[params.name] == nil then
                    TAGMAN:get_data().playerTags[params.name] = {}
                    TAGMAN:set_dirty()
                    TAGMAN:save()
                    self:playcommand("UpdateTagList")
                end
            end
        end,
        DeletedTagCommand = function(self)
            -- just a little hacky but im putting this bit here so its all in one place
            -- on tag deletions we want to reset back to the assign state so you dont keep deleting things
            -- its better to be forced to be slow to delete than to accidentally delete tags over and over
            tagListMode = "Assign"
            self:playcommand("UpdateTagList")
            -- from here the next thing that happens is the choice frame will take this Command
            -- it will reset the choice to Assign to match
            -- and that part is hardcoded because ive unfortunately run out of patience
            -- (fortunately im like 95% done with this)
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
                    self:GetParent():playcommand("UpdateTagList")
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
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateTagListCommand = function(self)
                local lb = clamp((page-1) * (columns * tagsPerColumn) + 1, 0, #tagNameList)
                local ub = clamp(page * columns * tagsPerColumn, 0, #tagNameList)
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
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "SecondaryBackground")
    end
}

t[#t+1] = Def.Quad {
    Name = "LipTop",
    InitCommand = function(self)
        self:halign(0)
        self:zoomto(actuals.Width, actuals.LipSeparatorThickness)
        self:diffusealpha(0.3)
        registerActorToColorConfigElement(self, "main", "SeparationDivider")
    end
}

t[#t+1] = tagList()

return t