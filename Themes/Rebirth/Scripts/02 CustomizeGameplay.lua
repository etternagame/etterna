local keymode
local usingReverse

MovableValues = {}
allowedCustomization = false

local function loadValuesTable()
    allowedCustomization = playerConfig:get_data().CustomizeGameplay
    usingReverse = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()

    local ratioX = SCREEN_WIDTH / playerConfig:get_data().CurrentWidth
    local ratioY = SCREEN_HEIGHT / playerConfig:get_data().CurrentHeight

    MovableValues.ScreenZoom = playerConfig:get_data().GameplaySizes[keymode].ScreenZoom
    MovableValues.ScreenX = playerConfig:get_data().GameplayXYCoordinates[keymode].ScreenX
    MovableValues.ScreenY = playerConfig:get_data().GameplayXYCoordinates[keymode].ScreenY
    MovableValues.JudgmentX = playerConfig:get_data().GameplayXYCoordinates[keymode].JudgmentX
    MovableValues.JudgmentY = playerConfig:get_data().GameplayXYCoordinates[keymode].JudgmentY
    MovableValues.JudgmentZoom = playerConfig:get_data().GameplaySizes[keymode].JudgmentZoom
    MovableValues.ComboX = playerConfig:get_data().GameplayXYCoordinates[keymode].ComboX
    MovableValues.ComboY = playerConfig:get_data().GameplayXYCoordinates[keymode].ComboY
    MovableValues.ComboZoom = playerConfig:get_data().GameplaySizes[keymode].ComboZoom
    MovableValues.ErrorBarX = playerConfig:get_data().GameplayXYCoordinates[keymode].ErrorBarX
    MovableValues.ErrorBarY = playerConfig:get_data().GameplayXYCoordinates[keymode].ErrorBarY
    MovableValues.ErrorBarWidth = playerConfig:get_data().GameplaySizes[keymode].ErrorBarWidth
    MovableValues.ErrorBarHeight = playerConfig:get_data().GameplaySizes[keymode].ErrorBarHeight
    MovableValues.TargetTrackerX = playerConfig:get_data().GameplayXYCoordinates[keymode].TargetTrackerX
    MovableValues.TargetTrackerY = playerConfig:get_data().GameplayXYCoordinates[keymode].TargetTrackerY
    MovableValues.TargetTrackerZoom = playerConfig:get_data().GameplaySizes[keymode].TargetTrackerZoom
    MovableValues.FullProgressBarX = playerConfig:get_data().GameplayXYCoordinates[keymode].FullProgressBarX
    MovableValues.FullProgressBarY = playerConfig:get_data().GameplayXYCoordinates[keymode].FullProgressBarY
    MovableValues.FullProgressBarWidth = playerConfig:get_data().GameplaySizes[keymode].FullProgressBarWidth
    MovableValues.FullProgressBarHeight = playerConfig:get_data().GameplaySizes[keymode].FullProgressBarHeight
    MovableValues.MiniProgressBarX = playerConfig:get_data().GameplayXYCoordinates[keymode].MiniProgressBarX
    MovableValues.MiniProgressBarY = playerConfig:get_data().GameplayXYCoordinates[keymode].MiniProgressBarY
    MovableValues.DisplayStdDevX = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayStdDevX
    MovableValues.DisplayStdDevY = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayStdDevY
    MovableValues.DisplayStdDevZoom = playerConfig:get_data().GameplaySizes[keymode].DisplayStdDevZoom
    MovableValues.DisplayEWMAX = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayEWMAX
    MovableValues.DisplayEWMAY = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayEWMAY
    MovableValues.DisplayEWMAZoom = playerConfig:get_data().GameplaySizes[keymode].DisplayEWMAZoom
    MovableValues.DisplayPercentX = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayPercentX
    MovableValues.DisplayPercentY = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayPercentY
    MovableValues.DisplayPercentZoom = playerConfig:get_data().GameplaySizes[keymode].DisplayPercentZoom
    MovableValues.DisplayMeanX = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayMeanX
    MovableValues.DisplayMeanY = playerConfig:get_data().GameplayXYCoordinates[keymode].DisplayMeanY
    MovableValues.DisplayMeanZoom = playerConfig:get_data().GameplaySizes[keymode].DisplayMeanZoom
    MovableValues.NoteFieldX = playerConfig:get_data().GameplayXYCoordinates[keymode].NoteFieldX
    MovableValues.NoteFieldY = playerConfig:get_data().GameplayXYCoordinates[keymode].NoteFieldY
    MovableValues.NoteFieldWidth = playerConfig:get_data().GameplaySizes[keymode].NoteFieldWidth
    MovableValues.NoteFieldHeight = playerConfig:get_data().GameplaySizes[keymode].NoteFieldHeight
    MovableValues.NoteFieldSpacing = playerConfig:get_data().GameplaySizes[keymode].NoteFieldSpacing
    MovableValues.JudgeCounterX = playerConfig:get_data().GameplayXYCoordinates[keymode].JudgeCounterX
    MovableValues.JudgeCounterY = playerConfig:get_data().GameplayXYCoordinates[keymode].JudgeCounterY
    MovableValues.JudgeCounterHeight = playerConfig:get_data().GameplaySizes[keymode].JudgeCounterHeight
    MovableValues.JudgeCounterWidth = playerConfig:get_data().GameplaySizes[keymode].JudgeCounterWidth
    MovableValues.JudgeCounterSpacing = playerConfig:get_data().GameplaySizes[keymode].JudgeCounterSpacing
    MovableValues.ReplayButtonsX = playerConfig:get_data().GameplayXYCoordinates[keymode].ReplayButtonsX
    MovableValues.ReplayButtonsY = playerConfig:get_data().GameplayXYCoordinates[keymode].ReplayButtonsY
    MovableValues.ReplayButtonsSpacing = playerConfig:get_data().GameplaySizes[keymode].ReplayButtonsSpacing
    MovableValues.ReplayButtonsZoom = playerConfig:get_data().GameplaySizes[keymode].ReplayButtonsZoom
    MovableValues.NPSGraphX = playerConfig:get_data().GameplayXYCoordinates[keymode].NPSGraphX
    MovableValues.NPSGraphY = playerConfig:get_data().GameplayXYCoordinates[keymode].NPSGraphY
    MovableValues.NPSGraphWidth = playerConfig:get_data().GameplaySizes[keymode].NPSGraphWidth
    MovableValues.NPSGraphHeight = playerConfig:get_data().GameplaySizes[keymode].NPSGraphHeight
    MovableValues.NPSDisplayX = playerConfig:get_data().GameplayXYCoordinates[keymode].NPSDisplayX
    MovableValues.NPSDisplayY = playerConfig:get_data().GameplayXYCoordinates[keymode].NPSDisplayY
    MovableValues.NPSDisplayZoom = playerConfig:get_data().GameplaySizes[keymode].NPSDisplayZoom
    MovableValues.LeaderboardX = playerConfig:get_data().GameplayXYCoordinates[keymode].LeaderboardX
    MovableValues.LeaderboardY = playerConfig:get_data().GameplayXYCoordinates[keymode].LeaderboardY
    MovableValues.LeaderboardSpacing = playerConfig:get_data().GameplaySizes[keymode].LeaderboardSpacing
    MovableValues.LeaderboardWidth = playerConfig:get_data().GameplaySizes[keymode].LeaderboardWidth
    MovableValues.LeaderboardHeight = playerConfig:get_data().GameplaySizes[keymode].LeaderboardHeight
    MovableValues.LifeP1X = playerConfig:get_data().GameplayXYCoordinates[keymode].LifeP1X
    MovableValues.LifeP1Y = playerConfig:get_data().GameplayXYCoordinates[keymode].LifeP1Y
    MovableValues.LifeP1Rotation = playerConfig:get_data().GameplayXYCoordinates[keymode].LifeP1Rotation
    MovableValues.LifeP1Width = playerConfig:get_data().GameplaySizes[keymode].LifeP1Width
    MovableValues.LifeP1Height = playerConfig:get_data().GameplaySizes[keymode].LifeP1Height
    MovableValues.PracticeCDGraphX = playerConfig:get_data().GameplayXYCoordinates[keymode].PracticeCDGraphX
    MovableValues.PracticeCDGraphY = playerConfig:get_data().GameplayXYCoordinates[keymode].PracticeCDGraphY
    MovableValues.PracticeCDGraphHeight = playerConfig:get_data().GameplaySizes[keymode].PracticeCDGraphHeight
    MovableValues.PracticeCDGraphWidth = playerConfig:get_data().GameplaySizes[keymode].PracticeCDGraphWidth
    MovableValues.BPMTextX = playerConfig:get_data().GameplayXYCoordinates[keymode].BPMTextX
    MovableValues.BPMTextY = playerConfig:get_data().GameplayXYCoordinates[keymode].BPMTextY
    MovableValues.BPMTextZoom = playerConfig:get_data().GameplaySizes[keymode].BPMTextZoom
    MovableValues.MusicRateX = playerConfig:get_data().GameplayXYCoordinates[keymode].MusicRateX
    MovableValues.MusicRateY = playerConfig:get_data().GameplayXYCoordinates[keymode].MusicRateY
    MovableValues.MusicRateZoom = playerConfig:get_data().GameplaySizes[keymode].MusicRateZoom
    MovableValues.PlayerInfoX = playerConfig:get_data().GameplayXYCoordinates[keymode].PlayerInfoX
    MovableValues.PlayerInfoY = playerConfig:get_data().GameplayXYCoordinates[keymode].PlayerInfoY
    MovableValues.PlayerInfoZoom = playerConfig:get_data().GameplaySizes[keymode].PlayerInfoZoom
    MovableValues.CoverHeight = playerConfig:get_data().GameplaySizes[keymode].CoverHeight
    MovableValues.MeasureCounterX = playerConfig:get_data().GameplayXYCoordinates[keymode].MeasureCounterX
    MovableValues.MeasureCounterY = playerConfig:get_data().GameplayXYCoordinates[keymode].MeasureCounterY
    MovableValues.MeasureCounterZoom = playerConfig:get_data().GameplaySizes[keymode].MeasureCounterZoom

    -- if the aspect ratio changed at all we need to reset all these numbers
    if ratioX ~= 1 or ratioY ~= 1 then
        for name, val in pairs(MovableValues) do
            if name:find("X$") ~= nil or name:find("Y$") ~= nil or name:find("Rotation$") ~= nil then
                -- coordinates
                playerConfig:get_data().GameplayXYCoordinates[keymode][name] = val
            elseif name:find("Zoom$") ~= nil or name:find("Height$") ~= nil or name:find("Width$") ~= nil or name:find("Spacing$") ~= nil then
                -- sizes
                playerConfig:get_data().GameplaySizes[keymode][name] = val
            end
        end
        playerConfig:get_data().CurrentWidth = SCREEN_WIDTH
        playerConfig:get_data().CurrentHeight = SCREEN_HEIGHT
        playerConfig:save()
    end
