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
    PageNumberUpperGap = 48 / 1080, -- bottom of upper lip to top of text

    ItemListUpperGap = 35 / 1080, -- bottom of upper lip to top of topmost item
    ItemAllottedSpace = 405 / 1080, -- top of topmost item to top of bottommost item
    ItemLowerLineUpperGap = 30 / 1080, -- top of top line to top of bottom line
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

local translations = {
    DeleteGoal = THEME:GetString("ScreenSelectMusic Goals", "DeleteGoal"),
    AlreadyBeat = THEME:GetString("ScreenSelectMusic Goals", "AlreadyBeat"),
    VacuousGoal = THEME:GetString("ScreenSelectMusic Goals", "VacuousGoal"),
    AchievedGoal = THEME:GetString("ScreenSelectMusic Goals", "AchievedGoal"),
    SetGoal = THEME:GetString("ScreenSelectMusic Goals", "SetGoal"),
    PrioritySortDisplay = THEME:GetString("ScreenSelectMusic Goals", "PrioritySortDisplay"),
    RateSortDisplay = THEME:GetString("ScreenSelectMusic Goals", "RateSortDisplay"),
    MSDSortDisplay = THEME:GetString("ScreenSelectMusic Goals", "MSDSortDisplay"),
    NameSortDisplay = THEME:GetString("ScreenSelectMusic Goals", "NameSortDisplay"),
    DateSortDisplay = THEME:GetString("ScreenSelectMusic Goals", "DateSortDisplay"),
    NewGoal = THEME:GetString("ScreenSelectMusic Goals", "NewGoal"),
    FilterShowAll = THEME:GetString("ScreenSelectMusic Goals", "FilterShowAll"),
    FilterComplete = THEME:GetString("ScreenSelectMusic Goals", "FilterComplete"),
    FilterIncomplete = THEME:GetString("ScreenSelectMusic Goals", "FilterIncomplete"),
}

local goalLine1TextSize = 0.85
local goalLine2TextSize = 0.75
local pageTextSize = 0.7

-- our fontpage SUCKS so this should make things look better
-- undo this if the fontpage doesnt SUCK
-- update i fixed the font but it looked back after i undid this
local dividerUpwardBump = 1

local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local textzoomFudge = 5

local goalListAnimationSeconds = 0.05

local function byAchieved(scoregoal)
    if not scoregoal or scoregoal:IsAchieved() then
        return COLORS:getColor("generalBox", "GoalAchieved")
    end
    return COLORS:getColor("generalBox", "GoalDefault")
end

--=====
--  Sorting functions
--  used for sorting goal tables in various ways
--  we do this from lua because i dont want to let C++ control this
--  and im micromanaging most of the equal cases so it comes out as clean as possible
--  yea
--  my only justification for doing this in a drawn out and purpose specific manner is
--      that i want to make all the logic exposed and in one place
--
--  dategetter is a function that takes a goal as input and outputs a date
--  it lets you determine either to sort by Set date or Achieved date
--=====
local function sortGoalsByPriority(goaltable, dategetter, ascending)
    if ascending == nil then ascending = true end
    table.sort(
        goaltable,
        function(a, b)
            local aprio = a:GetPriority()
            local bprio = b:GetPriority()
            if aprio ~= bprio then
                if ascending then
                    return aprio < bprio
                else
                    return aprio > bprio
                end
            else
                -- priority the same, sort by set date
                local adate = dategetter(a)
                local bdate = dategetter(b)
                if adate ~= bdate then
                    -- whats funny about the dates is that because the format is consistent we can just compare the strings
                    -- ... i think
                    if ascending then
                        return adate < bdate
                    else
                        return adate > bdate
                    end
                else
                    -- date somehow the same, sort by song name
                    local ack = a:GetChartKey()
                    local bck = b:GetChartKey()
                    local asong = SONGMAN:GetSongByChartKey(ack)
                    local bsong = SONGMAN:GetSongByChartKey(bck)
                    local aname = asong ~= nil and asong:GetDisplayMainTitle() or ack
                    local bname = bsong ~= nil and bsong:GetDisplayMainTitle() or bck
                    if aname ~= bname then
                        -- lua handles string comparisons on a char/byte kind of level so a is different from A
                        -- we dont want to care and we also want to keep the ordering equivalent to the wheel ordering
                        aname = WHEELDATA.makeSortString(aname)
                        bname = WHEELDATA.makeSortString(bname)
                        if ascending then
                            return aname < bname
                        else
                            return aname > bname
                        end
                    else
                        -- name the same, sort by rate
                        local arate = a:GetRate()
                        local brate = b:GetRate()
                        if arate ~= brate then
                            if ascending then
                                return arate < brate
                            else
                                return arate > brate
                            end
                        else
                            -- rate the same, sort by MSD (this is as far as ill go)
                            local asteps = SONGMAN:GetStepsByChartKey(ack)
                            local bsteps = SONGMAN:GetStepsByChartKey(bck)
                            -- the next 2 lines here might be backwards
                            if asteps == nil then return ascending end
                            if bsteps == nil then return not ascending end

                            local amsd = asteps:GetMSD(arate, 1)
                            local bmsd = bsteps:GetMSD(brate, 1)
                            if ascending then
                                return amsd < bmsd
                            else
                                return amsd > bmsd
                            end
                        end
                    end
                end
            end
        end
    )
