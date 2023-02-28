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
local enteringSong = false

-- sounds (actors)
local SOUND_MOVE = nil
local SOUND_SELECT = nil

-- timer stuff
local last_wheel_move_timestamp = GetTimeSinceStart()
local spinSpeed = function()
    if themeConfig and getWheelSpeed then
        local sp = getWheelSpeed()
        if sp > 0 then
            return sp
        else
            return 15
        end
    else
        return 15
    end
end

Wheel.mt = {
    updateMusicFromCurrentItem = function(whee)
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

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
    updateGlobalsFromCurrentItem = function(whee, dontUpdateSteps)
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

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

            -- only update steps if we want to
            if not dontUpdateSteps then
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
            end
        else
            -- currentItem is a GROUP
            GAMESTATE:SetCurrentSong(nil)
            GAMESTATE:SetCurrentSteps(PLAYER_1, nil)
        end
    end,
    move = function(whee, num)
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end
        
        if num == whee.moving then return end

        if whee.moving ~= 0 and num == 0 and whee.timeBeforeMovingBegins == 0 then
            if math.abs(whee.positionOffsetFromSelection) < 0.25 then
                whee:changemusic(whee.moving)
            end
        end

        whee.timeBeforeMovingBegins = 1 / 8 -- this was hardcoded and there is no justification for it
        whee.spinSpeed = spinSpeed() -- preference based (CHANGE THIS)
        whee.moving = num

        if whee.moving ~= 0 then
            whee:changemusic(whee.moving)
        else
            if SOUND_MOVE ~= nil then
                SOUND_MOVE:play()
            end
        end

        -- stop the music if moving so we dont leave it playing in a random place
        SOUND:StopMusic()
    end,
    changemusic = function(whee, num)
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end
        
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
    findSong = function(whee, chartkey, group) -- returns the group that opened while finding this song
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

        local song = nil
        local steps = nil

        -- either use profile/preference based song or locate by chartkey
        if chartkey == nil then
            song = GAMESTATE:GetPreferredSong()
        else
            -- if group is given, attempt to force the found song to be in that group
            if group ~= nil then
                local songs = WHEELDATA:GetSongsInFolder(group)
                if songs == nil or #songs == 0 then return nil end
                local function chartByChartkey(song)
                    for _, c in ipairs(song:GetAllSteps()) do
                        if c:GetChartKey() == chartkey then return c end
                    end
                    return nil
                end
                for _,s in ipairs(songs) do
                    if chartByChartkey(s) ~= nil then
                        song = s
                        break
                    end
                end
            end
            -- if no group is given or song is never found in the group, this will always work
            if song == nil then
                song = SONGMAN:GetSongByChartKey(chartkey)
            end
        end

        -- a song must pass the filter if being warped to
        if song ~= nil and not WHEELDATA:FilterCheck(song) then
            return nil
        end

        -- jump to the first instance of the song if it exists
        if song ~= nil then
            local newItems, songgroup, finalIndex = WHEELDATA:GetWheelItemsAndGroupAndIndexForSong(song, group)

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
    findSongHelper = function(w, params, bIsShiftAllowed)
        bIsShiftAllowed = bIsShiftAllowed or false -- nil replacement

        if bIsShiftAllowed and INPUTFILTER:IsShiftPressed() and w.lastlastrandomkey ~= nil then
            params.chartkey = w.lastlastrandomkey
            params.group = w.lastlastgroup
            params.song = nil
        end

        -- internalized function which takes a parameter table, usually from a Command
        if params.chartkey ~= nil then
            local group = w:findSong(params.chartkey, params.group)
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
                -- dont update the steps in updateGlobalsFromCurrentItem
                -- the steps was set in w:findSong
                w:updateGlobalsFromCurrentItem(true)
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

                if bIsShiftAllowed and not INPUTFILTER:IsShiftPressed() then
                    w.lastlastgroup = w.lastgroup
                    w.lastlastrandomkey = w.lastrandomkey
                    w.lastrandomkey = params.chartkey
                    w.lastgroup = w.group
                end

                return true
            end
        elseif params.song ~= nil then
            local charts = WHEELDATA:GetChartsMatchingFilter(params.song)
            if #charts > 0 then
                local group = w:findSong(charts[1]:GetChartKey(), params.group)
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

                    if bIsShiftAllowed and not INPUTFILTER:IsShiftPressed() then
                        w.lastlastgroup = w.lastgroup
                        w.lastlastrandomkey = w.lastrandomkey
                        w.lastrandomkey = GAMESTATE:GetCurrentSteps():GetChartKey()
                        w.lastgroup = w.group
                    end

                    return true
                end
            end
        end
        return false
    end,
    findGroup = function(whee, name, openGroup) -- returns success status
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

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
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

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
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

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
        -- if transitioning into a song dont let the wheel do anything
        if enteringSong then return end

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
            whee.frameUpdater(frame, whee:getItem(idx), offset, idx == whee.index)
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
        if whee.positionOffsetFromSelection == 0 and not whee.settled and whee.moving == 0 then
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
    local whee = Def.ActorFrame {
        Name = "Wheel",
        Def.Sound {
            Name = "MoveSound",
            File = THEME:GetPathS("LuaWheel", "move"),
            Precache = true,
            IsAction = true,
            InitCommand = function(self) SOUND_MOVE = self end,
        },
        Def.Sound {
            Name = "SelectSound",
            File = THEME:GetPathS("Common", "value"),
            Precache = true,
            IsAction = true,
            InitCommand = function(self) SOUND_SELECT = self end,
        }
    }
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
        local tscr = SCREENMAN:GetTopScreen()
        local snm = tscr:GetName()
        local anm = self:GetName()
        CONTEXTMAN:RegisterToContextSet(snm, "Main1", anm)
        local heldButtons = {}

        -- timing out the button combo to go to the sort mode menu
        local buttonQueue = {}
        local comboTimeout = nil
        local comboTimeoutSeconds = 1

        local function clearTimeout()
            if comboTimeout ~= nil then
                tscr:clearInterval(comboTimeout)
                comboTimeout = nil
            end
        end

        local function resetTimeout()
            clearTimeout()
            -- we use setInterval because setTimeout doesnt do what is needed
            comboTimeout = tscr:setInterval(function()
                buttonQueue = {}
                clearTimeout()
            end,
            comboTimeoutSeconds)
        end

        tscr:AddInputCallback(
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
                

                -- if transitioning into a song dont let the wheel do anything
                if enteringSong then return end

                -- MASSIVE HACK SECTION
                -- HUGE HACKS RIGHT HERE
                -- For every ScreenSelectMusic shortcut that is defined in c++, because the way things work
                -- if any of those buttons happen to overlap with a GameButton, the c++ input wont be called if it is redirected.
                -- that means some of the functionality will fail.
                -- To cope, mimic that exact behavior right here instead.
                if event.type == "InputEventType_FirstPress" and gameButton ~= "" and event.charNoModifiers ~= nil and SCREENMAN:get_input_redirected(PLAYER_1) then
                    local ctrl = INPUTFILTER:IsControlPressed()
                    local shift = INPUTFILTER:IsShiftPressed()
                    local char = event.charNoModifiers:upper()
                    local ssm = SCREENMAN:GetTopScreen()

                    -- if settled and the current screen is [net]selectmusic
                    local allowedToDoAnyOfThis = whee.settled and ssm ~= nil and ssm.GetMusicWheel ~= nil

                    if allowedToDoAnyOfThis then
                        if ctrl and shift and char == "R" then
                            -- Reload current song from disk (ctrl shift R)
                            ssm:ReloadCurrentSong()
                            return true
                        elseif ctrl and shift and char == "P" then
                            -- Reload current pack from disk (ctrl shift P)
                            ssm:ReloadCurrentPack()
                            return true
                        elseif ctrl and char == "F" then
                            -- Toggle favorite on current chart (ctrl F)
                            ssm:ToggleCurrentFavorite()
                            return true
                        elseif ctrl and char == "M" then
                            -- Toggle permamirror on current chart (ctrl M)
                            ssm:ToggleCurrentPermamirror()
                            return true
                        elseif ctrl and char == "G" then
                            -- Set a new goal on current chart (ctrl G)
                            ssm:GoalFromCurrentChart()
                            return true
                        elseif ctrl and char == "Q" then
                            -- Check songs folder for new songs and load them (ctrl Q)
                            SONGMAN:DifferentialReload()
                            return true
                        elseif ctrl and char == "O" then
                            -- Toggle practice mode (ctrl O)
                            GAMESTATE:SetPracticeMode(not GAMESTATE:IsPracticeMode())
                            ms.ok("Practice Mode "..(GAMESTATE:IsPracticeMode() and "On" or "Off"))
                            return true
                        elseif ctrl and char == "S" then
                            -- Save profile (ctrl S)
                            if PROFILEMAN:SaveProfile() then
                                ms.ok("Profile Saved")
                            else
                                ms.ok("Profile failed to save...")
                            end
                            return true
                        elseif ctrl and char == "P" then
                            -- Make a new playlist (ctrl P)
                            newPlaylistDialogue()
                            return true
                        elseif ctrl and char == "A" then
                            -- Add a new entry to the current playlist (ctrl A)
                            ssm:AddCurrentChartToActivePlaylist()
                            return true
                        elseif ctrl and char == "T" then
                            -- calc test stuff, dont care
                        end
                    end
                end
                -- END MASSIVE HACK SECTION
                -----------

                -- dont allow keyboard input on the wheel while in settings
                if CONTEXTMAN:CheckContextSet(snm, "Settings") then return end
                if CONTEXTMAN:CheckContextSet(snm, "AssetSettings") then return end
                if CONTEXTMAN:CheckContextSet(snm, "Keybindings") then return end
                if CONTEXTMAN:CheckContextSet(snm, "ColorConfig") then return end

                if event.type == "InputEventType_FirstPress" then
                    resetTimeout()
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
                        return true
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
                        if not heldButtons["left"] and not heldButtons["right"] then
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
                        tscr:Cancel()
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
                local MAX_WHEEL_SOUND_SPEED = spinSpeed()

                -- handle wheel movement moving moves
                if whee.moving ~= 0 and whee.timeBeforeMovingBegins == 0 then
                    local sping = whee.spinSpeed * whee.moving * 1.25
                    whee.positionOffsetFromSelection = clamp(whee.positionOffsetFromSelection - (sping * delta), -1, 1)
                    if (whee.moving == -1 and whee.positionOffsetFromSelection >= 0) or
                        (whee.moving == 1 and whee.positionOffsetFromSelection <= 0) then
                            whee:changemusic(whee.moving)

                            if whee.spinSpeed < MAX_WHEEL_SOUND_SPEED then
                                if SOUND_MOVE ~= nil then
                                    SOUND_MOVE:play()
                                end
                            end
                    end
                    -- maybe play moving sound here based on delta

                    local now = GetTimeSinceStart()
                    if whee.spinSpeed >= MAX_WHEEL_SOUND_SPEED and now > (last_wheel_move_timestamp + (1 / MAX_WHEEL_SOUND_SPEED)) then
                        last_wheel_move_timestamp = now
                        if SOUND_MOVE ~= nil then
                            SOUND_MOVE:play()
                        end
                    end

                else
                    -- the wheel should rotate toward selection but isnt "moving"
                    local sping = 1 + (math.abs(whee.positionOffsetFromSelection) / 0.1)
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
        tscr:setTimeout(
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
    enteringSong = false

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

                -- force the wheel to settle, otherwise we would crash
                w.positionOffsetFromSelection = 0
                w:update()

                crossedGroupBorder = true
                -- dont update steps (pass true as param)
                w:updateGlobalsFromCurrentItem(true)

                SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(songOrPack)
                SCREENMAN:GetTopScreen():SelectCurrent()
                SCREENMAN:set_input_redirected(PLAYER_1, false) -- unlock C++ input (the transition locks it in C++)
                MESSAGEMAN:Broadcast("SelectedSong")
                enteringSong = true -- lock wheel movement
                CONTEXTMAN.ContextIgnored = true -- lock all context controlled input
            else
                local group = songOrPack

                if WHEELDATA:GetCurrentSort() == 0 then
                    -- IN SORT MODE MENU
                    -- PICKING SORT
                    -- group is the name of the sortmode
                    group = group:gsub("Sort by ", "")
                    
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

    -- external access to force update the wheel state visually
    w.UpdateWheelCommand = function(self)
        w:update()
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
        if not w:findSongHelper(params, true) and WHEELDATA:FindTheOnlySearchResult() ~= nil then
            -- sometimes the Song returned via searching by first found chartkey can be from a dupe key
            -- and the Song metadata doesnt fit the Filter
            -- in that case we know theres probably a valid result
            -- so given that there is a valid result, set the position to 1
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
    end

    w.FindGroupCommand = function(self, params)
        if params.group ~= nil then
            local success = w:findGroup(params.group, false)
            if success then
                crossedGroupBorder = true
                forceGroupCheck = true
                MESSAGEMAN:Broadcast("OpenedGroup", {
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
            else
                -- in this case there was something wrong with the input
                -- usually it always is "successful" but gives an index of 1 if nothing is actually found
            end
        end
    end

    w.ReloadFilteredSongsCommand = function(self, params)
        -- this stops things like the diff reload from killing the game
        if enteringSong then return end

        local newItems = WHEELDATA:GetFilteredFolders()
        WHEELDATA:SetWheelItems(newItems)

        local success = false
        if params and params.chartkey ~= nil and params.group ~= nil then
            -- a specific chart within a group was given
            -- (this is used for reloading the wheel and remembering the current position)
            success = w:findSongHelper(params)
        elseif params and params.group ~= nil and params.chartkey == nil then
            -- only a group was given. find the group but dont pick any song
            success = w:findGroup(params.group, false)
        end

        -- failure to succeed.
        -- reset wheel to 1 position
        if not success then
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
        else
            w:changemusic(0)
            w:rebuildFrames()
        end
    end

    w.OpenSortModeMenuCommand = function(self)
        w:openSortModeMenu()
    end

    w.SetFrameTransformerCommand = function(self, params)
        if params and params.f then
            w.frameTransformer = params.f
        end
    end

    w.FavoritesUpdatedMessageCommand = function(self)
        w:update()
    end

    w.OptionUpdatedMessageCommand = function(self, params)
        local optionNamesToUpdateOn = {
            ["Video Banners"] = true,
        }
        if params and optionNamesToUpdateOn[params.name] then
            w:update()
        end
    end

    return w
end