end

-- registry for elements which are able to be modified in customizegameplay
local customizeGameplayElements = {}
local storedStateForUndoAction = {}
local selectedElementActor = nil
-- must be passed elementinfo, a table of:
--[[
    {
        actor = (an actorframe),
        coordInc = {big, small}, increment size for coordinates
        zoomInc = {big, small}, increment size for zooming (zoom or size both work here)
        rotationInc = {big, small}, increment size for rotation
        spacingInc = {big, small}, increment size for spacing
    }
]]
function registerActorToCustomizeGameplayUI(elementinfo, layer)
    customizeGameplayElements[#customizeGameplayElements+1] = elementinfo
    local elementFrame = elementinfo.actor

    if allowedCustomization then
        elementFrame:AddChildFromPath(THEME:GetPathG("", "elementborder"))
        if layer ~= nil then
            elementFrame:GetChild("BorderContainer"):RunCommandsRecursively(
                function(self)
                    local cmd = function(shelf)
                        shelf:z(layer)
                    end
                    if self:GetCommand("SetUpFinished") == nil then
                        self:addcommand("SetUpFinished", cmd)
                    else
                        print("Found duplicate SetUpFinished in element "..(self:GetName() or "UNNAMED"))
                    end
                end)
        end
    end
end
function registerActorToCustomizeGameplayUINoElementBorder(elementinfo)
    customizeGameplayElements[#customizeGameplayElements+1] = elementinfo
end

function getCustomizeGameplayElements()
    local o = {}
    for i,t in ipairs(customizeGameplayElements) do
        o[#o+1] = t.actor
    end
    return o
end

-- returns the big increment and the small increment
function getInfoForSelectedGameplayElement()
    if selectedElementActor == nil then return nil, nil end
    for i,t in ipairs(customizeGameplayElements) do
        if t.actor:GetName() == selectedElementActor:GetName() then
            return t
        end
    end
end

-- convenience to make it look like we arent copy pasting everywhere
-- and also return defaults for the ultra lazy
function getCoordinc(info, big, reverse)
    local o = 0
    if info.coordInc == nil then
        o = (big and 5 or 1)
    else
        o = (big and info.coordInc[1] or info.coordInc[2])
    end
    return o * (reverse and -1 or 1)
end
function getRotationinc(info, big, reverse)
    local o = 0
    if info.rotationInc == nil then
        o = (big and 5 or 1)
    else
        o = (big and info.rotationInc[1] or info.rotationInc[2])
    end
    return o * (reverse and -1 or 1)
end
function getZoominc(info, big, reverse)
    local o = 0
    -- oops lol
    if info.zoomInc == nil and info.sizeInc ~= nil then info.zoomInc = info.sizeInc end
    if info.zoomInc == nil then
        o = (big and 0.5 or 0.1)
    else
        o = (big and info.zoomInc[1] or info.zoomInc[2])
    end
    return o * (reverse and -1 or 1)
end
function getSpacinginc(info, big, reverse)
    local o = 0
    if info.spacingInc == nil then
        o = (big and 5 or 1)
    else
        o = (big and info.spacingInc[1] or info.spacingInc[2])
    end
    return o * (reverse and -1 or 1)
end

function getCoordinatesForElementName(name)
    local xv = playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "X"]
    local yv = playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "Y"]
    local rotZv = playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "Rotation"]

    return {
        x = xv,
        y = yv,
        rotation = rotZv,
    }
