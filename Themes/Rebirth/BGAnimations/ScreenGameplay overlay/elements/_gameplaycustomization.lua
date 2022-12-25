-- gameplay customization
-- assume that customization is active if this file is loaded

local snm = Var("LoadingScreen")

local ratios = {
    MenuHeight = 500 / 1080,
    MenuWidth = 600 / 1920,
    MenuDraggerHeight = 75 / 1080,
    EdgePadding = 10 / 1920,
}
local actuals = {
    MenuHeight = ratios.MenuHeight * SCREEN_HEIGHT,
    MenuWidth = ratios.MenuWidth * SCREEN_WIDTH,
    MenuDraggerHeight = ratios.MenuDraggerHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
}

local translations = {
    CustomizeGameplayInstruction = THEME:GetString("ScreenGameplay", "CustomizeGameplayInstruction"),
    Spacing = THEME:GetString("ScreenGameplay", "Spacing"),
    Height = THEME:GetString("ScreenGameplay", "Height"),
    Width = THEME:GetString("ScreenGameplay", "Width"),
    Zoom = THEME:GetString("ScreenGameplay", "Zoom"),
    Rotation = THEME:GetString("ScreenGameplay", "Rotation"),
    BPMText = THEME:GetString("ScreenGameplay", "BPMText"),
    Combo = THEME:GetString("ScreenGameplay", "Combo"),
    Cover = THEME:GetString("ScreenGameplay", "Cover"),
    DisplayMean = THEME:GetString("ScreenGameplay", "DisplayMean"),
    DisplayPercent = THEME:GetString("ScreenGameplay", "DisplayPercent"),
    DisplayEWMA = THEME:GetString("ScreenGameplay", "DisplayEWMA"),
    DisplayStdDev = THEME:GetString("ScreenGameplay", "DisplayStdDev"),
    ErrorBar = THEME:GetString("ScreenGameplay", "ErrorBar"),
    FullProgressBar = THEME:GetString("ScreenGameplay", "FullProgressBar"),
    JudgeCounter = THEME:GetString("ScreenGameplay", "JudgeCounter"),
    Judgment = THEME:GetString("ScreenGameplay", "Judgment"),
    LifeP1 = THEME:GetString("ScreenGameplay", "LifeP1"),
    MusicRate = THEME:GetString("ScreenGameplay", "MusicRate"),
    NoteField = THEME:GetString("ScreenGameplay", "NoteField"),
    NPSDisplay = THEME:GetString("ScreenGameplay", "NPSDisplay"),
    NPSGraph = THEME:GetString("ScreenGameplay", "NPSGraph"),
    PlayerInfo = THEME:GetString("ScreenGameplay", "PlayerInfo"),
    Screen = THEME:GetString("ScreenGameplay", "Screen"),
    TargetTracker = THEME:GetString("ScreenGameplay", "TargetTracker"),
}

local uiBGAlpha = 0.6
local textButtonHeightFudgeScalarMultiplier = 1.4
local buttonHoverAlpha = 0.6
local cursorAlpha = 0.5
local cursorAnimationSeconds = 0.05

local elementNameTextSize = 1
local elementCoordTextSize = 1
local elementSizeTextSize = 1
local elementListTextSize = 1
local uiInstructionTextSize = 0.6

local t = Def.ActorFrame {
    Name = "GameplayElementsCustomizer",
    InitCommand = function(self)
        -- in the off chance we end up in customization when in syncmachine, dont turn on autoplay
        if snm ~= "ScreenGameplaySyncMachine" then
            GAMESTATE:SetAutoplay(true)
        else
            GAMESTATE:SetAutoplay(false)
        end
    end,
    BeginCommand = function(self)
        local screen = SCREENMAN:GetTopScreen()

        local lifebar = screen:GetLifeMeter(PLAYER_1)
        local nf = screen:GetChild("PlayerP1"):GetChild("NoteField")
        local noteColumns = nf:get_column_actors()

        registerActorToCustomizeGameplayUI({
            actor = lifebar,
            coordInc = {5,1},
            rotationInc = {5,1},
            sizeInc = {0.1, 0.05},
        })
        registerActorToCustomizeGameplayUI({
            actor = nf,
            coordInc = {5,1},
            sizeInc = {0.1, 0.05},
            spacingInc = {5,1},
        }, 4)
    end,
    EndCommand = function(self)
        -- exiting customize gameplay will turn off autoplay
        GAMESTATE:SetAutoplay(false)
    end,
}

