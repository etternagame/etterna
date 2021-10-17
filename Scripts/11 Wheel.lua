local Wheel = {}
local function fillNilTableFieldsFrom(table1, defaultTable)
    for key, value in pairs(defaultTable) do
        if table1[key] == nil then
            table1[key] = defaultTable[key]
        end
    end
end

local function getIndexCircularly(table, idx)
    if #table == 0 then return 1 end
    if idx <= 0 then
        return getIndexCircularly(table, idx + #table)
    elseif idx > #table then
        return getIndexCircularly(table, idx - #table)
    end
    return idx
end

-- false if outside of a group
-- true if inside a group
-- toggles within the move function
-- becomes false if all groups are closed
local crossedGroupBorder = false
local forceGroupCheck = false
local diffSelection = 1 -- index of the selected chart

Wheel.mt = {
    updateMusicFromCurrentItem = function(whee)
        SOUND:StopMusic()

        local top = SCREENMAN:GetTopScreen()
        -- only for ScreenSelectMusic
        if top.PlayCurrentSongSampleMusic then
            if GAMESTATE:GetCurrentSong() ~= nil then
                -- chart preview active? dont play music
                -- chart preview handles music on its own
                if SCUFF.preview.active then return end

                -- currentItem should be a song
                top:PlayCurrentSongSampleMusic(false, false)
            end
        end
    end,
    updateGlobalsFromCurrentItem = function(whee)
        -- update Gamestate current song
        local currentItem = whee:getItem(whee.index)
        if currentItem.GetDisplayMainTitle then
            -- currentItem is a SONG
            GAMESTATE:SetCurrentSong(currentItem)
            GAMESTATE:SetPreferredSong(currentItem)

            -- dude how do we even mimic the spaghetti behavior the c++ causes
            local function findTheDiffToUseBasedOnStepsTypeAndDifficultyBothPreferred(charts, prefdiff, stepstype)
                local diffs = {
                    ["none"] = 0,
                    ["Difficulty_Beginner"] = 1,
                    ["Difficulty_Easy"] = 2,
                    ["Difficulty_Medium"] = 3,
                    ["Difficulty_Hard"] = 4,
                    ["Difficulty_Challenge"] = 5,
                    ["Difficulty_Edit"] = 6,
                }
                local dadiff = "none"
                local smallestdifferencefrompreferreddifficulty = 20
                local index = 1

                -- YOU CANT STOP ME FROM NESTING FUNCTIONS
                local function getTheDifferenceBetweenTwoDifficultiesAbsolutely(d1,d2)
                    -- so many nil errors im not going to try to figure out the behavior for
                    -- hahahaha HAHAHAH AH AH AHAHAHADASHGDJASHDASGSA
                    -- (good luck future reader)
                    if d1 == nil or d2 == nil then
                        return 0
                    else
                        return math.abs(diffs[d1] - diffs[d2])
                    end
                end

                for i, chart in ipairs(charts) do
                    -- look for the closest and/or preferred difficulty for this stepstype
                    if chart:GetStepsType() == stepstype then
                        if chart:GetDifficulty() == prefdiff then
                            dadiff = chart:GetDifficulty()
                            index = i
                            break
                        end

                        local difference = getTheDifferenceBetweenTwoDifficultiesAbsolutely(chart:GetDifficulty(), prefdiff)
                        if difference <= smallestdifferencefrompreferreddifficulty then
                            dadiff = chart:GetDifficulty()
                            smallestdifferencefrompreferreddifficulty = difference
                            index = i
                        end
                    end
                end
                -- look for the diff that matches closest to the current one
                if dadiff == "none" then
                    for i, chart in ipairs(charts) do
                        if chart:GetDifficulty() == prefdiff then
                            dadiff = chart:GetDifficulty()
                            index = i
                            break
                        end

                        local difference = getTheDifferenceBetweenTwoDifficultiesAbsolutely(chart:GetDifficulty(), prefdiff)
                        if difference <= smallestdifferencefrompreferreddifficulty then
                            dadiff = chart:GetDifficulty()
                            smallestdifferencefrompreferreddifficulty = difference
                            index = i
                        end
                    end
                end
                if dadiff == "none" then
                    -- only possible if no charts were given...
                end
                return index, dadiff
            end

            -- setting diff stuff
            local stepslist = WHEELDATA:GetChartsMatchingFilter(currentItem)
            if #stepslist == 0 then
                -- this scenario should be impossible but lets prepare for the case
                GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
            else
                local prefdiff = GAMESTATE:GetPreferredDifficulty()
                
                diffSelection = findTheDiffToUseBasedOnStepsTypeAndDifficultyBothPreferred(stepslist, prefdiff, GAMESTATE:GetPreferredStepsType())
                diffSelection = clamp(diffSelection, 1, #stepslist)

                GAMESTATE:SetCurrentSteps(PLAYER_1, stepslist[diffSelection])
            end
        else
            -- currentItem is a GROUP
            GAMESTATE:SetCurrentSong(nil)
            GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
        end
    end,
    move = function(whee, num)
        if num == whee.moving then return end

        if whee.moving ~= 0 and num == 0 and whee.timeBeforeMovingBegins == 0 then
            if math.abs(whee.positionOffsetFromSelection) < 0.25 then
                whee:changemusic(whee.moving)
            end
        end

        whee.timeBeforeMovingBegins = 1 / 8 -- this was hardcoded and there is no justification for it
        whee.spinSpeed = 15 -- preference based (CHANGE THIS)
        whee.moving = num

        if whee.moving ~= 0 then
            whee:changemusic(whee.moving)
        end

        -- stop the music if moving so we dont leave it playing in a random place
        SOUND:StopMusic()
    end,
    changemusic = function(whee, num)
        whee.index = getIndexCircularly(whee.items, whee.index + num)
        whee.positionOffsetFromSelection = whee.positionOffsetFromSelection + num
        MESSAGEMAN:Broadcast("WheelIndexChanged", {
            index = whee.index,
            maxIndex = #whee.items,
        })
    end,
    setNewState = function(whee, index, startindex, itemsgetter, items, group)
        -- effectively moves the song wheel based on the params you give, but it needs to have an update run
        whee.index = index
        whee.startIndex = startindex
        whee.itemsGetter = itemsgetter
        whee.items = items
        whee.group = group
    end,
    findSong = function(whee, chartkey) -- returns the group that opened while finding this song
        local song = nil
        local steps = nil

        -- either use profile/preference based song or locate by chartkey
        if chartkey == nil then
            song = GAMESTATE:GetPreferredSong()
        else
            song = SONGMAN:GetSongByChartKey(chartkey)
        end

        -- jump to the first instance of the song if it exists
        if song ~= nil then
            local newItems, songgroup, finalIndex = WHEELDATA:GetWheelItemsAndGroupAndIndexForSong(song)

            if chartkey == nil then
                steps = WHEELDATA:GetChartsMatchingFilter(song)[1]
            else
                steps = SONGMAN:GetStepsByChartKey(chartkey)
            end

            WHEELDATA:SetWheelItems(newItems)
            whee:setNewState(
                finalIndex,
                finalIndex,
                function() return WHEELDATA:GetWheelItems() end,
                newItems,
                songgroup
            )
            GAMESTATE:SetCurrentSong(song)
            GAMESTATE:SetCurrentSteps(PLAYER_1, steps)

            return songgroup
        else
            return nil
        end
    end,
    findGroup = function(whee, name, openGroup) -- returns success status
        if name == nil then name = whee.group end
        if name == nil then return nil end

        local items = WHEELDATA:GetFilteredFolders()
        local index = WHEELDATA:FindIndexOfFolder(name)

        if index == -1 then return false end
        if openGroup then
            items = WHEELDATA:GetWheelItemsForOpenedFolder(name)
        end
        WHEELDATA:SetWheelItems(items)
        whee:setNewState(
            index,
            index,
            function() return WHEELDATA:GetWheelItems() end,
            items,
            nil
        )
        GAMESTATE:SetCurrentSong(nil)
        GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
        return true
    end,
    exitGroup = function(whee)
        if whee.group == nil then return end
        crossedGroupBorder = false
        forceGroupCheck = true
        whee:findGroup(whee.group, false)
        whee:updateGlobalsFromCurrentItem()
        whee:updateMusicFromCurrentItem()
        whee:rebuildFrames()
        MESSAGEMAN:Broadcast("ClosedGroup", {
            group = whee.group,
        })
        MESSAGEMAN:Broadcast("ModifiedGroups", {
            group = whee.group,
            index = whee.index,
            maxIndex = #whee.items,
        })
        MESSAGEMAN:Broadcast("WheelSettled", {
            song = nil,
            group = whee.group,
            hovered = whee:getCurrentItem(),
            steps = nil,
            index = whee.index,
            maxIndex = #whee.items,
        })
    end,
    openSortModeMenu = function(w)
        -- 0 is the sort mode menu
        WHEELDATA:SetCurrentSort(0)
        WHEELDATA:UpdateFilteredSonglist()

        local newItems = WHEELDATA:GetFilteredFolders()
        WHEELDATA:SetWheelItems(newItems)

        w:setNewState(
            1,
            1,
            function() return WHEELDATA:GetWheelItems() end,
            newItems,
            nil
        )
        crossedGroupBorder = true
        forceGroupCheck = true
        GAMESTATE:SetCurrentSong(nil)
        GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
        
        MESSAGEMAN:Broadcast("ClosedGroup", {
            group = w.group
        })
        w:rebuildFrames()
        MESSAGEMAN:Broadcast("ModifiedGroups", {
            group = w.group,
            index = w.index,
            maxIndex = #w.items,
        })
        w:updateGlobalsFromCurrentItem()
        w:updateMusicFromCurrentItem()
        MESSAGEMAN:Broadcast("WheelSettled", {
            song = GAMESTATE:GetCurrentSong(),
            group = w.group,
            hovered = w:getCurrentItem(),
            steps = GAMESTATE:GetCurrentSteps(),
            index = w.index, maxIndex = #w.items,
        })
        w.settled = true
    end,
    getItem = function(whee, idx)
        return whee.items[getIndexCircularly(whee.items, idx)]
        -- For some reason i have to +1 here
        -- (why am i not doing it anymore?)
    end,
    getCurrentItem = function(whee)
        return whee:getItem(whee.index)
    end,
    getFrame = function(whee, idx)
        return whee.frames[getIndexCircularly(whee.frames, idx)]
        -- For some reason i have to +1 here
        -- (why am i not doing it anymore?)
    end,
    getCurrentFrame = function(whee)
        return whee:getFrame(whee.index)
    end,
    update = function(whee)
        -- this is written so that iteration runs in a specific direction
        -- pretty much to avoid certain texture updating issues
        local direction = whee.positionOffsetFromSelection >= 0 and 1 or -1
        local startI = direction == 1 and 1 or #whee.frames
        local endI = startI == 1 and #whee.frames or 1

        local numFrames = #(whee.frames)
        local idx = whee.index
        idx = idx - (direction == 1 and math.ceil(numFrames / 2) or -math.floor(numFrames / 2) + 1)

        for i = startI, endI, direction do
            local frame = whee.frames[i]
            local offset = i - math.ceil(numFrames / 2) + whee.positionOffsetFromSelection
            whee.frameTransformer(frame, offset - 1, i, whee.count)
            whee.frameUpdater(frame, whee:getItem(idx), offset)
            idx = idx + direction
        end

        -- handle scrolling into and out of groups
        if whee.group then
            if whee:getCurrentItem().GetDisplayMainTitle then
                if forceGroupCheck or not crossedGroupBorder then
                    crossedGroupBorder = true
                    forceGroupCheck = false
                    MESSAGEMAN:Broadcast("ScrolledIntoGroup", {
                        group = whee.group
                    })
                end
            else
                if forceGroupCheck or crossedGroupBorder then
                    crossedGroupBorder = false
                    forceGroupCheck = false
                    MESSAGEMAN:Broadcast("ScrolledOutOfGroup", {
                        group = whee.group
                    })
                end
            end

        end

        -- the wheel has settled
        if whee.positionOffsetFromSelection == 0 and not whee.settled then
            whee:updateGlobalsFromCurrentItem()
            whee:updateMusicFromCurrentItem()
            -- settled brings along the Song, Group, Steps, and HoveredItem
            -- Steps should be set correctly immediately on Move, so no problems should arise.
            MESSAGEMAN:Broadcast("WheelSettled", {
                song = GAMESTATE:GetCurrentSong(),
                group = whee.group,
                hovered = whee:getCurrentItem(),
                steps = GAMESTATE:GetCurrentSteps(),
                index = whee.index,
                maxIndex = #whee.items
            })
            whee.settled = true
        end
        if whee.positionOffsetFromSelection ~= 0 then
            whee.settled = false
        end
    end,
    rebuildFrames = function(whee, newIndex)
        whee.items = whee.itemsGetter()
        if whee.sort then
            table.sort(whee.items, whee.sort)
        end
        if not whee.index then
            whee.index = newIndex or whee.startIndex
        end
        whee:update()
    end
}

Wheel.defaultParams = {
    itemsGetter = function()
        -- Should return an array table of elements for the wheel
        -- This is a function so it can be delayed, and rebuilt
        --  with different items using this function
        return SONGMAN:GetAllSongs()
    end,
    count = 20,
    frameBuilder = function()
        return LoadFont("Common Normal") .. {}
    end,
    frameUpdater = function(frame, item) -- Update an frame created with frameBuilder with an item
        frame:settext(item:GetMainTitle())
    end,
    x = 0,
    y = 0,
    highlightBuilder = function()
        return Def.ActorFrame {}
    end,
    buildOnInit = true, -- Build wheel in InitCommand (Will be empty until rebuilt otherwise)
    frameTransformer = function(frame, offsetFromCenter, index, total) -- Handle frame positioning
        frame:y(offsetFromCenter * 30)
    end,
    startIndex = 1,
    speed = 15,
    onSelection = nil, -- function(item)
    sort = nil -- function(a,b) return boolean end
}
function Wheel:new(params)
    params = params or {}
    fillNilTableFieldsFrom(params, Wheel.defaultParams)
    local whee = Def.ActorFrame {Name = "Wheel"}
    setmetatable(whee, {__index = Wheel.mt})
    crossedGroupBorder = false -- reset default
    diffSelection = 1 -- reset default
    whee.settled = false -- leaving this false causes 1 settle message on init
    whee.itemsGetter = params.itemsGetter
    whee.count = params.count
    whee.sort = params.sort
    whee.startIndex = params.startIndex
    whee.frameUpdater = params.frameUpdater
    whee.buildOnInit = params.buildOnInit
    whee.frameTransformer = params.frameTransformer
    whee.index = whee.startIndex
    whee.onSelection = params.onSelection
    whee.positionOffsetFromSelection = 0
    whee.moving = 0
    whee.timeBeforeMovingBegins = 0
    whee.x = params.x
    whee.y = params.y
    whee.items = {}
    whee.BeginCommand = function(self)
        local snm = SCREENMAN:GetTopScreen():GetName()
        local anm = self:GetName()
        CONTEXTMAN:RegisterToContextSet(snm, "Main1", anm)
        local heldButtons = {}
        local buttonQueue = {}
        SCREENMAN:GetTopScreen():AddInputCallback(
            function(event)
                local gameButton = event.button
                local key = event.DeviceInput.button
                local left = gameButton == "MenuLeft" or gameButton == "Left"
                local enter = gameButton == "Start"
                local right = gameButton == "MenuRight" or gameButton == "Right"
                local exit = gameButton == "Back"
                local up = gameButton == "Up" or gameButton == "MenuUp"
                local down = gameButton == "Down" or gameButton == "MenuDown"
                local keydirection = key == "DeviceButton_left" or key == "DeviceButton_right"

                if event.type == "InputEventType_FirstPress" then
                    buttonQueue[#buttonQueue + 1] = gameButton
                    if #buttonQueue > 4 then
                        buttonQueue[1] = buttonQueue[2]
                        buttonQueue[2] = buttonQueue[3]
                        buttonQueue[3] = buttonQueue[4]
                        buttonQueue[4] = buttonQueue[5]
                        buttonQueue[5] = nil
                    end
                    -- check for up down up down
                    local function u(b) return b == "Up" or b == "MenuUp" end
                    local function d(b) return b == "Down" or b == "MenuDown" end
                    if u(buttonQueue[1]) and d(buttonQueue[2]) and u(buttonQueue[3]) and d(buttonQueue[4]) then
                        -- open sort mode menu
                        whee:openSortModeMenu()
                        buttonQueue = {}
                        return false
                    end
                end

                if left or right then
                    local direction = left and "left" or "right"
                    if event.type == "InputEventType_FirstPress" or event.type == "InputEventType_Repeat" then
                        -- dont allow input, but do allow left and right arrow input
                        if not CONTEXTMAN:CheckContextSet(snm, "Main1") and not keydirection then return end
                        heldButtons[direction] = true
                        
                        if (left and heldButtons["right"]) or (right and heldButtons["left"]) then
                            -- dont move if holding both buttons
                            whee:move(0)
                        else
                            -- move on single press or repeat
                            whee:move(right and 1 or -1)
                        end
                    elseif event.type == "InputEventType_Release" then
                        heldButtons[direction] = false
                        if heldButtons["left"] == false and heldButtons["right"] == false then
                            -- cease movement
                            whee:move(0)
                        end
                    end
                elseif enter then
                    if event.type == "InputEventType_FirstPress" then
                        if not CONTEXTMAN:CheckContextSet(snm, "Main1") then return end
                        whee.onSelection(whee:getCurrentFrame(), whee:getCurrentItem())
                    end
                elseif exit then
                    if event.type == "InputEventType_FirstPress" then
                        if not CONTEXTMAN:CheckContextSet(snm, "Main1") then return end
                        SCREENMAN:set_input_redirected(PLAYER_1, false)
                        SCREENMAN:GetTopScreen():Cancel()
                    end
                elseif up or down then
                    local direction = up and "up" or "down"
                    if event.type == "InputEventType_FirstPress" then
                        if not CONTEXTMAN:CheckContextSet(snm, "Main1") then return end
                        heldButtons[direction] = true
                        if heldButtons["up"] and heldButtons["down"] then
                            whee:exitGroup()
                        end
                    elseif event.type == "InputEventType_Release" then
                        heldButtons[direction] = false
                    end
                end
                return false
            end
        )
        self:SetUpdateFunction(
            function(self, delta)
                -- begin the moving
                if whee.moving ~= 0 then
                    whee.timeBeforeMovingBegins = clamp(whee.timeBeforeMovingBegins - delta, 0, whee.timeBeforeMovingBegins)
                end

                -- little hack to make sure the wheel doesnt update when nothing has to move
                if whee.positionOffsetFromSelection ~= 0 then
                    whee:update()
                    whee.keepupdating = true
                elseif whee.keepupdating then
                    whee.keepupdating = false
                    whee:update()
                end

                -- handle wheel movement moving moves
                if whee.moving ~= 0 and whee.timeBeforeMovingBegins == 0 then
                    local sping = whee.spinSpeed * whee.moving
                    whee.positionOffsetFromSelection = clamp(whee.positionOffsetFromSelection - (sping * delta), -1, 1)
                    if (whee.moving == -1 and whee.positionOffsetFromSelection >= 0) or
                        (whee.moving == 1 and whee.positionOffsetFromSelection <= 0) then
                            whee:changemusic(whee.moving)
                    end
                    -- maybe play moving sound here based on delta 
                else
                    -- the wheel should rotate toward selection but isnt "moving"
                    local sping = 0.2 + (math.abs(whee.positionOffsetFromSelection) / 0.1)
                    if whee.positionOffsetFromSelection > 0 then
                        whee.positionOffsetFromSelection = math.max(whee.positionOffsetFromSelection - (sping * delta), 0)
                    elseif whee.positionOffsetFromSelection < 0 then
                        whee.positionOffsetFromSelection = math.min(whee.positionOffsetFromSelection + (sping * delta), 0)
                    end
                end
            end
        )
        -- default interval is 0.016 which is TOO SLOW C++ IS LITERALLY 0 WTF
        self:SetUpdateFunctionInterval(0.001)

        -- mega hack to make things init 0.1 seconds after real init
        SCREENMAN:GetTopScreen():setTimeout(
            function()
                if params.buildOnInit then
                    whee:rebuildFrames()
                end
            end,
            0.1
        )
    end
    whee.InitCommand = function(self)
        whee.actor = self
        self:x(whee.x):y(whee.y)
    end
    whee.frames = {}
    for i = 1, (params.count) do
        local frame = params.frameBuilder() .. {
            InitCommand = function(self)
                whee.frames[i] = self
                self.index = i
            end
        }
        whee[#whee + 1] = frame
    end
    whee[#whee + 1] = params.highlightBuilder() .. {
        InitCommand = function(self)
            whee.highlight = self
        end
    }
    return whee
end
MusicWheel = {}
MusicWheel.defaultParams = {
    songActorBuilder = function()
        local s
        s = Def.ActorFrame {
            InitCommand = function(self)
                s.actor = self
            end,
            LoadFont("Common Normal") .. {
                BeginCommand = function(self)
                    s.actor.fontActor = self
                end
            }
        }
        return s
    end,
    groupActorBuilder = function()
        local g
        g = Def.ActorFrame {
            InitCommand = function(self)
                g.actor = self
            end,
            LoadFont("Common Normal") .. {
                BeginCommand = function(self)
                    g.actor.fontActor = self
                end
            }
        }
        return g
    end,
    songActorUpdater = function(self, song)
        (self.fontActor):settext(song:GetMainTitle())
    end,
    groupActorUpdater = function(self, packName)
        (self.fontActor):settext(packName)
    end,
    highlightBuilder = nil,
    frameTransformer = nil, --function(frame, offsetFromCenter, index, total) -- Handle frame positioning
}

function MusicWheel:new(params)
    local noOverrideFrameBuilder = false
    local noOverrideFrameUpdater = false
    params = params or {}
    if params.frameBuilder ~= nil then
        noOverrideFrameBuilder = true
    end
    if params.frameUpdater ~= nil then
        noOverrideFrameUpdater = true
    end
    fillNilTableFieldsFrom(params, MusicWheel.defaultParams)
    local groupActorBuilder = params.groupActorBuilder
    local songActorBuilder = params.songActorBuilder
    local songActorUpdater = params.songActorUpdater
    local groupActorUpdater = params.groupActorUpdater

    -- reset all WHEELDATA info, set up stats
    WHEELDATA:Init()

    local w
    w = Wheel:new {
        count = params.count,
        buildOnInit = params.buildOnInit,
        frameTransformer = params.frameTransformer,
        x = params.x,
        highlightBuilder = params.highlightBuilder,
        y = params.y,
        frameBuilder = noOverrideFrameBuilder and params.frameBuilder or function()
            local x
            x = Def.ActorFrame {
                InitCommand = function(self)
                    x.actor = self
                end,
                groupActorBuilder() .. {
                    BeginCommand = function(self)
                        x.actor.g = self
                    end
                },
                songActorBuilder() .. {
                    BeginCommand = function(self)
                        x.actor.s = self
                    end
                }
            }
            return x
        end,
        frameUpdater = noOverrideFrameUpdater and params.frameUpdater or function(frame, songOrPack)
            if songOrPack.GetAllSteps then -- song
                -- Update songActor and make group actor invis
                local s = frame.s
                s:visible(true)
                local g = (frame.g)
                g:visible(false)
                songActorUpdater(s, songOrPack)
            else
                --update group actor and make song actor invis
                local s = frame.s
                s:visible(false)
                local g = (frame.g)
                g:visible(true)
                groupActorUpdater(g, songOrPack)
            end
        end,
        onSelection = function(frame, songOrPack)
            if songOrPack.GetAllSteps then
                -- STARTING SONG
                crossedGroupBorder = true
                w:updateGlobalsFromCurrentItem()

                SCREENMAN:GetTopScreen():SelectCurrent()
                SCREENMAN:set_input_redirected(PLAYER_1, false)
                MESSAGEMAN:Broadcast("SelectedSong")
            else
                local group = songOrPack

                if WHEELDATA:GetCurrentSort() == 0 then
                    -- IN SORT MODE MENU
                    -- PICKING SORT
                    -- group is the name of the sortmode
                    
                    WHEELDATA:SetCurrentSort(group)
                    WHEELDATA:UpdateFilteredSonglist()
        
                    local newItems = WHEELDATA:GetFilteredFolders()
                    WHEELDATA:SetWheelItems(newItems)

                    w:setNewState(
                        1,
                        1,
                        function() return WHEELDATA:GetWheelItems() end,
                        newItems,
                        nil
                    )
                    crossedGroupBorder = true
                    forceGroupCheck = true
                    GAMESTATE:SetCurrentSong(nil)
                    GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
                    
                    MESSAGEMAN:Broadcast("ClosedGroup", {
                        group = w.group,
                    })
                    w:rebuildFrames()
                    MESSAGEMAN:Broadcast("ModifiedGroups", {
                        group = w.group,
                        index = w.index,
                        maxIndex = #w.items,
                    })
                    w:updateGlobalsFromCurrentItem()
                    w:updateMusicFromCurrentItem()
                    MESSAGEMAN:Broadcast("WheelSettled", {
                        song = GAMESTATE:GetCurrentSong(),
                        group = w.group,
                        hovered = w:getCurrentItem(),
                        steps = GAMESTATE:GetCurrentSteps(),
                        index = w.index,
                        maxIndex = #w.items,
                    })
                    w.settled = true
                    return
                end

                if w.group and w.group == group then
                    -- CLOSING PACK
                    crossedGroupBorder = false
                    w.group = nil

                    local newItems = WHEELDATA:GetFilteredFolders()
                    WHEELDATA:SetWheelItems(newItems)

                    w.index = findKeyOf(newItems, group)
                    w.itemsGetter = function() return WHEELDATA:GetWheelItems() end

                    MESSAGEMAN:Broadcast("ClosedGroup", {
                        group = group,
                    })
                else
                    -- OPENING PACK
                    crossedGroupBorder = false
                    w.group = group

                    local newItems = WHEELDATA:GetWheelItemsForOpenedFolder(group)
                    WHEELDATA:SetWheelItems(newItems)
                    
                    w.index = findKeyOf(newItems, group)
                    w.itemsGetter = function() return WHEELDATA:GetWheelItems() end

                    crossedGroupBorder = true
                    MESSAGEMAN:Broadcast("OpenedGroup", {
                        group = group,
                    })
                end
                w:rebuildFrames()
                MESSAGEMAN:Broadcast("ModifiedGroups", {
                    group = w.group,
                    index = w.index,
                    maxIndex = #w.items,
                })
            end
        end,
        itemsGetter = function()
            return WHEELDATA:GetWheelItems()
        end
    }

    -- external access to move the wheel in a direction
    -- give either a percentage (musicwheel scrollbar movement) or a distance from current position
    -- params.percent or params.direction
    w.MoveCommand = function(self, params)
        if params and params.direction and tonumber(params.direction) then
            w:move(params.direction)
            w:move(0)
        elseif params.percent and tonumber(params.percent) >= 0 then
            local now = w.index
            local max = #w.items
            local indexFromPercent = clamp(math.floor(params.percent * max), 0, max)
            local distanceToMove = indexFromPercent - now
            w:move(distanceToMove)
            w:move(0)
        end
    end

    -- external access command for SelectCurrent with a condition
    w.OpenIfGroupCommand = function(self)
        local i = w:getCurrentItem()
        if i.GetDisplayMainTitle == nil then
            w.onSelection(w:getCurrentFrame(), w:getCurrentItem())
        end
    end

    -- grant external access to the selection function
    w.SelectCurrentCommand = function(self)
        w.onSelection(w:getCurrentFrame(), w:getCurrentItem())
    end

    -- trigger a rebuild on F9 presses in case any specific text uses transliteration
    w.DisplayLanguageChangedMessageCommand = function(self)
        w:rebuildFrames()
    end

    -- building the wheel with startOnPreferred causes init to start on the chart stored in Gamestate
    if params.startOnPreferred then
        w.OnCommand = function(self)
            local group = w:findSong()
            if #w.frames > 0 and group ~= nil then
                -- found the song, set up the group focus and send out the related messages for consistency
                crossedGroupBorder = true
                forceGroupCheck = true
                MESSAGEMAN:Broadcast("OpenedGroup", {
                    group = group,
                })
                w:rebuildFrames()
                MESSAGEMAN:Broadcast("ModifiedGroups", {
                    group = group,
                    index = w.index,
                    maxIndex = #w.items,
                })
            else
                -- if the song was not found or there are no items to refresh, do nothing
                w:rebuildFrames()
            end
        end
    end

    w.FindSongCommand = function(self, params)
        if params.chartkey ~= nil then
            local group = w:findSong(params.chartkey)
            if group ~= nil then
                -- found the song, set up the group focus and send out the related messages for consistency
                crossedGroupBorder = true
                forceGroupCheck = true
                MESSAGEMAN:Broadcast("OpenedGroup", {
                    group = group,
                })
                w:rebuildFrames()
                MESSAGEMAN:Broadcast("ModifiedGroups", {
                    group = group,
                    index = w.index,
                    maxIndex = #w.items,
                })
                w:move(0)
                w:updateGlobalsFromCurrentItem()
                w:updateMusicFromCurrentItem()
                MESSAGEMAN:Broadcast("WheelSettled", {
                    song = GAMESTATE:GetCurrentSong(),
                    group = w.group,
                    hovered = w:getCurrentItem(),
                    steps = GAMESTATE:GetCurrentSteps(),
                    index = w.index,
                    maxIndex = #w.items,
                })
                w.settled = true
            end
        elseif params.song ~= nil then
            local charts = WHEELDATA:GetChartsMatchingFilter(params.song)
            if #charts > 0 then
                local group = w:findSong(charts[1]:GetChartKey())
                if group ~= nil then
                    -- found the song, set up the group focus and send out the related messages for consistency
                    crossedGroupBorder = true
                    forceGroupCheck = true
                    MESSAGEMAN:Broadcast("OpenedGroup", {
                        group = group,
                    })
                    w:rebuildFrames()
                    MESSAGEMAN:Broadcast("ModifiedGroups", {
                        group = group,
                        index = w.index,
                        maxIndex = #w.items,
                    })
                    w:updateGlobalsFromCurrentItem()
                    w:updateMusicFromCurrentItem()
                    MESSAGEMAN:Broadcast("WheelSettled", {
                        song = GAMESTATE:GetCurrentSong(),
                        group = w.group,
                        hovered = w:getCurrentItem(),
                        steps = GAMESTATE:GetCurrentSteps(),
                        index = w.index,
                        maxIndex = #w.items,
                    })
                    w.settled = true
                end
            end
        end
    end

    w.FindGroupCommand = function(self, params)
        if params.group ~= nil then
            local success = w:findGroup(params.group, false)
            if success then
                crossedGroupBorder = true
                forceGroupCheck = true
                MESSAGEMAN:Broadcast("OpenedGroup", {
                    group = group,
                })
                w:rebuildFrames()
                MESSAGEMAN:Broadcast("ModifiedGroups", {
                    group = group,
                    index = w.index,
                    maxIndex = #w.items,
                })
                w:updateGlobalsFromCurrentItem()
                w:updateMusicFromCurrentItem()
                MESSAGEMAN:Broadcast("WheelSettled", {
                    song = GAMESTATE:GetCurrentSong(),
                    group = w.group,
                    hovered = w:getCurrentItem(),
                    steps = GAMESTATE:GetCurrentSteps(),
                    index = w.index,
                    maxIndex = #w.items,
                })
                w.settled = true
            else
                -- in this case there was something wrong with the input
                -- usually it always is "successful" but gives an index of 1 if nothing is actually found
            end
        end
    end

    w.UpdateFiltersCommand = function(self)
        -- reset wheel position to 1 (todo: dont)
        -- refresh filters
        WHEELDATA:UpdateFilteredSonglist()
        
        local newItems = WHEELDATA:GetFilteredFolders()
        WHEELDATA:SetWheelItems(newItems)

        w:setNewState(
            1,
            1,
            function() return WHEELDATA:GetWheelItems() end,
            newItems,
            nil
        )
        crossedGroupBorder = true
        forceGroupCheck = true
        GAMESTATE:SetCurrentSong(nil)
        GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
        
        MESSAGEMAN:Broadcast("ClosedGroup", {
            group = w.group,
        })
        w:rebuildFrames()
        MESSAGEMAN:Broadcast("ModifiedGroups", {
            group = w.group,
            index = w.index,
            maxIndex = #w.items,
        })
        w:updateGlobalsFromCurrentItem()
        w:updateMusicFromCurrentItem()
        MESSAGEMAN:Broadcast("WheelSettled", {
            song = GAMESTATE:GetCurrentSong(),
            group = w.group,
            hovered = w:getCurrentItem(),
            steps = GAMESTATE:GetCurrentSteps(),
            index = w.index,
            maxIndex = #w.items
        })
        w.settled = true
    end

    w.OpenSortModeMenuCommand = function(self)
        w:openSortModeMenu()
    end

    return w
end