end

function getSizesForElementName(name)
    local zoom = playerConfig:get_data().GameplaySizes[keymode][name .. "Zoom"]
    local width = playerConfig:get_data().GameplaySizes[keymode][name .. "Width"]
    local height = playerConfig:get_data().GameplaySizes[keymode][name .. "Height"]
    local spacing = playerConfig:get_data().GameplaySizes[keymode][name .. "Spacing"]

    return {
        zoom = zoom,
        width = width,
        height = height,
        spacing = spacing,
    }
end

function elementHasAnyMovableCoordinates(name)
    return playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "X"] ~= nil or playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "Y"]
end

-- store the current state of the element for an undo action later
-- if necessary
function setStoredStateForUndoAction(name)
    storedStateForUndoAction.coords = getCoordinatesForElementName(name)
    storedStateForUndoAction.sizes = getSizesForElementName(name)
    storedStateForUndoAction.name = name
    storedStateForUndoAction.actor = selectedElementActor
end

function getStoredStateForUndoAction()
    return storedStateForUndoAction
end

-- execute an undo action
function resetElementUsingStoredState()
    local coord = storedStateForUndoAction.coords
    local size = storedStateForUndoAction.sizes
    if storedStateForUndoAction.name == nil or storedStateForUndoAction.actor == nil then
        return
    end
    local name = storedStateForUndoAction.name
    local actor = storedStateForUndoAction.actor

    if coord ~= nil then
        if coord.x ~= nil then
            local tname = name .. "X"
            local v = coord.x
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = v
            MovableValues[tname] = v
        end
        if coord.y ~= nil then
            local tname = name .. "Y"
            local v = coord.y
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = v
            MovableValues[tname] = v
        end
        if coord.rotation ~= nil then
            local tname = name .. "Rotation"
            local v = coord.rotation
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = v
            MovableValues[tname] = v
        end
    end
    if size ~= nil then
        if size.zoom ~= nil then
            local tname = name .. "Zoom"
            local v = size.zoom
            playerConfig:get_data().GameplaySizes[keymode][tname] = v
            MovableValues[tname] = v
        end
        if size.width ~= nil then
            local tname = name .. "Width"
            local v = size.width
            playerConfig:get_data().GameplaySizes[keymode][tname] = v
            MovableValues[tname] = v
        end
        if size.height ~= nil then
            local tname = name .. "Height"
            local v = size.height
            playerConfig:get_data().GameplaySizes[keymode][tname] = v
            MovableValues[tname] = v
        end
        if size.spacing ~= nil then
            local tname = name .. "Spacing"
            local v = size.spacing
            playerConfig:get_data().GameplaySizes[keymode][tname] = v
            MovableValues[tname] = v
        end
    end
    -- tell everything to update MovableValues (lazy)
    MESSAGEMAN:Broadcast("SetUpMovableValues")

    playerConfig:set_dirty()
    -- alert ui of update
    MESSAGEMAN:Broadcast("CustomizeGameplayElementUndo", {name=name})