local function makeUI()
    local itemsPerPage = 8
    local itemListFrame = nil

    -- init moved to OnCommand due to timing reasons
    local elements = {}
    local page = 1
    local maxPage = 1

    local cursorPos = 1
    local selectedElement = nil
    local selectedElementCoords = {}
    local selectedElementSizes = {}
    -- valid choices: "Coordinate", "Zoom" (zoomx), "Size" (zoomto), "Spacing", "Rotation"
    local selectedElementMovementType = ""

    local allowedSpace = actuals.MenuHeight - actuals.MenuDraggerHeight - (actuals.EdgePadding*2)
    local topItemY = -actuals.MenuHeight/2 + actuals.MenuDraggerHeight + actuals.EdgePadding

    -- get the next element movement type
    -- provide curr to set a starting point to search
    -- will run recursively until it finds a working match
    local function getNextElementMovementType(curr, recurses)
        if selectedElementMovementType == nil then setSelectedElementMovementType() return selectedElementMovementType end
        if curr == nil then curr = selectedElementMovementType end
        if recurses == nil then recurses = 1 end
        if recurses > 10 then return nil end
        if curr == "Coordinate" then
            if selectedElementCoords ~= nil and selectedElementCoords.rotation ~= nil then
                return "Rotation"
            end
            return getNextElementMovementType("Rotation", recurses + 1)
        end
        if curr == "Rotation" then
            if selectedElementSizes ~= nil and selectedElementSizes.zoom ~= nil then
                return "Zoom"
            end
            return getNextElementMovementType("Zoom", recurses + 1)
        end
        if curr == "Zoom" then
            if selectedElementSizes ~= nil and selectedElementSizes.width ~= nil or selectedElementSizes.height ~= nil then
                return "Size"
            end
            return getNextElementMovementType("Size", recurses + 1)
        end
        if curr == "Size" then
            if selectedElementSizes ~= nil and selectedElementSizes.spacing ~= nil then
                return "Spacing"
            end
            return getNextElementMovementType("Spacing", recurses + 1)
        end
        if curr == "Spacing" then
            if selectedElementCoords ~= nil and selectedElementCoords.x ~= nil or selectedElementCoords.y ~= nil then
                return "Coordinate"
            end
            return getNextElementMovementType("Coordinate", recurses + 1)
        end
    end

    -- set the selected element movement type
    -- if no param is given, set the default (first available)
    -- if param is given, find the next one available (i know, unintuitive, dont care)
    local function setSelectedElementMovementType(movementType)
        if movementType == nil then
            if selectedElementCoords ~= nil then
                if selectedElementCoords.x ~= nil or selectedElementCoords.y ~= nil then
                    selectedElementMovementType = "Coordinate"
                    return
                end
                if selectedElementCoords.rotation ~= nil then
                    selectedElementMovementType = "Rotation"
                    return
                end
            end
            if selectedElementSizes ~= nil then
                if selectedElementSizes.zoom ~= nil then
                    selectedElementMovementType = "Zoom"
                    return
                end
                if selectedElementSizes.width ~= nil or selectedElementSizes.height ~= nil then
                    selectedElementMovementType = "Size"
                    return
                end
                if selectedElementSizes.spacing ~= nil then
                    selectedElementMovementType = "Spacing"
                    return
                end
            end
            -- impossible??
            ms.ok("Selected element movement type could not be determined. Report to developer")
        else
            selectedElementMovementType = getNextElementMovementType(movementType)
        end
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

        local currentPageLowerBound = (page-1) * itemsPerPage + 1
        local currentPageUpperBound = page * itemsPerPage
        if cursorPos < currentPageLowerBound then
            cursorPos = currentPageLowerBound
            if itemListFrame ~= nil then
                itemListFrame:queuecommand("UpdateCursor")
            end
        elseif cursorPos > currentPageUpperBound then
            cursorPos = currentPageUpperBound
            if itemListFrame ~= nil then
                itemListFrame:queuecommand("UpdateCursor")
            end
        end

        if itemListFrame ~= nil then
            itemListFrame:playcommand("UpdateItemList")
        end
    end

    local function moveCursor(n)
        cursorPos = cursorPos + n
        if cursorPos > #elements then
            cursorPos = 1
            page = 1
            if maxPage ~= 1 and itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        elseif cursorPos < 1 then
            cursorPos = #elements
            page = maxPage
            if maxPage ~= 1 and itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        end

        local currentPageLowerBound = (page-1) * itemsPerPage + 1
        local currentPageUpperBound = page * itemsPerPage
        if cursorPos < currentPageLowerBound then
            -- lazy: move only 1 page but its possible to move many pages
            page = page-1
            if itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        elseif cursorPos > currentPageUpperBound then
            -- lazy: move only 1 page but its possible to move many pages
            page = page+1
            if itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        end

        if itemListFrame ~= nil then
            itemListFrame:playcommand("UpdateCursor")
        end
    end

    local function visibilityBySelectedElement(self, reverse)
        if reverse then
            self:visible(selectedElement ~= nil)
        else
            self:visible(selectedElement == nil)
        end
    end

    local function updateSelectedElementValues()
        if selectedElement == nil then return end
        selectedElementCoords = getCoordinatesForElementName(selectedElement)
        selectedElementSizes = getSizesForElementName(selectedElement)
    end

    local function selectCurrent()
        selectedElement = elements[cursorPos]:GetName()
        updateSelectedElementValues()
        setSelectedElementMovementType()
        if selectedElement == "Screen" then
            setSelectedCustomizeGameplayElementActor(SCREENMAN:GetTopScreen(), "Screen")
        else
            setSelectedCustomizeGameplayElementActorByName(selectedElement)
        end
        setStoredStateForUndoAction(selectedElement)
        itemListFrame:playcommand("UpdateItemList")
    end

    local function item(i)
        local index = i
        local element = elements[index]
        return UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "Item_"..i,
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                txt:halign(0)
                txt:zoom(elementListTextSize)
                txt:maxwidth((actuals.MenuWidth-(actuals.EdgePadding*2)) / elementListTextSize)
                bg:halign(0)
                self:x(-actuals.MenuWidth + actuals.EdgePadding)
                self:y(topItemY + (allowedSpace / itemsPerPage) * (i-1) + (allowedSpace / itemsPerPage / 2))

                self.alphaDeterminingFunction = function(self)
                    local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                    local visiblemultiplier = self:IsInvisible() and 0 or 1
                    self:diffusealpha(1 * hovermultiplier * visiblemultiplier)
                end
            end,
            SetItemCommand = function(self)
                visibilityBySelectedElement(self)

                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                index = (page - 1) * itemsPerPage + i
                element = elements[index]
                if element ~= nil then
                    self:diffusealpha(1)
                    txt:settext(translations[element:GetName()] or element:GetName())
                    bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                else
                    self:diffusealpha(0)
                end
            end,
            UpdateCursorCommand = function(self)
                if index == cursorPos then
                    local cursor = self:GetParent():GetChild("Cursor")
                    cursor:finishtweening()
                    cursor:smooth(cursorAnimationSeconds)
                    cursor:xy(self:GetX(), self:GetY())
                    local bg = self:GetChild("BG")
                    cursor:zoomto(bg:GetZoomedWidth(), bg:GetZoomedHeight())
                    cursor:diffusealpha(cursorAlpha)
                end
            end,
            RolloverUpdateCommand = function(self, params)
                if self:IsInvisible() then return end
                if selectedElement == nil then
                    cursorPos = index
                    self:playcommand("UpdateCursor")
                end
                self:alphaDeterminingFunction()
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    cursorPos = index
                    selectCurrent()
                    self:playcommand("UpdateCursor")
                    self:alphaDeterminingFunction()
                end
            end,
        }
    end

    local t = Def.ActorFrame {
        Name = "ItemListContainer",
        InitCommand = function(self)
            itemListFrame = self
            -- make container above all other button elements
            -- not guaranteed but this works for now
            self:z(10)
        end,
        EndCommand = function(self)
            -- triggered immediately before screen deletion
            SCREENMAN:set_input_redirected(PLAYER_1, false)
        end,
        OnCommand = function(self)
            -- these are initialized here because most elements either need BeginCommand or InitCommand to run for them to be registered
            -- Order of execution: Init -> Begin -> On
            elements = getCustomizeGameplayElements()
            -- hacky addition of the Screen element, which affects the entire screen zoom and X/Y
            elements[#elements+1] = {
                GetName = function(self) return "Screen" end,
            }
            table.sort(
                elements,
                function(a, b)
                    local aname = translations[a:GetName()] or a:GetName()
                    local bname = translations[b:GetName()] or b:GetName()
                    return aname:lower() < bname:lower()
                end
            )
            maxPage = math.ceil(#elements / itemsPerPage)

            -- lock input events to only lua, no c++
            SCREENMAN:set_input_redirected(PLAYER_1, true)

            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type ~= "InputEventType_Release" then -- allow Repeat and FirstPress
                    local gameButton = event.button
                    local key = event.DeviceInput.button
                    local up = gameButton == "Up" or gameButton == "MenuUp"
                    local down = gameButton == "Down" or gameButton == "MenuDown"
                    local right = gameButton == "MenuRight" or gameButton == "Right"
                    local left = gameButton == "MenuLeft" or gameButton == "Left"
                    local ctrl = INPUTFILTER:IsBeingPressed("left ctrl") or INPUTFILTER:IsBeingPressed("right ctrl")
                    local shift = INPUTFILTER:IsBeingPressed("left shift") or INPUTFILTER:IsBeingPressed("right shift")
                    local space = key == "DeviceButton_space"

                    -- these inputs shouldnt repeat just to prevent being annoying
                    local enter = (gameButton == "Start")
                        and event.type == "InputEventType_FirstPress"
                    local back = (gameButton == "Back")
                        and event.type == "InputEventType_FirstPress"
                    local undo = (key == "DeviceButton_delete" or gameButton == "RestartGameplay" or key == "DeviceButton_backspace")
                        and event.type == "InputEventType_FirstPress"

                    -- handled in a really special way to prevent holding left mouse and right clicking
                    -- stop trying to break things
                    local rightclick = (key == "DeviceButton_right mouse button")
                        and event.type == "InputEventType_FirstPress" and not INPUTFILTER:IsBeingPressed("left mouse button", "Mouse")

                    -- exit
                    if back then
                        -- (why did we make a specific function for this instead of :Cancel() ?)
                        SCREENMAN:GetTopScreen():begin_backing_out()
                        return true
                    end

                    if selectedElement ~= nil then
                        if up or down or left or right then
                            local info = getInfoForSelectedGameplayElement()
                            -- mega hack (sign of bad design)
                            -- if the screen is selected, fake an info object
                            if info == nil then
                                info = {
                                    coordInc = {5,1},
                                    zoomInc = {0.05,0.01},
                                }
                            end

                            local bigIncrement = true
                            local reverse = false
                            if shift then
                                -- small increment arrow key usage
                                bigIncrement = false
                            else
                                -- regular arrow key usage
                            end

                            if down or left then reverse = true end
                            local tname = selectedElement

                            if selectedElementMovementType == "Coordinate" then
                                if selectedElementCoords ~= nil then
                                    local increment = getCoordinc(info, bigIncrement, reverse)
                                    if left or right then tname = tname .. "X" end
                                    if up or down then tname = tname .. "Y" increment = increment * -1 end
                                    updateGameplayCoordinate(tname, increment)
                                end
                            elseif selectedElementMovementType == "Rotation" then
                                if selectedElementCoords ~= nil then
                                    local increment = getRotationinc(info, bigIncrement, reverse)
                                    tname = tname .. "Rotation"
                                    updateGameplayCoordinate(tname, increment)
                                end
                            elseif selectedElementMovementType == "Zoom" then
                                if selectedElementSizes ~= nil then
                                    local increment = getZoominc(info, bigIncrement, reverse)
                                    tname = tname .. "Zoom"
                                    updateGameplaySize(tname, increment)
                                end
                            elseif selectedElementMovementType == "Size" then
                                if selectedElementSizes ~= nil then
                                    local increment = getZoominc(info, bigIncrement, reverse)
                                    if left or right then tname = tname .. "Width" end
                                    if up or down then tname = tname .. "Height" end
                                    updateGameplaySize(tname, increment)
                                end
                            elseif selectedElementMovementType == "Spacing" then
                                if selectedElementSizes ~= nil then
                                    local increment = getSpacinginc(info, bigIncrement, reverse)
                                    tname = tname .. "Spacing"
                                    updateGameplaySize(tname, increment)
                                end
                            end

                        elseif space then
                            -- go to next element movement type
                            setSelectedElementMovementType(selectedElementMovementType)
                            -- set a checkpoint for undo
                            setStoredStateForUndoAction(selectedElement)
                            self:playcommand("UpdateItemList")
                        elseif undo then
                            if ctrl then
                                -- reset to default
                                resetElementToDefault()
                                setStoredStateForUndoAction(selectedElement)
                            else
                                -- undo changes and return
                                resetElementUsingStoredState()
                                selectedElement = nil
                                self:playcommand("UpdateItemList")
                            end
                        elseif enter or rightclick then
                            -- save position and return
                            -- the reality is all positions are saved always haha
                            -- just go back in this case
                            -- save to disk will occur at screen exit
                            selectedElement = nil
                            self:playcommand("UpdateItemList")
                        end
                    else
                        if up or left then
                            -- up
                            moveCursor(-1)
                        elseif down or right then
                            -- down
                            moveCursor(1)
                        elseif enter then
                            -- select element
                            selectCurrent()
                        end
                    end

                    -- let all mouse inputs through
                    if event.DeviceInput.button:find("mouse") ~= nil then
                        return false
                    end
                    -- eat all other inputs to not let duplicates get through
                    return true
                end

                -- let all mouse inputs through
                if event.DeviceInput.button:find("mouse") ~= nil then
                    return false
                end
                return true
            end)
            self:playcommand("UpdateItemList")
            self:playcommand("UpdateCursor")
            self:finishtweening()
        end,
        UpdateItemListCommand = function(self)
            self:playcommand("SetItem")
        end,
        CustomizeGameplayElementSelectedMessageCommand = function(self, params)
            if params == nil or params.name == nil then return end

            local name = params.name
            selectedElement = name
            updateSelectedElementValues()
            setSelectedElementMovementType()
            setStoredStateForUndoAction(name)
            self:playcommand("UpdateItemList")
        end,
        CustomizeGameplayElementMovedMessageCommand = function(self, params)
            if params == nil or params.name == nil then return end

            local name = params.name
            updateSelectedElementValues()
            self:playcommand("UpdateItemInfo")
        end,
        CustomizeGameplayElementUndoMessageCommand = function(self, params)
            if params == nil or params.name == nil then return end

            updateSelectedElementValues()
            self:playcommand("UpdateItemInfo")
        end,
        CustomizeGameplayElementDefaultedMessageCommand = function(self, params)
            self:playcommand("CustomizeGameplayElementUndo", params)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(1)
                self:zoomto(actuals.MenuWidth, actuals.MenuHeight)
                registerActorToColorConfigElement(self, "main", "PrimaryBackground")
                self:diffusealpha(uiBGAlpha)
            end,
            MouseScrollMessageCommand = function(self, params)
                if isOver(self) then
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                    self:GetParent():playcommand("UpdateItemList")
                end
            end
        },
        UIElements.QuadButton(1) .. {
            Name = "DraggableLip",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:y(-actuals.MenuHeight / 2)
                self:zoomto(actuals.MenuWidth, actuals.MenuDraggerHeight)
                registerActorToColorConfigElement(self, "main", "SecondaryBackground")
                self:diffusealpha(uiBGAlpha)
            end,
            MouseDragCommand = function(self, params)
                local newx = params.MouseX - (self.initialClickX or 0)
                local newy = params.MouseY - (self.initialClickY or 0)
                self:GetParent():addx(newx):addy(newy)
            end,
            MouseDownCommand = function(self, params)
                self.initialClickX = params.MouseX
                self.initialClickY = params.MouseY
            end,
        },
        Def.ActorFrame {
            Name = "SelectedElementPage",
            InitCommand = function(self)
                self:y(topItemY + (allowedSpace / itemsPerPage / 2))
                self:x(-actuals.MenuWidth/2)
            end,
            UpdateItemListCommand = function(self)
                visibilityBySelectedElement(self, true)
                if selectedElement ~= nil then
                    self:playcommand("UpdateItemInfo")
                end
            end,
            UpdateItemInfoCommand = function(self)
                -- line management
                self.cl = 0
                self.sl = 0
                if selectedElementCoords["x"] then self.cl = self.cl + 1 end
                if selectedElementCoords["y"] then self.cl = self.cl + 1 end
                if selectedElementCoords["rotation"] then self.cl = self.cl + 1 end
                if selectedElementSizes["zoom"] then self.sl = self.sl + 1 end
                if selectedElementSizes["width"] then self.sl = self.sl + 1 end
                if selectedElementSizes["height"] then self.sl = self.sl + 1 end
                if selectedElementSizes["spacing"] then self.sl = self.sl + 1 end
            end,

            LoadFont("Common Normal") .. {
                Name = "ElementName",
                InitCommand = function(self)
                    local line = 1
                    self:valign(0)
                    self:y((allowedSpace / itemsPerPage) * (line - 1) - (allowedSpace / itemsPerPage)/2)
                    self:settext(" ")
                    self:maxwidth(actuals.MenuWidth / elementNameTextSize)
                end,
                UpdateItemInfoCommand = function(self)
                    self:settextf("%s", translations[selectedElement] or selectedElement)
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "CurrentCoordinates",
                InitCommand = function(self)
                    local line = 2
                    self:valign(0)
                    self:y((allowedSpace / itemsPerPage) * (line - 1) - (allowedSpace / itemsPerPage)/2)
                    self:settext(" ")
                    self:maxwidth(actuals.MenuWidth / elementCoordTextSize)
                end,
                UpdateItemInfoCommand = function(self, params)
                    local outstr = {}
                    if selectedElementCoords["x"] ~= nil then
                        local fstr = selectedElementMovementType == "Coordinate" and "[X: %5.2f]" or "X: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, selectedElementCoords["x"])
                    end
                    if selectedElementCoords["y"] ~= nil then
                        local fstr = selectedElementMovementType == "Coordinate" and "[Y: %5.2f]" or "Y: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, selectedElementCoords["y"])
                    end
                    if selectedElementCoords["rotation"] ~= nil then
                        local fstr = selectedElementMovementType == "Rotation" and "[%s: %5.2f]" or "%s: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, translations["Rotation"], selectedElementCoords["rotation"])
                    end
                    self:settextf(table.concat(outstr, "\n"))
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "CurrentSizing",
                InitCommand = function(self)
                    self:valign(0)
                    self:settext(" ")
                    self:maxwidth(actuals.MenuWidth / elementSizeTextSize)
                end,
                UpdateItemInfoCommand = function(self, params)
                    local outstr = {}
                    local coordLines = self:GetParent().cl
                    local coordactor = self:GetParent():GetChild("CurrentCoordinates")
                    self:y(coordactor:GetY() + coordactor:GetZoomedHeight() + (allowedSpace / itemsPerPage)/2)

                    if selectedElementSizes["zoom"] ~= nil then
                        local fstr = selectedElementMovementType == "Zoom" and "[%s: %5.2f]" or "%s: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, translations["Zoom"], selectedElementSizes["zoom"])
                    end
                    if selectedElementSizes["width"] ~= nil then
                        local fstr = selectedElementMovementType == "Size" and "[%s: %5.2f]" or "%s: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, translations["Width"], selectedElementSizes["width"])
                    end
                    if selectedElementSizes["height"] ~= nil then
                        local fstr = selectedElementMovementType == "Size" and "[%s: %5.2f]" or "%s: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, translations["Height"], selectedElementSizes["height"])
                    end
                    if selectedElementSizes["spacing"] ~= nil then
                        local fstr = selectedElementMovementType == "Spacing" and "[%s: %5.2f]" or "%s: %5.2f"
                        outstr[#outstr+1] = string.format(fstr, translations["Spacing"], selectedElementSizes["spacing"])
                    end
                    self:settextf(table.concat(outstr, "\n"))
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Instruction",
                InitCommand = function(self)
                    self:valign(1)
                    self:y(topItemY + (allowedSpace / itemsPerPage / 2))
                    self:addy(actuals.MenuHeight - actuals.EdgePadding)
                    self:zoom(uiInstructionTextSize)
                    self:maxwidth(actuals.MenuWidth / uiInstructionTextSize)
                    self:settext(translations["CustomizeGameplayInstruction"])
                end,
            }
        }
    }

    for i = 1, itemsPerPage do
        t[#t+1] = item(i)
    end

    t[#t+1] = Def.Quad {
        Name = "Cursor",
        InitCommand = function(self)
            self:halign(0)
            registerActorToColorConfigElement(self, "main", "SeparationDivider")
            self:diffusealpha(cursorAlpha)
        end,
        UpdateItemListCommand = function(self)
            visibilityBySelectedElement(self)
        end,
    }
    return t
end

t[#t+1] = Def.ActorFrame {
    Name = "UIContainer",
    InitCommand = function(self)
        self:xy(SCREEN_WIDTH, SCREEN_CENTER_Y)
    end,
    makeUI(),
}

return t