end
local function sortGoalsByRate(goaltable, dategetter, ascending)
    if ascending == nil then ascending = true end
    table.sort(
        goaltable,
        function(a, b)
            local arate = a:GetRate()
            local brate = b:GetRate()
            if arate ~= brate then
                if ascending then
                    return arate < brate
                else
                    return arate > brate
                end
            else
                -- rate is the same, sort by priority
                local aprio = a:GetPriority()
                local bprio = b:GetPriority()
                if aprio ~= bprio then
                    if ascending then
                        return aprio < bprio
                    else
                        return aprio > bprio
                    end
                else
                    -- priority the same, sort by song name (a rate sort would probably be comparing goals on rates on songs)
                    -- the alternative here would be to sort by set date which makes no sense to me if that is the assumption
                    local ack = a:GetChartKey()
                    local bck = b:GetChartKey()
                    local asong = SONGMAN:GetSongByChartKey(ack)
                    local bsong = SONGMAN:GetSongByChartKey(bck)
                    local aname = asong ~= nil and asong:GetDisplayMainTitle() or ack
                    local bname = bsong ~= nil and bsong:GetDisplayMainTitle() or bck
                    if aname ~= bname then
                        -- lua handles string comparisons on a char/byte kind of level so a is different from A
                        -- we dont want to care and we also want to keep the ordering equivalent to the wheel ordering
                        aname = WHEELDATA.makeSortString(aname)
                        bname = WHEELDATA.makeSortString(bname)
                        if ascending then
                            return aname < bname
                        else
                            return aname > bname
                        end
                    else
                        -- names are the same so sort by date and thats as far as we care
                        local adate = dategetter(a)
                        local bdate = dategetter(b)
                        if ascending then
                            return adate < bdate
                        else
                            return adate > bdate
                        end
                    end
                end
            end

        end
    )
end
local function sortGoalscolorByMSD(goaltable, dategetter, ascending)
    if ascending == nil then ascending = true end
    table.sort(
        goaltable,
        function(a, b)
            local ack = a:GetChartKey()
            local bck = b:GetChartKey()
            local asteps = SONGMAN:GetStepsByChartKey(ack)
            local bsteps = SONGMAN:GetStepsByChartKey(bck)
            if asteps == nil then return ascending end
            if bsteps == nil then return not ascending end
            local arate = a:GetRate()
            local brate = b:GetRate()

            local amsd = asteps:GetMSD(arate, 1)
            local bmsd = bsteps:GetMSD(brate, 1)
            if amsd ~= bmsd then
                if ascending then
                    return amsd < bmsd
                else
                    return amsd > bmsd
                end
            else
                -- msd is the same, sort by priority
                local aprio = a:GetPriority()
                local bprio = b:GetPriority()
                if aprio ~= bprio then
                    if ascendingn then
                        return aprio < bprio
                    else
                        return aprio > bprio
                    end
                else
                    -- priority is the same, sort by song name and thats as far as we care (msd is rarely equal to begin with)
                    local asong = SONGMAN:GetSongByChartKey(ack)
                    local bsong = SONGMAN:GetSongByChartKey(bck)
                    local aname = asong ~= nil and asong:GetDisplayMainTitle() or ack
                    local bname = bsong ~= nil and bsong:GetDisplayMainTitle() or bck
                    aname = WHEELDATA.makeSortString(aname)
                    bname = WHEELDATA.makeSortString(bname)
                    if ascending then
                        return aname < bname
                    else
                        return aname > bname
                    end
                end
            end
        end
    )