end

-- reset an element to its default state
-- this leverages the storedStateForUndoAction
-- as such, you need to setStoredStateForUndoAction first
-- which basically means you utilized setSelectedCustomizeGameplayElementActorByName
-- (an element must be first selected to be reset to default.)
function resetElementToDefault()
    local coord = storedStateForUndoAction.coords
    local size = storedStateForUndoAction.sizes
    if storedStateForUndoAction.name == nil or storedStateForUndoAction.actor == nil then
        return
    end
    local name = storedStateForUndoAction.name
    local actor = storedStateForUndoAction.actor

    if coord ~= nil then
        if coord.x ~= nil then
            local tname = name .. "X"
            local default = getDefaultGameplayCoordinate(tname)
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = default
            MovableValues[tname] = default
        end
        if coord.y ~= nil then
            local tname = name .. "Y"
            local default = getDefaultGameplayCoordinate(tname)
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = default
            MovableValues[tname] = default
        end
        if coord.rotation ~= nil then
            local tname = name .. "Rotation"
            local default = getDefaultGameplayCoordinate(tname)
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = default
            MovableValues[tname] = default
        end
    end
    if size ~= nil then
        if size.zoom ~= nil then
            local tname = name .. "Zoom"
            local default = getDefaultGameplaySize(tname)
            playerConfig:get_data().GameplaySizes[keymode][tname] = default
            MovableValues[tname] = default
        end
        if size.width ~= nil then
            local tname = name .. "Width"
            local default = getDefaultGameplaySize(tname)
            playerConfig:get_data().GameplaySizes[keymode][tname] = default
            MovableValues[tname] = default
        end
        if size.height ~= nil then
            local tname = name .. "Height"
            local default = getDefaultGameplaySize(tname)
            playerConfig:get_data().GameplaySizes[keymode][tname] = default
            MovableValues[tname] = default
        end
        if size.spacing ~= nil then
            local tname = name .. "Spacing"
            local default = getDefaultGameplaySize(tname)
            playerConfig:get_data().GameplaySizes[keymode][tname] = default
            MovableValues[tname] = default
        end
    end
    -- tell everything to update MovableValues (lazy)
    MESSAGEMAN:Broadcast("SetUpMovableValues")

    playerConfig:set_dirty()
    -- alert ui of update
    MESSAGEMAN:Broadcast("CustomizeGameplayElementDefaulted", {name=name})