end
local function sortGoalsByName(goaltable, dategetter, ascending)
    if ascending == nil then ascending = true end
    table.sort(
        goaltable,
        function(a, b)
            local ack = a:GetChartKey()
            local bck = b:GetChartKey()
            local asong = SONGMAN:GetSongByChartKey(ack)
            local bsong = SONGMAN:GetSongByChartKey(bck)
            local aname = asong ~= nil and asong:GetDisplayMainTitle() or ack
            local bname = bsong ~= nil and bsong:GetDisplayMainTitle() or bck
            if aname ~= bname then
                aname = WHEELDATA.makeSortString(aname)
                bname = WHEELDATA.makeSortString(bname)
                if ascending then
                    return aname < bname
                else
                    return aname > bname
                end
            else
                -- names the same, sort by priority
                local aprio = a:GetPriority()
                local bprio = b:GetPriority()
                if aprio ~= bprio then
                    if ascending then
                        return aprio < bprio
                    else
                        return aprio > bprio
                    end
                else
                    -- priority the same, sort by rate and thats as far as we care
                    local arate = a:GetRate()
                    local brate = b:GetRate()
                    if ascending then
                        return arate < brate
                    else
                        return arate > brate
                    end
                end
            end
        end
    )
end
local function sortGoalsByDate(goaltable, dategetter, ascending)
    if ascending == nil then ascending = true end
    table.sort(
        goaltable,
        function(a, b)
            local adate = dategetter(a)
            local bdate = dategetter(b)
            if adate ~= bdate then
                if ascending then
                    return adate < bdate
                else
                    return adate > bdate
                end
            else
                -- date the same, sort by priority
                -- date should basically never be the same...
                local aprio = a:GetPriority()
                local bprio = b:GetPriority()
                if aprio ~= bprio then
                    if ascending then
                        return aprio < bprio
                    else
                        return aprio > bprio
                    end
                else
                    -- priority the same, sort by name and thats as far as we care
                    local ack = a:GetChartKey()
                    local bck = b:GetChartKey()
                    local asong = SONGMAN:GetSongByChartKey(ack)
                    local bsong = SONGMAN:GetSongByChartKey(bck)
                    local aname = asong ~= nil and asong:GetDisplayMainTitle() or ack
                    local bname = bsong ~= nil and bsong:GetDisplayMainTitle() or bck
                    aname = WHEELDATA.makeSortString(aname)
                    bname = WHEELDATA.makeSortString(bname)
                    if ascending then
                        return aname < bname
                    else
                        return aname > bname
                    end
                end
            end
        end
    )
end

-- end sorting functions
--=====