end

function setSelectedCustomizeGameplayElementActorByName(elementName)
    local index = 0
    local elementActor = nil
    for i, e in ipairs(customizeGameplayElements) do
        if e.actor:GetName() == elementName then
            index = i
            elementActor = e.actor
            break
        end
    end

    -- element found, set up things
    if elementActor ~= nil then
        selectedElementActor = elementActor
        MESSAGEMAN:Broadcast("CustomizeGameplayElementSelected", {name=elementName})
    end
    return elementHasAnyMovableCoordinates(elementName)
end

function setSelectedCustomizeGameplayElementActor(actor, name)
    selectedElementActor = actor
    MESSAGEMAN:Broadcast("CustomizeGameplayElementSelected", {name=name})
    return elementHasAnyMovableCoordinates(name)
end

function getSelectedCustomizeGameplayMovableActor()
    return selectedElementActor
end

-- set the new XY coordinates of an element using the DIFFERENCE from before it was changed and the new value
-- mostly just meant to be used with the mouse dragging functionality
function setSelectedCustomizeGameplayElementActorPosition(differenceX, differenceY)
    if selectedElementActor ~= nil then
        local name = selectedElementActor:GetName()
        local xv = playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "X"]
        local yv = playerConfig:get_data().GameplayXYCoordinates[keymode][name .. "Y"]

        if xv ~= nil then
            local tname = name .. "X"
            local v = xv + differenceX
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = v
            MovableValues[tname] = v
        end
        if yv ~= nil then
            local tname = name .. "Y"
            local v = yv + differenceY
            playerConfig:get_data().GameplayXYCoordinates[keymode][tname] = v
            MovableValues[tname] = v
        end
        playerConfig:set_dirty()
        MESSAGEMAN:Broadcast("CustomizeGameplayElementMoved", {name=name})
    end
end

local function updateCustomizeGameplayTables(tname, increment, tableName)
    if selectedElementActor ~= nil then
        local beforeVal = playerConfig:get_data()[tableName][keymode][tname]
        if beforeVal ~= nil then
            local afterVal = beforeVal + increment
            if tname:find("Rotation") then
                afterVal = afterVal % 360
            end
            playerConfig:get_data()[tableName][keymode][tname] = afterVal
            MovableValues[tname] = afterVal
            playerConfig:set_dirty()
            -- tell everything to update MovableValues (lazy)
            MESSAGEMAN:Broadcast("SetUpMovableValues")
            MESSAGEMAN:Broadcast("CustomizeGameplayElementMoved", {name=selectedElementActor:GetName()})
        end
    end
end

-- set any GameplayXYCoordinates value using an increment of the existing value
function updateGameplayCoordinate(tname, increment)
    updateCustomizeGameplayTables(tname, increment, "GameplayXYCoordinates")
end

-- set any GameplaySizes value using an increment of the existing value
function updateGameplaySize(tname, increment)
    updateCustomizeGameplayTables(tname, increment, "GameplaySizes")
end

function unsetMovableKeymode()
    MovableValues = {}
    customizeGameplayElements = {}
end

-- required to be called once for most things to work at all
-- normally called right at gameplay init as early as possible
function setMovableKeymode(key)
    keymode = key
    customizeGameplayElements = {}
    loadValuesTable()
end