-- the entire goal ActorFrame
local function goalList()
    -- modifiable parameters
    local goalItemCount = 7

    -- internal var storage
    local page = 1
    local maxPage = 1
    local goalListFrame = nil
    local profile = GetPlayerOrMachineProfile(PLAYER_1)
    local goalTable = profile:GetGoalTable()

    -- sortmode info storage
    -- default to Priority Sort
    -- the order of preference for cases of equality is:
    -- valid choices: Priority, Rate, MSD, Name, Date
    local sortMode = "Priority"
    local defaultAscending = false
    local sortAscending = defaultAscending
    -- filter visible goals by achievement
    --  valid choices: All, Complete, Incomplete
    local visibleGoalType = "All"

    -- this resets the goal table and sorting done by the C++
    -- why is this done in C++ and not Lua? great question
    -- this is handled in Lua anyways (it would just change ascending and sortmodes for goals)
    profile:SetFromAll()

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

        if goalListFrame then
            goalListFrame:playcommand("UpdateGoalList")
        end
    end

    -- function to reset the goal list and filter it
    local function resortGoals()
        goalTable = {}

        -- filter the goal table by chosen visible goals
        for _, g in ipairs(profile:GetGoalTable()) do
            if visibleGoalType == "Complete" then
                -- complete only
                if g:IsAchieved() then
                    goalTable[#goalTable+1] = g
                end
            elseif visibleGoalType == "Incomplete" then
                -- incomplete only
                if not g:IsAchieved() and not g:IsVacuous() then
                    goalTable[#goalTable+1] = g
                end
            else
                -- all goals
                goalTable[#goalTable+1] = g
            end
        end

        page = 1
        maxPage = math.ceil(#goalTable / goalItemCount)

        -- set up date getter for sorting
        local dategetter
        if visibleGoalType == "Complete" then
            -- if the visible goal type is Complete then we care about the achieved date
            dategetter = function(goal)
                return goal:WhenAchieved()
            end
        else
            -- if the visible goal type is not Complete then we only care about the set date
            dategetter = function(goal)
                return goal:WhenAssigned()
            end
        end

        -- sort goals by sortmode
        if sortMode == "Priority" then
            sortGoalsByPriority(goalTable, dategetter, sortAscending)
        elseif sortMode == "Rate" then
            sortGoalsByRate(goalTable, dategetter, sortAscending)
        elseif sortMode == "MSD" then
            sortGoalscolorByMSD(goalTable, dategetter, sortAscending)
        elseif sortMode == "Name" then
            sortGoalsByName(goalTable, dategetter, sortAscending)
        elseif sortMode == "Date" then
            sortGoalsByDate(goalTable, dategetter, sortAscending)
        else
            sortGoalsByPriority(goalTable, dategetter, sortAscending)
        end
    end

    local function goalListItem(i)
        local index = i
        local goal = nil
        local goalSteps = nil

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
        local msdW = remainingWidth / 60 * 13 - actuals.IconWidth * 2 -- above comment -- 13/60
        local msdX = div3X + msdW/2
        local itemHeight = (actuals.ItemAllottedSpace / (goalItemCount - 1))

        return Def.ActorFrame {
            Name = "GoalItemFrame_"..i,
            InitCommand = function(self)
                self:y(itemHeight * (i-1) + actuals.ItemListUpperGap + actuals.UpperLipHeight)
            end,
            UpdateGoalListCommand = function(self)
                index = (page - 1) * goalItemCount + i
                goal = goalTable[index]
                goalSteps = nil
                self:finishtweening()
                self:diffusealpha(0)
                if goal ~= nil then
                    goalSteps = SONGMAN:GetStepsByChartKey(goal:GetChartKey())
                    self:playcommand("UpdateText")
                    self:smooth(goalListAnimationSeconds * i)
                    self:diffusealpha(1)
                end
            end,
            GoalsUpdatedMessageCommand = function(self)
                -- ONLY TRIGGERED BY CTRL+G
                -- MAKE SURE THIS MIMICS THE NEW GOAL BUTTON

                -- this will load the new goal into the list and keep the page where it already was
                local pagebefore = page
                profile:SetFromAll()
                resortGoals()
                page = clamp(pagebefore, 1, maxPage)
                self:playcommand("UpdateGoalList")
                self:playcommand("UpdateText")
            end,

            Def.Quad {
                Name = "BG",
                InitCommand = function(self)
                    self:halign(0)
                    self:valign(0)
                    self:zoomto(itemWidth, itemHeight * 0.95)
                    self:y(-itemHeight/8)
                    self:diffusealpha(0.1)
                    registerActorToColorConfigElement(self, "generalBox", "GoalBackground")
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Priority",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0):valign(0)
                    bg:halign(0)

                    self:x(prioX)
                    txt:zoom(goalLine1TextSize)
                    txt:maxwidth(prioW / goalLine1TextSize - textzoomFudge)
                    txt:settext(" ")
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                    bg:zoomto(prioW, txt:GetZoomedHeight())
                    bg:y(txt:GetZoomedHeight() / 2)
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end
                    local txt = self:GetChild("Text")

                    txt:settextf("%d.", goal:GetPriority())
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if goal == nil then return end
                    if goal:IsAchieved() or goal:IsVacuous() then return end -- completed goals cant be updated
                    if params.update == "OnMouseDown" then
                        if params.event == "DeviceButton_left mouse button" then
                            goal:SetPriority(goal:GetPriority() + 1)
                            self:GetParent():GetParent():playcommand("UpdateGoalList")
                        elseif params.event == "DeviceButton_right mouse button" then
                            goal:SetPriority(goal:GetPriority() - 1)
                            self:GetParent():GetParent():playcommand("UpdateGoalList")
                        end
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
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("UpdateText")
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end

                    if goalSteps ~= nil then
                        self:settextf("%s", getShortDifficulty(goalSteps:GetDifficulty()))
                        self:diffuse(colorByDifficulty(goalSteps:GetDifficulty()))
                    else
                        self:settext("")
                    end
                end
            },
            Def.Quad {
                Name = "Div1",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(div1X)
                    self:y(-dividerUpwardBump)
                    self:zoomto(actuals.ItemDividerThickness, actuals.ItemDividerLength)
                    registerActorToColorConfigElement(self, "main", "SeparationDivider")
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Rate",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:valign(0)

                    self:x(rateX)
                    txt:zoom(goalLine1TextSize)
                    txt:maxwidth(rateW / goalLine1TextSize - textzoomFudge)
                    txt:settext(" ")
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                    bg:zoomto(rateW, txt:GetZoomedHeight())
                    bg:y(txt:GetZoomedHeight() / 2)
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end
                    local txt = self:GetChild("Text")

                    local ratestr = string.format("%.2f", goal:GetRate()):gsub("%.?0$", "") .. "x"
                    txt:settext(ratestr)
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if goal == nil then return end
                    if goal:IsAchieved() then return end -- completed goals cant be updated
                    if params.update == "OnMouseDown" then
                        if params.event == "DeviceButton_left mouse button" then
                            goal:SetRate(goal:GetRate() + 0.05)
                            self:GetParent():GetParent():playcommand("UpdateGoalList")
                        elseif params.event == "DeviceButton_right mouse button" then
                            goal:SetRate(goal:GetRate() - 0.05)
                            self:GetParent():GetParent():playcommand("UpdateGoalList")
                        end
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
                    registerActorToColorConfigElement(self, "main", "SeparationDivider")
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Percentage",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:valign(0)

                    self:x(percentX)
                    txt:zoom(goalLine1TextSize)
                    txt:maxwidth(percentW / goalLine1TextSize - textzoomFudge)
                    txt:settext(" ")
                    bg:zoomto(percentW, txt:GetZoomedHeight())
                    bg:y(txt:GetZoomedHeight() / 2)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("UpdateText")
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end
                    local txt = self:GetChild("Text")

                    local finalstr = ""

                    local perc = notShit.round(goal:GetPercent() * 100000) / 1000
                    local percStr = ""
                    if perc <= 99 or perc == 100 then
                        percStr = string.format("%5.2f", perc)
                    elseif (perc < 99.8) then
                        percStr = string.format("%5.2f", perc)
                    else
                        percStr = string.format("%5.3f", perc)
                    end

                    local pb = goal:GetPBUpTo()
                    local pbStr = ""
                    if pb then
                        local pbperc = notShit.round(pb:GetWifeScore() * 100000) / 1000
                        if pbperc <= 99 or pbperc == 100 then
                            pbStr = string.format("%5.2f", pbperc)
                        elseif (perc < 99.8) then
                            pbStr = string.format("%5.2f", pbperc)
                        else
                            pbStr = string.format("%5.3f", pbperc)
                        end

                        local rstr = ""
                        if pb:GetMusicRate() < goal:GetRate() then
                            rstr = string.format(" %5.2f", pb:GetMusicRate()):gsub("%.?0$", "") .. "x"
                        end
                        -- these gsubs get rid of right trailing 0s and .
                        finalstr = string.format("%s (%s%s)", percStr:gsub("%.?0+$", "") .. "%", pbStr:gsub("%.?0+$", "") .. "%", rstr)
                    else
                        -- these gsubs get rid of right trailing 0s and .
                        finalstr = string.format("%s", percStr:gsub("%.?0+$", "") .. "%")
                    end

                    txt:settext(finalstr)
                    txt:diffuse(byAchieved(goal))
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if goal == nil then return end
                    if goal:IsAchieved() then return end -- completed goals cant be updated
                    if params.update == "OnMouseDown" then
                        if params.event == "DeviceButton_left mouse button" then
                            goal:SetPercent(goal:GetPercent() + 0.01)
                            self:GetParent():GetParent():playcommand("UpdateGoalList")
                        elseif params.event == "DeviceButton_right mouse button" then
                            goal:SetPercent(goal:GetPercent() - 0.01)
                            self:GetParent():GetParent():playcommand("UpdateGoalList")
                        end
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
                    registerActorToColorConfigElement(self, "main", "SeparationDivider")
                end
            },
            LoadFont("Common Normal") .. {
                Name = "MSD",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(msdX)
                    self:zoom(goalLine1TextSize)
                    -- the trashcan intrudes in this area so dont let them overlap
                    self:maxwidth(msdW / goalLine1TextSize - textzoomFudge)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("UpdateText")
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end

                    if goalSteps ~= nil then
                        local msd = goalSteps:GetMSD(goal:GetRate(), 1)
                        self:settextf("%5.1f", msd)
                        self:diffuse(colorByMSD(msd))
                    else
                        self:settext("??")
                        self:diffuse(color("1,1,1,1"))
                    end
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "deleteGoal")) .. {
                Name = "DeleteGoal",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(msdX + msdW/2)
                    self:zoomto(actuals.IconWidth, actuals.IconHeight)
                    registerActorToColorConfigElement(self, "main", "IconColor")
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then
                        self:diffusealpha(0)
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText(translations["DeleteGoal"])
                            TOOLTIP:Show()
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if goal == nil then return end

                    if params.event == "DeviceButton_left mouse button" then
                        -- delete goal and then refresh the list
                        goal:Delete()
                        local pagebefore = page
                        -- this updates the list that comes from resetting the goal table
                        -- (if you dont do this then delete a goal twice you crash)
                        profile:SetFromAll()
                        resortGoals()
                        page = clamp(pagebefore, 1, maxPage)
                        self:GetParent():GetParent():playcommand("UpdateGoalList")
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:SetText(translations["DeleteGoal"])
                    TOOLTIP:Show()
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:Hide()
                    self:diffusealpha(1)
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:valign(0):halign(0)
                    self:x(prioX + prioW/2)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(goalLine2TextSize)
                    self:maxwidth((div2X - prioX - prioW/2) / goalLine2TextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(1)
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        self:diffusealpha(1)
                        local ck = goal:GetChartKey()
                        local wheel = SCREENMAN:GetTopScreen():GetChild("WheelFile")
                        if wheel then
                            setMusicRate(goal:GetRate())
                            wheel:playcommand("FindSong", {chartkey = ck})
                        end
                    end
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end

                    local sname = ""
                    if goalSteps ~= nil then
                        sname = SONGMAN:GetSongByChartKey(goal:GetChartKey()):GetDisplayMainTitle()
                    else
                        sname = goal:GetChartKey()
                    end

                    self:settextf("%s", sname)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Date",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(percentX)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(goalLine2TextSize)
                    self:maxwidth(percentW / goalLine2TextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                UpdateTextCommand = function(self)
                    if goal == nil then return end

                    local when = ""
                    local status = goal:IsAchieved() and "Achieved" or (goal:IsVacuous() and "Vacuous" or "Set")

                    if status == "Achieved" then
                        when = extractDateFromDateString(goal:WhenAchieved())
                    elseif status == "Vacuous" then
                        when = "- " .. translations["AlreadyBeat"]
                    else
                        -- Created/Set
                        when = extractDateFromDateString(goal:WhenAssigned())
                    end

                    self:settextf("%s %s", translations[status.."Goal"], when)
                end
            }
        }
    end

    local function goalChoices()
        -- keeping track of which choices are on at any moment (keys are indices, values are true/false/nil)
        -- starting with priority sort on
        local activeChoices = {[1] = true}

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
                Name = "prioritysort",
                Type = "Exclusive",
                Display = {translations["PrioritySortDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    if sortMode == "Priority" then
                        sortAscending = not sortAscending
                    else
                        sortMode = "Priority"
                        sortAscending = defaultAscending
                    end
                    resortGoals()
                end,
            },
            {   -- Sort by Rate
                Name = "ratesort",
                Type = "Exclusive",
                Display = {translations["RateSortDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    if sortMode == "Rate" then
                        sortAscending = not sortAscending
                    else
                        sortMode = "Rate"
                        sortAscending = defaultAscending
                    end
                    resortGoals()
                end,
            },
            {   -- Sort by MSD
                Name = "msdsort",
                Type = "Exclusive",
                Display = {translations["MSDSortDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    if sortMode == "MSD" then
                        sortAscending = not sortAscending
                    else
                        sortMode = "MSD"
                        sortAscending = defaultAscending
                    end
                    resortGoals()
                end,
            },
            {   -- Sort by Song (Song + ChartKey)
                Name = "namesort",
                Type = "Exclusive",
                Display = {translations["NameSortDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    if sortMode == "Name" then
                        sortAscending = not sortAscending
                    else
                        sortMode = "Name"
                        sortAscending = defaultAscending
                    end
                    resortGoals()
                end,
            },
            {   -- Sort by Set Date
                Name = "datesort",
                Type = "Exclusive",
                Display = {translations["DateSortDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    if sortMode == "Date" then
                        sortAscending = not sortAscending
                    else
                        sortMode = "Date"
                        sortAscending = defaultAscending
                    end
                    resortGoals()
                end,
            },
            {   -- New Goal on current Chart
                Name = "newgoal",
                Type = "Tap",
                Display = {translations["NewGoal"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    local steps = GAMESTATE:GetCurrentSteps()
                    if steps ~= nil then
                        local ck = steps:GetChartKey()
                        local success = profile:AddGoal(ck)
                        -- success means goal was added
                        -- false means it was a duplicate or something else weird maybe
                        if success then
                            -- this will load the new goal into the list and keep the page where it already was
                            local pagebefore = page
                            profile:SetFromAll()
                            resortGoals()
                            page = clamp(pagebefore, 1, maxPage)
                        end
                    end
                end,
            },
            {   -- Toggle between All, Completed, and Incomplete Goals
                Name = "filtergoals",
                Type = "Toggle",
                Display = {translations["FilterShowAll"], translations["FilterComplete"], translations["FilterIncomplete"]},
                IndexGetter = function()
                    if visibleGoalType == "All" then
                        return 1
                    elseif visibleGoalType == "Complete" then
                        return 2
                    elseif visibleGoalType == "Incomplete" then
                        return 3
                    else
                        return 1
                    end
                end,
                Condition = function() return true end,
                TapFunction = function()
                    if visibleGoalType == "All" then
                        visibleGoalType = "Complete"
                    elseif visibleGoalType == "Complete" then
                        visibleGoalType = "Incomplete"
                    elseif visibleGoalType == "Incomplete" then
                        visibleGoalType = "All"
                    else
                        visibleGoalType = "All"
                    end
                    resortGoals()
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
            goalListFrame = self
            resortGoals()
            self:playcommand("UpdateGoalList")
            self:playcommand("UpdateText")
        end,
        UpdateGoalsTabCommand = function(self)
            page = 1
            self:playcommand("UpdateGoalList")
            self:playcommand("UpdateText")
        end,
        UpdateGoalListCommand = function(self)
            -- in case tooltip is stuck for some reason
            TOOLTIP:Hide()
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
                -- oddly precise max width but this should fit with the original size
                self:maxwidth(actuals.Width * 0.14 / pageTextSize - textzoomFudge)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateGoalListCommand = function(self)
                local lb = clamp((page-1) * (goalItemCount) + 1, 0, #goalTable)
                local ub = clamp(page * goalItemCount, 0, #goalTable)
                self:settextf("%d-%d/%d", lb, ub, #goalTable)
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

t[#t+1] = goalList()

return t
