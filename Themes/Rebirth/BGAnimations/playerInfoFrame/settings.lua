-- every time i look at this file my desire to continue modifying it gets worse
-- at least it isnt totally spaghetti code yet
-- hmm
-- this absolute behemoth of a file ...
local ratios = {
    RightWidth = 782 / 1920,
    LeftWidth = 783 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    BottomLipHeight = 99 / 1080,

    EdgePadding = 12 / 1920, -- distance from edges for text and items

    --
    -- right options
    OptionTextWidth = 275 / 1920, -- left edge of text to edge of area for text
    OptionTextListTopGap = 21 / 1080, -- bottom of right top lip to top of text
    OptionTextBuffer = 7 / 1920, -- distance from end of width to beginning of selection frame
    OptionSelectionFrameWidth = 250 / 1920, -- allowed area for option selection
    OptionBigTriangleHeight = 19 / 1080, -- visually the width most of the time because the triangles are usually turned
    OptionBigTriangleWidth = 20 / 1920,
    OptionSmallTriangleHeight = 12 / 1080,
    OptionSmallTriangleWidth = 13 / 1920,
    OptionSmallTriangleGap = 2 / 1920,
    OptionChoiceDirectionGap = 7 / 1920, -- gap between direction arrow pairs and between direction arrows and choices
    OptionChoiceAllottedWidth = 450 / 1920, -- width between the arrows for MultiChoices basically (or really long SingleChoices)
    OptionChoiceUnderlineThickness = 2 / 1080,

    -- for this area, this is the allowed height for all options including sub options
    -- when an option opens, it may only show as many sub options as there are lines after subtracting the amount of option categories
    -- so 1 category with 24 sub options has 25 lines
    -- 2 categories can then only have up to 23 sub options each to make 25 lines
    -- etc
    OptionAllottedHeight = 672 / 1080, -- from top of top option to bottom of bottom option
    NoteskinDisplayWidth = 240 / 1920, -- width of the text but lets fit the arrows within this
    NoteskinDisplayRightGap = 17 / 1920, -- distance from right edge of frame to right edge of display
    NoteskinDisplayReceptorTopGap = 29 / 1080, -- bottom of text to top of receptors
    NoteskinDisplayTopGap = 21 / 1080, -- bottom of right top lip to top of text

    -- the smaller of these values is used to pick how big the color box is and how tall the sliders are
    ColorBoxHeight = 339 / 1080,
    ColorBoxWidth = 339 / 1920,

    -- controls the width of the mouse wheel scroll box, should be the same number as the general box X position
    -- (found in generalBox.lua)
    GeneralBoxLeftGap = 1140 / 1920, -- distance from left edge to the left edge of the general box
}

local actuals = {
    LeftWidth = ratios.LeftWidth * SCREEN_WIDTH,
    RightWidth = ratios.RightWidth * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    BottomLipHeight = ratios.BottomLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    OptionTextWidth = ratios.OptionTextWidth * SCREEN_WIDTH,
    OptionTextListTopGap = ratios.OptionTextListTopGap * SCREEN_HEIGHT,
    OptionTextBuffer = ratios.OptionTextBuffer * SCREEN_WIDTH,
    OptionSelectionFrameWidth = ratios.OptionSelectionFrameWidth * SCREEN_WIDTH,
    OptionBigTriangleHeight = ratios.OptionBigTriangleHeight * SCREEN_HEIGHT,
    OptionBigTriangleWidth = ratios.OptionBigTriangleWidth * SCREEN_WIDTH,
    OptionSmallTriangleHeight = ratios.OptionSmallTriangleHeight * SCREEN_HEIGHT,
    OptionSmallTriangleWidth = ratios.OptionSmallTriangleWidth * SCREEN_WIDTH,
    OptionSmallTriangleGap = ratios.OptionSmallTriangleGap * SCREEN_WIDTH,
    OptionChoiceDirectionGap = ratios.OptionChoiceDirectionGap * SCREEN_WIDTH,
    OptionChoiceAllottedWidth = ratios.OptionChoiceAllottedWidth * SCREEN_WIDTH,
    OptionChoiceUnderlineThickness = ratios.OptionChoiceUnderlineThickness * SCREEN_HEIGHT,
    OptionAllottedHeight = ratios.OptionAllottedHeight * SCREEN_HEIGHT,
    NoteskinDisplayWidth = ratios.NoteskinDisplayWidth * SCREEN_WIDTH,
    NoteskinDisplayRightGap = ratios.NoteskinDisplayRightGap * SCREEN_WIDTH,
    NoteskinDisplayReceptorTopGap = ratios.NoteskinDisplayReceptorTopGap * SCREEN_HEIGHT,
    NoteskinDisplayTopGap = ratios.NoteskinDisplayTopGap * SCREEN_HEIGHT,
    ColorBoxHeight = ratios.ColorBoxHeight * SCREEN_HEIGHT,
    ColorBoxWidth = ratios.ColorBoxWidth * SCREEN_WIDTH,
    GeneralBoxLeftGap = ratios.GeneralBoxLeftGap * SCREEN_WIDTH,
}

local visibleframeY = SCREEN_HEIGHT - actuals.Height
local animationSeconds = 0.1
local focused = false
local lefthidden = false

local titleTextSize = 0.8
local explanationTextSize = 0.8
local textZoomFudge = 5

local choiceTextSize = 0.8
local buttonHoverAlpha = 0.6
local previewOpenedAlpha = 0.6
local previewButtonTextSize = 0.8

local keyinstructionsTextSize = 0.7
local bindingChoicesTextSize = 0.75
local currentlybindingTextSize = 0.7
local menuBindingTextSize = 0.7
local colorConfigTextSize = 0.75
local colorConfigChoiceTextSize = 0.75

local optionTitleTextSize = 0.7
local optionChoiceTextSize = 0.7
-- for accessibility concerns, make buttons a bit bigger than the text they cover
local textButtonHeightFudgeScalarMultiplier = 1.6
local optionRowAnimationSeconds = 0.15
local optionRowQuickAnimationSeconds = 0.07
-- theoretically this is how long it takes for text to write out when queued by the explanation text
-- but because the game isnt perfect this isnt true at all
-- (but changing this number does make a difference)
local explanationTextWriteAnimationSeconds = 0.2
-- color config list animation time
local itemListAnimationSeconds = 0.1

local maxExplanationTextLines = 2

-- lost patience
-- undertaking this was a massive mistake
-- hope you people like it
SCUFF.showingNoteskins = false
SCUFF.showingPreview = false
SCUFF.showingColor = false
SCUFF.showingKeybinds = false

-- reset customize gameplay here
-- couldnt think of a really good place to put it instead
playerConfig:get_data().CustomizeGameplay = false
playerConfig:set_dirty()
playerConfig:save()

local t = Def.ActorFrame {
    Name = "SettingsFile",
    InitCommand = function(self)
        self:y(visibleframeY)
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:playcommand("HideLeft")
        self:playcommand("HideRight")
        MESSAGEMAN:Broadcast("ShowWheel")
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "Settings" then
            --
            -- movement is delegated to the left and right halves
            -- right half immediately comes out
            -- left half comes out when selecting "Customize Playfield" or "Customize Keybinds" or some appropriate choice
            --
            self:diffusealpha(1)
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
            self:playcommand("ShowRight")
            self:playcommand("HideLeft")
            MESSAGEMAN:Broadcast("ShowWheel")
        else
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(0)
            self:playcommand("HideLeft")
            self:playcommand("HideRight")
            MESSAGEMAN:Broadcast("ShowWheel")
            focused = false
        end
    end,
    FinishFocusingCommand = function(self)
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Settings")
    end,
    ShowSettingsAltMessageCommand = function(self, params)
        if params and params.name then
            self:playcommand("ShowLeft", params)
        else
            self:playcommand("HideLeft")
        end
    end,
    OptionCursorUpdatedMessageCommand = function(self, params)
        if params and params.name then
            -- will only work when hovering certain options
            if SCUFF.optionsThatWillOpenTheLeftSideWhenHovered[params.name] ~= nil then
                MESSAGEMAN:Broadcast("ShowSettingsAlt", params)
            else
                -- if moving off of the noteskin tab (without keybinds)
                if SCUFF.showingNoteskins and not SCUFF.showingKeybinds then
                    self:playcommand("HideLeft")
                    -- HACK HACK HACK HACK HACK
                    MESSAGEMAN:Broadcast("ShowSettingsAlt")
                    CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Settings")
                end
            end
        end
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetPosition")
    end,
}


local function leftFrame()
    local offscreenX = -actuals.LeftWidth
    local onscreenX = 0

    local t = Def.ActorFrame {
        Name = "LeftFrame",
        InitCommand = function(self)
            self:playcommand("SetPosition")
            self:diffusealpha(0)
        end,
        BeginCommand = function(self)
            self:playcommand("HideLeft")
        end,
        HideLeftCommand = function(self)
            -- move off screen left and go invisible
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(0)
            self:x(offscreenX)
            lefthidden = true
        end,
        ShowLeftCommand = function(self, params)
            -- move on screen from left and go visible
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(1)
            self:x(onscreenX)
            lefthidden = false
        end,
        SetPositionCommand = function(self)
            if getWheelPosition() then
                onscreenX = 0
                offscreenX = -actuals.LeftWidth
            else
                onscreenX = SCREEN_WIDTH - actuals.LeftWidth
                offscreenX = SCREEN_WIDTH
            end
            if lefthidden then
                self:x(offscreenX)
            else
                self:x(onscreenX)
            end
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(actuals.LeftWidth, actuals.Height)
                self:diffusealpha(0.6)
                registerActorToColorConfigElement(self, "main", "PrimaryBackground")
            end
        },
        Def.Quad {
            Name = "TopLip",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(actuals.LeftWidth, actuals.TopLipHeight)
                self:diffusealpha(0.6)
                registerActorToColorConfigElement(self, "main", "SecondaryBackground")
            end
        },
        LoadFont("Common Normal") .. {
            Name = "HeaderText",
            InitCommand = function(self)
                self:halign(0)
                self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
                self:zoom(titleTextSize)
                self:maxwidth((actuals.LeftWidth - actuals.EdgePadding*2) / titleTextSize - textZoomFudge)
                self:settext("")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            ShowLeftCommand = function(self, params)
                if params and params.name then
                    self:settext(params.name)
                end
            end,

        }
    }

    -- the noteskin page function as noteskin preview and keybindings
    local function createNoteskinPage()
        -- list of GameButtons we can map
        local gameButtonsToMap = INPUTMAPPER:GetGameButtonsToMap()
        -- list of MenuButtons we can map
        -- could be grabbed by INPUTMAPPER:GetMenuButtonsToMap() but want to be really specific
        local menuButtonsToMap = {
            "Coin",
            "EffectUp",
            "EffectDown",
            "RestartGameplay"
        }
        local inMenuPage = false
        local aspectRatioProportion = (16/9) / (SCREEN_WIDTH / SCREEN_HEIGHT)
        local menuBoxSize = 48 * aspectRatioProportion -- hard coded. do not care (add 20 more menu buttons then i will care)
        local currentController = 0
        local currentlyBinding = false
        local currentKey = ""
        local cursorIndex = 1
        local automaticallyBindingEverything = false -- when true, move forward until we bound the last allowed index
        local optionActive = 0 -- 0 = nothing, 1 = bind all, 2 = swap pages (to keep track of vertical hover position)

        -- entries into this list are not allowed to be bound
        local bannedKeys = {
            -- valid entries:
            -- "key" (all keyboard input)
            -- "mouse" (all mouse input)
            -- "cz" (the letter z)
            -- "left" (the left arrow on the keyboard)
            -- "kp 2" (2 on the numpad)
            mouse = true,
            enter = true,
            escape = true,
        }

        -- function to remove all double+ binding and leave only defaults
        -- this goes out to all cheaters and losers
        -- if you want to use double bindings dont touch this settings menu
        local function setUpKeyBindings()
            INPUTBINDING:RemoveDoubleBindings(false)
            automaticallyBindingEverything = false

            -- kill your precious menu double bindings (not gonna lie couldnt think of a better way to guarantee what you see is what is bound)
            -- will only mess with the left side bindings (controller 0)
            for _, b in ipairs(menuButtonsToMap) do
                local tmp = INPUTMAPPER:GetButtonMapping(b, 0, INPUTBINDING.defaultColumn)
                if tmp == nil then tmp = "nil" end
                for col = 0, INPUTBINDING.maxColumn do
                    INPUTMAPPER:SetInputMap("", b, col, 0)
                end
                INPUTMAPPER:SetInputMap(tmp, b, INPUTBINDING.defaultColumn, 0)
            end

            MESSAGEMAN:Broadcast("UpdatedBoundKeys")
        end

        -- just moves the cursor, for keyboard compatibility only
        local function selectKeybind(direction)
            local n = cursorIndex + direction
            local maxindex = not inMenuPage and #gameButtonsToMap*2 or #menuButtonsToMap
            if n > maxindex then n = 1 end
            if n < 1 then n = maxindex end

            cursorIndex = n
            MESSAGEMAN:Broadcast("UpdatedBoundKeys")
        end

        -- switch page between menu buttons and game buttons
        local function switchBindingPage()
            inMenuPage = not inMenuPage
            currentKey = ""
            currentlyBinding = false
            currentController = 0
            cursorIndex = 1
            automaticallyBindingEverything = false
            optionActive = 0

            MESSAGEMAN:Broadcast("UpdatedBoundKeys") -- hack to get visible cursor position to update
            MESSAGEMAN:Broadcast("BindingPageSet")
        end

        -- select this specific key to begin binding, lock input
        local function startBinding(buttonName, controller)
            currentKey = buttonName
            currentController = controller
            currentlyBinding = true
            MESSAGEMAN:Broadcast("StartedBinding", {key = currentKey, controller = controller})
        end
        local function stopBinding()
            currentlyBinding = false
            automaticallyBindingEverything = false
            MESSAGEMAN:Broadcast("StoppedBinding")
        end
        local function startBindingEverything()
            cursorIndex = 1
            automaticallyBindingEverything = true
            optionActive = 0
            local controller = ((not inMenuPage and cursorIndex > #gameButtonsToMap) and 1 or 0)
            local buttonindex = controller == 0 and cursorIndex or cursorIndex - #gameButtonsToMap
            local buttonbinding = not inMenuPage and gameButtonsToMap[buttonindex] or menuButtonsToMap[buttonindex]
            MESSAGEMAN:Broadcast("UpdatedBoundKeys") -- hack to get visible cursor position to update
            startBinding(buttonbinding, controller)
        end

        -- for the currentKey, use this InputEventPlus to bind the pressed key to the button
        local function bindCurrentKey(event)
            if event == nil or event.DeviceInput == nil then return end -- ??
            local dev = event.DeviceInput.device
            if dev == nil then return end -- ???
            local key = event.DeviceInput.button
            if key == nil then return end -- ????
            local spldev = strsplit(dev, "_")
            if spldev == nil or #spldev ~= 2 then return end -- ?????
            local splkey = strsplit(key, "_")
            if splkey == nil or #splkey ~= 2 then return end -- ??????
            local pizzaHut = spldev[2]:lower()
            local tacoBell = splkey[2]:lower()
            -- numpad buttons and F keys are case sensitive
            if tacoBell:sub(1,2) == "kp" then
                tacoBell = tacoBell:gsub("kp", "KP")
            elseif tacoBell:sub(1,1) == "f" and tonumber(tacoBell:sub(2,2)) ~= nil then
                tacoBell = tacoBell:gsub("f", "F")
            end
            local combinationPizzaHutAndTacoBell = (pizzaHut .. "_" .. tacoBell)
            -- not gonna bother finding a better way to do all that
            if currentKey == nil or #currentKey == 0 then return end -- ???????
            if bannedKeys[tacoBell] or bannedKeys[pizzaHut] or bannedKeys[combinationPizzaHutAndTacoBell] then return true end -- ????????

            -- bind it
            INPUTMAPPER:SetInputMap(combinationPizzaHutAndTacoBell, currentKey, INPUTBINDING.defaultColumn, currentController)
            -- check to see if the button bound
            local result = INPUTMAPPER:GetButtonMapping(currentKey, currentController, INPUTBINDING.defaultColumn)
            return result ~= nil
        end

        local t = Def.ActorFrame {
            Name = "NoteSkinPageContainer",
            ShowLeftCommand = function(self, params)
                if params and (params.name == "Noteskin" or params.name == "Customize Keybinds") then
                    if params.name == "Customize Keybinds" then
                        SCUFF.showingKeybinds = true
                        setUpKeyBindings()
                        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Keybindings")
                    else
                        if SCUFF.showingKeybinds then
                            INPUTMAPPER:SaveMappingsToDisk()
                        end
                        SCUFF.showingKeybinds = false
                    end
                    self:diffusealpha(1)
                    self:z(1)
                    SCUFF.showingNoteskins = true
                else
                    self:playcommand("HideLeft")
                end
            end,
            HideLeftCommand = function(self)
                self:diffusealpha(0)
                self:z(-1)
                -- save when exiting
                if SCUFF.showingKeybinds then
                    INPUTMAPPER:SaveMappingsToDisk()
                end
                SCUFF.showingNoteskins = false
                SCUFF.showingKeybinds = false
            end,
            BeginCommand = function(self)
                local snm = SCREENMAN:GetTopScreen():GetName()
                local anm = self:GetName()

                -- cursor input management for keybindings
                -- noteskin display is not relevant for this, just contains it for reasons
                CONTEXTMAN:RegisterToContextSet(snm, "Keybindings", anm)
                CONTEXTMAN:ToggleContextSet(snm, "Keybindings", false)

                SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                    -- if locked out, dont allow
                    if not CONTEXTMAN:CheckContextSet(snm, "Keybindings") then return end
                    if event.type ~= "InputEventType_Release" then -- allow Repeat and FirstPress
                        local gameButton = event.button
                        local key = event.DeviceInput.button
                        local up = gameButton == "Up" or gameButton == "MenuUp"
                        local down = gameButton == "Down" or gameButton == "MenuDown"
                        local right = gameButton == "MenuRight" or gameButton == "Right"
                        local left = gameButton == "MenuLeft" or gameButton == "Left"
                        local enter = gameButton == "Start"
                        local ctrl = INPUTFILTER:IsBeingPressed("left ctrl") or INPUTFILTER:IsBeingPressed("right ctrl")
                        local back = key == "DeviceButton_escape"
                        local rightclick = key == "DeviceButton_right mouse button"
                        local leftclick = key == "DeviceButton_left mouse button"

                        if not currentlyBinding and left then
                            -- functionality to attempt to make vertical choice movement very slightly more intuitive
                            -- this probably just confuses the user
                            -- dont care
                            if inMenuPage and optionActive == 0 and cursorIndex == 1 then
                                optionActive = 2
                                cursorIndex = 0
                            elseif inMenuPage and optionActive == 2 then
                                optionActive = 1
                            else
                                optionActive = 0
                                selectKeybind(-1)
                            end
                            self:playcommand("Set")
                        elseif not currentlyBinding and right then
                            if inMenuPage and optionActive == 0 and cursorIndex == #menuButtonsToMap then
                                optionActive = 1
                                cursorIndex = 0
                            elseif inMenuPage and optionActive == 1 then
                                optionActive = 2
                            else
                                optionActive = 0
                                selectKeybind(1)
                            end
                            self:playcommand("Set")
                        elseif not currentlyBinding and (up or down) then
                            -- functionality to let movement go into the extra vertical choices
                            -- because we orient the menu page vertically it makes menu logic that much more cancer
                            if optionActive == 1 then -- hovered "bind all"
                                if up then
                                    optionActive = 0
                                    cursorIndex = inMenuPage and #menuButtonsToMap or 1
                                else
                                    optionActive = 2
                                end
                            elseif optionActive == 2 then -- hovered "swap page"
                                if up then
                                    optionActive = 1
                                else
                                    optionActive = 0
                                    cursorIndex = 1
                                end
                            elseif optionActive <= 0 then -- not hovered on anything
                                if inMenuPage then
                                    if up and cursorIndex == 1 then
                                        optionActive = 2
                                        cursorIndex = 0
                                    elseif down and cursorIndex == #menuButtonsToMap then
                                        optionActive = 1
                                        cursorIndex = 0
                                    elseif down then
                                        selectKeybind(1)
                                    elseif up then
                                        selectKeybind(-1)
                                    end
                                else
                                    cursorIndex = 0
                                    if up then
                                        optionActive = 2
                                    else
                                        optionActive = 1
                                    end
                                end
                            end
                            self:playcommand("Set")
                        elseif not currentlyBinding and enter then
                            if cursorIndex <= 0 then
                                -- logic for pressing enter on the vertical choices
                                -- this is really hacked
                                if optionActive == 1 then
                                    -- bind all start
                                    startBindingEverything()
                                    self:playcommand("Set")
                                elseif optionActive == 2 then
                                    -- swap page button
                                    switchBindingPage()
                                    optionActive = 2
                                    cursorIndex = 0
                                    MESSAGEMAN:Broadcast("UpdatedBoundKeys") -- hack to get visible cursor position to update
                                    self:playcommand("Set")
                                end
                            else
                                -- i overcomplicated logic here just for you, reader. you are welcome
                                -- (consider menu bindings, use either the gamebutton table or the menubutton table)
                                local controller = ((not inMenuPage and cursorIndex > #gameButtonsToMap) and 1 or 0)
                                local buttonindex = controller == 0 and cursorIndex or cursorIndex - #gameButtonsToMap
                                local buttonbinding = not inMenuPage and gameButtonsToMap[buttonindex] or menuButtonsToMap[buttonindex]
                                startBinding(buttonbinding, controller)
                            end
                        elseif not currentlyBinding and back then
                            -- shortcut to exit back to settings
                            -- press twice to exit back to general
                            MESSAGEMAN:Broadcast("PlayerInfoFrameTabSet", {tab = "Settings"})
                        elseif currentlyBinding and (back or rightclick or leftclick) then
                            -- cancel the binding process
                            -- update highlights
                            stopBinding()
                            self:playcommand("Set")
                        elseif currentlyBinding then
                            -- pressed a button that could potentially be bindable and we should bind it
                            local result = bindCurrentKey(event)
                            if result then
                                if automaticallyBindingEverything then
                                    local cursorbefore = cursorIndex
                                    selectKeybind(1)
                                    -- if the cursor moved backwards, we finished binding everything
                                    if cursorIndex < cursorbefore then
                                        stopBinding()
                                    else
                                        local controller = ((not inMenuPage and cursorIndex > #gameButtonsToMap) and 1 or 0)
                                        local buttonindex = controller == 0 and cursorIndex or cursorIndex - #gameButtonsToMap
                                        local buttonbinding = not inMenuPage and gameButtonsToMap[buttonindex] or menuButtonsToMap[buttonindex]
                                        startBinding(buttonbinding, controller)
                                    end
                                else
                                    stopBinding()
                                end
                            else
                                ms.ok(currentKey)
                                ms.ok(currentController)
                                ms.ok("There was some error in attempting to bind the key... Report to developers")
                            end
                            self:playcommand("Set")
                        else
                            -- nothing happens
                            return
                        end
                    end
                end)
            end,
        }

        -- yeah these numbers are bogus (but are in fact based on the 4key numbers so they arent all that bad)
        local columnwidth = 64
        local noteskinwidthbaseline = 256
        local secondrowYoffset = 64
        local noteskinbasezoom = 1.5 -- pick a zoom that fits 4key in 16:9 aspect ratio
        local NSDirTable = GivenGameToFullNSkinElements(GAMESTATE:GetCurrentGame():GetName())
        local keybindBGSizeMultiplier = 0.97 -- this is multiplied with columnwidth
        local keybindBG2SizeMultiplier = 0.97 -- this is multiplied with columnwidth and keybindBGSizeMultiplier
        local keybindingTextSize = 1 -- text size inside the key button thing
        -- calculation: find a zoom that fits for the current chosen column count the same way 4key on 16:9 does
        local noteskinzoom = noteskinbasezoom / (#NSDirTable * columnwidth / noteskinwidthbaseline) / aspectRatioProportion

        -- finds noteskin index
        local function findNoteskinIndex(skin)
            local nsnames = NOTESKIN:GetNoteSkinNames()
            for i, name in ipairs(nsnames) do
                if name:lower() == skin:lower() then
                    return i
                end
            end
            return 1
        end

        local tt = Def.ActorFrame {
            Name = "SkinContainer",
            InitCommand = function(self)
                self:x(actuals.LeftWidth / 2)
                self:zoom(noteskinzoom)
                self:y(actuals.Height / 4)
            end,
            OnCommand = function(self)
                local ind = findNoteskinIndex(getPlayerOptions():NoteSkin())
                self:playcommand("SetSkinVisibility", {index = ind})
            end,
            UpdateVisibleSkinMessageCommand = function(self, params)
                local ind = findNoteskinIndex((params or {}).name or "")
                self:playcommand("SetSkinVisibility", {index = ind})
            end,
            BindingPageSetMessageCommand = function(self)
                if inMenuPage then
                    self:diffusealpha(0)
                else
                    self:diffusealpha(1)
                end
            end,
            ShowLeftCommand = function(self)
                if SCUFF.showingKeybinds then
                    self:x(actuals.LeftWidth / 3)
                    self:zoom(noteskinzoom / 2)
                    self:playcommand("BindingPageSet")
                else
                    self:x(actuals.LeftWidth / 2)
                    self:zoom(noteskinzoom)
                end
            end,
        }
        -- works almost exactly like the legacy PlayerOptions preview
        -- except has some secret things attached
        -- at this point in time we cannot load every Game's noteskin like I would like to
        for i, dir in ipairs(NSDirTable) do
            -- so the elements are centered
            -- add half a column width because elements are center aligned
            local leftoffset = -columnwidth * #NSDirTable / 2 + columnwidth / 2
            local tapForThisIteration = nil
            local receptorForThisIteration = nil

            -- load taps
            tt[#tt+1] = Def.ActorFrame {
                InitCommand = function(self)
                    self:x(leftoffset + columnwidth * (i-1))
                    self:y(secondrowYoffset)
                    tapForThisIteration = self
                end,
                Def.ActorFrame {
                    LoadNSkinPreview("Get", dir, "Tap Note", false) .. {
                        OnCommand = function(self)
                            for i = 1, #NOTESKIN:GetNoteSkinNames() do
                                local c = self:GetChild("N"..i)
                                c:visible(true)
                            end
                        end,
                        SetSkinVisibilityCommand = function(self, params)
                            if params and params.index then
                                local ind = params.index
                                -- noteskin displays are actually many sprites in one spot
                                -- for the chosen noteskin, display only the one we want
                                -- have to search the list to find it
                                for i = 1, #NOTESKIN:GetNoteSkinNames() do
                                    local c = self:GetChild("N"..i)
                                    if i == ind then
                                        c:diffusealpha(1)
                                    else
                                        c:diffusealpha(0)
                                    end
                                end
                            end
                        end,
                    }
                },
            }
            -- load receptors
            tt[#tt+1] = Def.ActorFrame {
                InitCommand = function(self)
                    self:x(leftoffset + columnwidth * (i-1))
                    receptorForThisIteration = self
                end,
                Def.ActorFrame {
                    LoadNSkinPreview("Get", dir, "Receptor", false) .. {
                        OnCommand = function(self)
                            for i = 1, #NOTESKIN:GetNoteSkinNames() do
                                local c = self:GetChild("N"..i)
                                c:visible(true)
                            end
                        end,
                        SetSkinVisibilityCommand = function(self, params)
                            if params and params.index then
                                local ind = params.index
                                -- noteskin displays are actually many sprites in one spot
                                -- for the chosen noteskin, display only the one we want
                                -- have to search the list to find it
                                for i = 1, #NOTESKIN:GetNoteSkinNames() do
                                    local c = self:GetChild("N"..i)
                                    if i == ind then
                                        c:diffusealpha(1)
                                    else
                                        c:diffusealpha(0)
                                    end
                                end
                            end
                        end,
                    }
                },
            }
            -- load shadow taps (doubles modes)
            tt[#tt+1] = Def.ActorProxy {
                InitCommand = function(self)
                    -- ActorProxy offsets only have to be relative to the original
                    -- set x to the same as the highest offset
                    self:x(columnwidth * (#NSDirTable))
                end,
                BeginCommand = function(self)
                    self:SetTarget(tapForThisIteration)
                end,
                ShowLeftCommand = function(self)
                    if SCUFF.showingKeybinds then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end
                end,
            }
            -- load shadow receptors (doubles modes)
            tt[#tt+1] = Def.ActorProxy {
                InitCommand = function(self)
                    -- ActorProxy offsets only have to be relative to the original
                    -- set x to the same as the highest offset
                    self:x(columnwidth * (#NSDirTable))
                end,
                BeginCommand = function(self)
                    self:SetTarget(receptorForThisIteration)
                end,
                ShowLeftCommand = function(self)
                    if SCUFF.showingKeybinds then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end
                end,
            }
            -- load keybinding display
            -- this is put into a function to prevent a lot of copy pasting and unmaintainability
            local function keybindingDisplay(i, isDoublesSide)
                -- the doubles side starting index is #NSDirTable+1
                local trueIndex = i + (isDoublesSide and #NSDirTable or 0)
                local controller = isDoublesSide and 1 or 0
                return Def.ActorFrame {
                    Name = "KeybindingFrame",
                    InitCommand = function(self)
                        self:x(leftoffset + columnwidth * (i-1))
                        if isDoublesSide then
                            self:addx(columnwidth * #NSDirTable)
                        end
                        self:y(secondrowYoffset * 2)
                    end,
                    ShowLeftCommand = function(self)
                        if SCUFF.showingKeybinds then
                            self:diffusealpha(1)
                        else
                            self:diffusealpha(0)
                        end
                    end,
                    UIElements.QuadButton(1, 1) .. {
                        Name = "KeybindBGBG",
                        InitCommand = function(self)
                            -- font color
                            self:zoomto(columnwidth * keybindBGSizeMultiplier, columnwidth * keybindBGSizeMultiplier)
                            self:playcommand("Set")
                            registerActorToColorConfigElement(self, "options", "KeybindButtonEdge")
                        end,
                        SetAlphaCommand = function(self)
                            if isOver(self) or cursorIndex == trueIndex then
                                self:diffusealpha(0.6 * buttonHoverAlpha)
                            else
                                self:diffusealpha(0.6)
                            end
                        end,
                        SetCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        UpdatedBoundKeysMessageCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        MouseOverCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        MouseOutCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        MouseDownCommand = function(self)
                            if self:IsInvisible() then return end
                            if not currentlyBinding and not inMenuPage then
                                local dist = trueIndex - cursorIndex
                                selectKeybind(dist)
                                startBinding(gameButtonsToMap[i], controller)
                            end
                        end,
                    },
                    Def.Quad {
                        Name = "KeybindBG",
                        InitCommand = function(self)
                            -- generally bg color
                            self:diffusealpha(0.6)
                            self:zoomto(columnwidth * keybindBGSizeMultiplier * keybindBG2SizeMultiplier, columnwidth * keybindBGSizeMultiplier * keybindBG2SizeMultiplier)
                            registerActorToColorConfigElement(self, "options", "KeybindButtonBackground")
                        end,
                    },
                    LoadFont("Common Large") .. {
                        Name = "KeybindText",
                        InitCommand = function(self)
                            self:zoom(keybindingTextSize)
                            self:maxwidth(columnwidth * keybindBGSizeMultiplier * keybindBGSizeMultiplier / keybindingTextSize)
                            self:maxheight(columnwidth * keybindBGSizeMultiplier * keybindBG2SizeMultiplier / keybindingTextSize)
                            registerActorToColorConfigElement(self, "main", "PrimaryText")
                        end,
                        UpdatedBoundKeysMessageCommand = function(self)
                            self:playcommand("Set")
                        end,
                        SetCommand = function(self)
                            local buttonmapped = INPUTMAPPER:GetButtonMappingString(gameButtonsToMap[i], controller, INPUTBINDING.defaultColumn)
                            if buttonmapped then
                                self:settext(buttonmapped:gsub("Key ", ""))
                            else
                                self:settext("none")
                            end
                        end,
                    }
                }
            end
            tt[#tt+1] = keybindingDisplay(i, false)
            tt[#tt+1] = keybindingDisplay(i, true)
        end
        t[#t+1] = tt

        -- more elements to keybinding screen
        -- many numbers which follow are fudged hard
        -- this function creates a menu binding element for only player 1
        local function menuBinding(i, key)
            return Def.ActorFrame {
                Name = "KeybindingFrame",
                    InitCommand = function(self)
                        self:x(actuals.LeftWidth / 8)
                        self:y(menuBoxSize * (i-1))
                    end,
                    UIElements.QuadButton(1, 1) .. {
                        Name = "KeybindBGBG",
                        InitCommand = function(self)
                            -- font color
                            self:zoomto(menuBoxSize * keybindBGSizeMultiplier, menuBoxSize * keybindBGSizeMultiplier)
                            self:playcommand("Set")
                            registerActorToColorConfigElement(self, "options", "KeybindButtonEdge")
                        end,
                        SetAlphaCommand = function(self)
                            if isOver(self) or cursorIndex == i then
                                self:diffusealpha(0.6 * buttonHoverAlpha)
                            else
                                self:diffusealpha(0.6)
                            end
                        end,
                        SetCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        UpdatedBoundKeysMessageCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        MouseOverCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        MouseOutCommand = function(self)
                            self:playcommand("SetAlpha")
                        end,
                        MouseDownCommand = function(self)
                            if not currentlyBinding then
                                local dist = i - cursorIndex
                                selectKeybind(dist)
                                startBinding(key, 0)
                            end
                        end,
                    },
                    Def.Quad {
                        Name = "KeybindBG",
                        InitCommand = function(self)
                            -- generally bg color
                            self:diffusealpha(0.6)
                            self:zoomto(menuBoxSize * keybindBGSizeMultiplier * keybindBG2SizeMultiplier, menuBoxSize * keybindBGSizeMultiplier * keybindBG2SizeMultiplier)
                            registerActorToColorConfigElement(self, "options", "KeybindButtonBackground")
                        end,
                    },
                    LoadFont("Common Large") .. {
                        Name = "KeybindText",
                        InitCommand = function(self)
                            self:zoom(keybindingTextSize)
                            self:maxwidth(menuBoxSize * keybindBGSizeMultiplier * keybindBGSizeMultiplier / keybindingTextSize)
                            self:maxheight(menuBoxSize * keybindBGSizeMultiplier * keybindBG2SizeMultiplier / keybindingTextSize)
                            registerActorToColorConfigElement(self, "main", "PrimaryText")
                        end,
                        UpdatedBoundKeysMessageCommand = function(self)
                            self:playcommand("Set")
                        end,
                        SetCommand = function(self)
                            local buttonmapped = INPUTMAPPER:GetButtonMappingString(key, 0, INPUTBINDING.defaultColumn)
                            if buttonmapped then
                                self:settext(buttonmapped:gsub("Key ", ""))
                            else
                                self:settext("none")
                            end
                        end,
                    },
                    LoadFont("Common Normal") .. {
                        Name = "KeybindButtonText",
                        InitCommand = function(self)
                            self:x(menuBoxSize / 2 + 5)
                            self:halign(0)
                            self:zoom(menuBindingTextSize)
                            self:maxwidth((actuals.LeftWidth - menuBoxSize * 3 - actuals.LeftWidth/8) / menuBindingTextSize)
                            self:settext(key:gsub("_", " "))
                            registerActorToColorConfigElement(self, "main", "PrimaryText")
                        end,
                    }
            }
        end
        t[#t+1] = Def.ActorFrame {
            Name = "ExtraKeybindingElementsFrame",
            ShowLeftCommand = function(self)
                if SCUFF.showingKeybinds then
                    self:diffusealpha(1)
                else
                    self:diffusealpha(0)
                end
            end,

            LoadFont("Common Normal") .. {
                Name = "CurrentlyBinding",
                InitCommand = function(self)
                    self:valign(1)
                    self:x(actuals.LeftWidth/2)
                    self:maxwidth(actuals.LeftWidth / currentlybindingTextSize)
                    self:playcommand("Set")
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self)
                    if inMenuPage then
                        self:y(actuals.Height / 1.5)
                    else
                        self:y(actuals.Height / 2)
                    end

                    if currentlyBinding and not inMenuPage then
                        self:settextf("Currently Binding: %s (Controller %s)", currentKey, currentController)
                    elseif currentlyBinding and inMenuPage then
                        self:settextf("Currently Binding: %s", currentKey)
                    else
                        self:settext("Currently Binding: ")
                    end
                end,
                StartedBindingMessageCommand = function(self)
                    self:playcommand("Set")
                end,
                StoppedBindingMessageCommand = function(self)
                    self:playcommand("Set")
                end,
                BindingPageSetMessageCommand = function(self)
                    self:playcommand("Set")
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "Instructions",
                InitCommand = function(self)
                    self:valign(0)
                    self:xy(actuals.LeftWidth/2, actuals.TopLipHeight * 1.2)
                    self:zoom(keyinstructionsTextSize)
                    self:wrapwidthpixels(actuals.LeftWidth - 10)
                    self:maxheight((actuals.Height / 4 - actuals.TopLipHeight * 1.5) / keyinstructionsTextSize)
                    self:settext("Select a button to rebind with mouse or keyboard.\nPress Escape or click to cancel binding.")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "StartBindingAll",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0)
                    bg:halign(0)
                    self:xy(actuals.EdgePadding, actuals.Height/2 + actuals.Height/4)
                    txt:zoom(bindingChoicesTextSize)
                    txt:maxwidth(actuals.LeftWidth / bindingChoicesTextSize)
                    txt:settext("Start Binding All")
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                    bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    bg:diffusealpha(0.2)
                    self.alphaDeterminingFunction = function(self)
                        local multiplier = optionActive == 1 and buttonHoverAlpha or 1
                        if isOver(bg) then
                            self:diffusealpha(buttonHoverAlpha * multiplier)
                        else
                            self:diffusealpha(1 * multiplier)
                        end
                    end
                end,
                SetCommand = function(self)
                    if self:IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        startBindingEverything()
                    end
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "ToggleAdvancedKeybindings",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0)
                    bg:halign(0)
                    self:xy(actuals.EdgePadding, actuals.Height/2 + actuals.Height/4 + 30 * bindingChoicesTextSize)
                    txt:zoom(bindingChoicesTextSize)
                    txt:maxwidth(actuals.LeftWidth / bindingChoicesTextSize)
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                    bg:diffusealpha(0.2)
                    self:playcommand("BindingPageSet")
                    self.alphaDeterminingFunction = function(self)
                        local multiplier = optionActive == 2 and buttonHoverAlpha or 1
                        if isOver(bg) then
                            self:diffusealpha(buttonHoverAlpha * multiplier)
                        else
                            self:diffusealpha(1 * multiplier)
                        end
                    end
                end,
                BindingPageSetMessageCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    if inMenuPage then
                        txt:settext("View Gameplay Keybindings")
                    else
                        txt:settext("View Menu Keybindings")
                    end
                    bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                end,
                SetCommand = function(self)
                    if self:IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        switchBindingPage()
                    end
                end,
            },
        }

        -- to collect all the menu bindings
        local function mbf()
            local t = Def.ActorFrame {
                Name = "MenuBindingFrame",
                InitCommand = function(self)
                    self:y(actuals.Height / 4)
                    self:playcommand("BindingPageSet")
                end,
                BindingPageSetMessageCommand = function(self)
                    if inMenuPage then
                        self:diffusealpha(1)
                        self:z(1)
                    else
                        self:diffusealpha(0)
                        self:z(-1)
                    end
                end,
            }
            for i, b in ipairs(menuButtonsToMap) do
                t[#t+1] = menuBinding(i, b)
            end
            return t
        end
        t[#t+1] = mbf()

        return t
    end

    -- the notefield preview, an optional showcase of what mods are doing
    -- literally a copy of chart preview -- an ActorProxy
    local function createPreviewPage()
        local t = Def.ActorFrame {
            Name = "PreviewPageContainer",
            ShowLeftCommand = function(self, params)
                -- dont open the preview if left is already opened and it is being used
                if params and params.name == "Preview" and not SCUFF.showingNoteskins and not SCUFF.showingColor then
                    self:diffusealpha(1)
                    self:z(1)
                    SCUFF.showingPreview = true
                    MESSAGEMAN:Broadcast("PreviewPageOpenStatusChanged", {opened = true})
                else
                    self:playcommand("HideLeft")
                end
            end,
            HideLeftCommand = function(self)
                self:diffusealpha(0)
                self:z(-1)
                SCUFF.showingPreview = false
                MESSAGEMAN:Broadcast("PreviewPageOpenStatusChanged", {opened = false})
            end,

            -- the preview notefield (but not really)
            Def.ActorProxy {
                Name = "NoteField",
                InitCommand = function(self)
                    -- centered horizontally and vertically
                    self:x(actuals.LeftWidth / 2)
                    self:y(actuals.Height / 4)
                end,
                BeginCommand = function(self)
                    -- take the long road to find the actual chart preview actor
                    local realnotefieldpreview = SCREENMAN:GetTopScreen():safeGetChild(
                        "RightFrame",
                        "GeneralBoxFile",
                        "Container",
                        "GeneralPageFile",
                        "ChartPreviewFile",
                        "NoteField"
                    )
                    if realnotefieldpreview ~= nil then
                        self:SetTarget(realnotefieldpreview)
                        self:addx(-realnotefieldpreview:GetX())
                    else
                        print("It appears that chart preview is not where it should be ....")
                    end
                end,
            },
            Def.Quad {
                Name = "MouseWheelRegion",
                InitCommand = function(self)
                    self:diffusealpha(0.4)
                    -- the sizing here should make everything left of the wheel a mousewheel region
                    -- and also just a bit above and below it
                    -- and also the empty region to the right
                    -- the wheel positioning is not as clear as it could be
                    self:halign(0)
                    self:valign(0)
                    self:playcommand("SetPosition")
                    self:zoomto(actuals.GeneralBoxLeftGap, actuals.Height)
                end,
                SetPositionCommand = function(self)
                    if getWheelPosition() then
                        self:halign(0)
                        self:x(0)
                    else
                        self:halign(1)
                        self:x(actuals.LeftWidth)
                    end
                end,
                UpdateWheelPositionCommand = function(self)
                    self:playcommand("SetPosition")
                end,
                MouseScrollMessageCommand = function(self, params)
                    if isOver(self) and SCUFF.showingPreview then
                        if params.direction == "Up" then
                            SCREENMAN:GetTopScreen():GetChild("WheelFile"):playcommand("Move", {direction = -1})
                        else
                            SCREENMAN:GetTopScreen():GetChild("WheelFile"):playcommand("Move", {direction = 1})
                        end
                    end
                end,
                MouseClickPressMessageCommand = function(self, params)
                    if params ~= nil and params.button ~= nil and SCUFF.showingPreview then
                        if params.button == "DeviceButton_right mouse button" then
                            if isOver(self) then
                                SCREENMAN:GetTopScreen():PauseSampleMusic()
                            end
                        end
                    end
                end
            },
        }
        return t
    end

    -- includes color modifying and preset picking
    -- while the code may work it contains incredibly bad practice (for one, dont spam messageman broadcasts)
    local function createColorConfigPage()
        local saturationOverlay = nil
        local colorPickPosition = nil
        local saturationSliderPos = nil
        local alphaSliderPos = nil
        local textCursorPos = 1
        local boxSize = math.min(actuals.ColorBoxHeight, actuals.ColorBoxWidth)
        local sliderWidth = boxSize / 10
        local textLineSeparation = boxSize / 8 -- basically the y position of the bottom of each line
        local widthOfTheRightSide = actuals.LeftWidth - (boxSize + actuals.EdgePadding * 2 + sliderWidth) - actuals.EdgePadding * 2
        local halfWayInTheMiddleOfTheRightSide = actuals.LeftWidth - widthOfTheRightSide/2

        -- probably make this an odd number for the ocd kids because this includes a top item which does not ever change
        local colorConfigItemCount = 13
        -- lost track of what this means dont care
        -- basically if this is true and you press enter you save
        -- and if not and you press enter, the change is applied then you have to hit enter again
        local aboutToSave = false

        -- stores the data for items to display
        -- should be a list of strings
        -- starting it off with categories because categories is the starting state
        local displayItemDatas = getColorConfigCategories()
        local page = 1
        local maxPage = math.ceil(#displayItemDatas / (colorConfigItemCount-1))
        local cursorPosition = 1

        -- selected and saved element colors and HSV info (defaulted to white)
        local currentColor = color("1,1,1,1")
        local savedColor = currentColor
        local hueNum, satNum, valNum, alphaNum = colorToHSVNums(currentColor)

        -- determines the current state of color config selection
        -- valid options:
        --  category - currently selecting a color config element category
        --  element - currently selecting an element in a selected category
        --  preset - currently selecting a color config preset
        --  editing - currently editing an element color
        local selectionstate = "category"
        local selectedcategory = ""
        local selectedpreset = getColorPreset()
        local selectedelement = ""
        local hexEntryString = ""

        -- switch this variable (here, not at runtime) to display and allow editing alpha
        local showAlpha = false
        local hexStringMaxLengthWithAlpha = 9
        local hexStringMaxLengthWithoutAlpha = 7
        local hexStringMaxLength = showAlpha and hexStringMaxLengthWithAlpha or hexStringMaxLengthWithoutAlpha

        -- move choice pages
        local function movePage(n)
            if maxPage <= 1 then
                return
            end
            -- the tooltip gets stuck on if it is visible and page changes
            TOOLTIP:Hide()
            -- math to make pages loop both directions
            local nn = (page + n) % (maxPage + 1)
            if nn == 0 then
                nn = n > 0 and 1 or maxPage
            end
            page = nn
            MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
        end
        -- move choice selection cursor and also maybe move pages
        local function moveChoiceCursor(n)
            -- math to make pages loop both directions
            local newpos = cursorPosition + n
            if newpos > #displayItemDatas then
                newpos = 1
                cursorPosition = newpos
                if page ~= 1 then
                    page = 1
                    MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
                end
            elseif newpos < 1 then
                newpos = #displayItemDatas
                cursorPosition = newpos
                if maxPage ~= page then
                    page = maxPage
                    MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
                end
            else
                cursorPosition = newpos
                local lb = clamp((page-1) * (colorConfigItemCount-1) + 1, 0, #displayItemDatas)
                local ub = clamp(page * colorConfigItemCount-1, 0, #displayItemDatas)
                if cursorPosition < lb then
                    page = page - 1
                    MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
                elseif cursorPosition > ub then
                    page = page + 1
                    MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
                end
            end
            MESSAGEMAN:Broadcast("UpdateColorConfigChoiceCursorDisplay")
        end

        -- apply the HSV+A vars to the current state of the config
        -- updates the elements which display the information about the color
        local function applyHSV()
            local newColor = HSV(hueNum, 1 - satNum, 1 - valNum)
            newColor[4] = alphaNum
            currentColor = newColor

            -- the color information Actors may not be present for various reasons
            if colorPickPosition ~= nil then
                colorPickPosition:xy(boxSize * hueNum/360, boxSize * valNum)
            end
            if saturationOverlay ~= nil then
                saturationOverlay:diffusealpha(satNum)
            end
            if saturationSliderPos ~= nil then
                saturationSliderPos:y(boxSize * satNum)
            end
            if alphaSliderPos ~= nil then
                alphaSliderPos:y(boxSize * (1-alphaNum))
            end

            textCursorPos = hexStringMaxLength
            hexEntryString = "#" .. ColorToHex(currentColor)
            hexEntryString = hexEntryString:sub(1,hexStringMaxLength)

            MESSAGEMAN:Broadcast("ClickedNewColor")
        end
        local function updateSaturation(percent)
            if percent < 0 then percent = 0 elseif percent > 1 then percent = 1 end
            satNum = percent
            applyHSV()
        end
        local function updateAlpha(percent)
            if percent < 0 then percent = 0 elseif percent > 1 then percent = 1 end
            alphaNum = 1 - percent
            applyHSV()
        end
        local function updateColor(percentX, percentY)
            if percentY < 0 then percentY = 0 elseif percentY > 1 then percentY = 1 end
            if percentX < 0 then percentX = 0 elseif percentX > 1 then percentX = 1 end
            -- not 360 because 360 makes it produce FF00FF instead of FF0000
            hueNum = 359.99 * percentX
            valNum = percentY
            applyHSV()
        end

        -- handling keyboard inputs for hex characters only - str:match("[%x]")
        local function handleHexEntry(character)
            character = character:upper()
            if #hexEntryString <= hexStringMaxLength then -- #23 45 67 89 format
                if #hexEntryString == hexStringMaxLength and textCursorPos == hexStringMaxLength then
                    hexEntryString = hexEntryString:sub(1,-2) .. character
                else
                    if textCursorPos == #hexEntryString + 1 then
                        hexEntryString = hexEntryString .. character
                    else
                        local left = hexEntryString:sub(1,textCursorPos-1)
                        local right = hexEntryString:sub(textCursorPos+1)
                        hexEntryString = left .. character .. right
                    end
                    textCursorPos = textCursorPos + 1
                end
            end
            if textCursorPos > hexStringMaxLength then textCursorPos = hexStringMaxLength end
            aboutToSave = false
            MESSAGEMAN:Broadcast("UpdateStringDisplay")
        end

        -- preparing to save via pressing Enter
        local function handleTextUpdate()
            local hxl = #hexEntryString - 1
            local finalcolor = color("1,1,1,1")
            if hxl == 3 or hxl == 4 or hxl == 5 then -- color 3/4/5 hex
                finalcolor[1] = tonumber("0x"..hexEntryString:sub(2,2)) / 15
                finalcolor[2] = tonumber("0x"..hexEntryString:sub(3,3)) / 15
                finalcolor[3] = tonumber("0x"..hexEntryString:sub(4,4)) / 15
                if hxl == 4 then finalcolor[4] = tonumber("0x"..hexEntryString:sub(5,5)) / 15 end
                if hxl == 5 then finalcolor[4] = tonumber("0x"..hexEntryString:sub(5,6)) / 255 end
            elseif hxl == 6 or hxl == 7 or hxl == 8 then -- color 6/7/8 hex
                finalcolor[1] = tonumber("0x"..hexEntryString:sub(2,3)) / 255
                finalcolor[2] = tonumber("0x"..hexEntryString:sub(4,5)) / 255
                finalcolor[3] = tonumber("0x"..hexEntryString:sub(6,7)) / 255
                if hxl == 7 then finalcolor[4] = tonumber("0x"..hexEntryString:sub(7,7)) / 15 end
                if hxl == 8 then finalcolor[4] = tonumber("0x"..hexEntryString:sub(8,9)) / 255 end
            else
                return
            end
            local bh, bs, bv, ba = hueNum, satNum, valNum, alphaNum
            hueNum, satNum, valNum, alphaNum = colorToHSVNums(finalcolor)
            -- check to see if the color changed
            if bh ~= hueNum or bs ~= satNum or valNum ~= bv or alphaNum ~= ba then
                aboutToSave = true
            else
                aboutToSave = false
            end
            applyHSV()
        end

        ------=== magical text cursor functions (fun fact utf-16 probably breaks this hard copy pasters beware)
        -- find the x position for a char in text relative to text left edge
        local function getXPositionInText(self, index)
            local tlChar1 = self:getGlyphRect(1) -- top left vertex of first char in text
            local tlCharIndex = self:getGlyphRect(index) -- top left of char at text
            -- the [1] index is the x coordinate of the vertex
            if tlCharIndex and tlChar1 then
                local theX = tlCharIndex[1] - tlChar1[1]
                return theX * self:GetZoom()
            else
                return 0
            end
        end
        local function getWidthOfChar(self, index)
            local tl, bl, tr, br = self:getGlyphRect(index) -- topleft/bottomleft/topright/bottomright coord tables
            if tr and br then
                local glyphWidth = tr[1] - bl[1]
                return glyphWidth * self:GetZoom() * 0.95 -- slightly smaller than needed because it was too big
            else
                return 1
            end
        end
        local function cursorCanMove(speed)
            local maxTextSize = (#hexEntryString == hexStringMaxLength and hexStringMaxLength or #hexEntryString + 1)
            local tmpCursor = textCursorPos + speed
            if tmpCursor > maxTextSize or tmpCursor < 2 then
                return 0
            end
            return speed
        end
        ------===

        -- just revert the color back to the saved color
        local function undoChanges()
            aboutToSave = false
            currentColor = savedColor
            hueNum, satNum, valNum, alphaNum = colorToHSVNums(currentColor)
            applyHSV()
        end
        -- revert current color to original default color, do not save
        local function resetToDefault()
            if selectedcategory == "" or selectedelement == "" then return end
            aboutToSave = true
            currentColor = getDefaultColor(selectedcategory, selectedelement)
            hueNum, satNum, valNum, alphaNum = colorToHSVNums(currentColor)
            applyHSV()
        end
        -- revert color state to blank
        local function resetToBlank()
            aboutToSave = false -- this shouldnt be used when color is editable
            currentColor = color("1,1,1,1")
            savedColor = currentColor
            hueNum, satNum, valNum, alphaNum = colorToHSVNums(currentColor)
            applyHSV()
        end
        -- save changes
        local function saveColor()
            if selectedpreset == "" or selectedcategory == "" or selectedelement == "" then return end
            aboutToSave = false
            savedColor = currentColor
            COLORS:saveColor(selectedcategory, selectedelement, hexEntryString)
            COLORS:saveColorPreset(selectedpreset)
            applyHSV()
            MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
        end

        -- handle switching states and stuff
        local function switchSelectionState(cat)
            if cat == nil then cat = "" end
            cat = cat:lower()
            if cat == "category" then
                -- populate with all the categories
                displayItemDatas = getColorConfigCategories()
                selectedcategory = ""
                selectedelement = ""
                resetToBlank()
            elseif cat == "element" then
                -- populate with all elements in the selected category
                displayItemDatas = getColorConfigElementsForCategory(selectedcategory)
                selectedelement = ""
                resetToBlank()
            elseif cat == "preset" then
                -- populate with all available presets
                displayItemDatas = getColorConfigPresets()
                selectedpreset = ""
                selectedcategory = ""
                selectedelement = ""
                resetToBlank()
            elseif cat == "editing" then
                -- dont change listing, but change state to allow color editing
                currentColor = COLORS:getColor(selectedcategory, selectedelement)
                savedColor = currentColor
                hueNum, satNum, valNum, alphaNum = colorToHSVNums(currentColor)
                aboutToSave = false
                applyHSV()
            else
                return
            end
            maxPage = math.ceil(#displayItemDatas / (colorConfigItemCount-1))
            page = 1
            cursorPosition = 1
            selectionstate = cat
            MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
        end

        local function selectCategory(category)
            -- selecting a category brings you to element selection state
            selectedcategory = category
            switchSelectionState("element")
        end
        local function selectElement(element)
            -- selecting an element begins editing of the element color
            selectedelement = element
            switchSelectionState("editing")
        end
        local function selectPreset(preset)
            -- selecting a preset brings you to category select
            -- it also loads the preset globally
            selectedpreset = preset
            changeCurrentColorPreset(preset)
            switchSelectionState("category")
        end
        local function looking4preset()
            -- looking at the list of presets to load
            switchSelectionState("preset")
        end

        local function goUpOneLayer()
            if selectionstate == "category" then
                switchSelectionState("preset")
            elseif selectionstate == "element" then
                switchSelectionState("category")
            elseif selectionstate == "preset" then
                -- impossible
            elseif selectionstate == "editing" then
                switchSelectionState("category")
            else
                return
            end
        end

        -- typing window for making a new preset
        local function newPresetDialogue()
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

            local function f(answer)
                -- success:
                -- blank color info, jump to preset select page
                resetToBlank()
                if COLORS:loadColorPreset(answer:lower()) then
                    selectPreset(answer:lower())
                else
                    looking4preset()
                end
                on()
            end
            local question = "NEW COLOR CONFIG PRESET\nPlease enter a new preset name."
            askForInputStringWithFunction(
                question,
                128,
                false,
                f,
                function(answer)
                    local result = answer ~= nil and answer:gsub("^%s*(.-)%s*$", "%1") ~= "" and not answer:match("::") and answer:gsub("^%s*(.-)%s*$", "%1"):sub(-1) ~= ":"
                    local presets = getColorConfigPresets()
                    -- no dupes
                    for _, n in ipairs(presets) do
                        if n:lower() == answer:lower() then
                            result = false
                            break
                        end
                    end
                    -- so far we can attempt to make the color config entry
                    if result then
                        result = COLORS:newColorPreset(answer:lower())
                        if not result then
                            SCREENMAN:GetTopScreen():GetChild("Question"):settext(question .. "\nThere was an issue creating the new color config preset. You may try again.\nTo exit, press Esc.")
                            return false, "Response invalid."
                        else
                            return true, "Response invalid." -- the 2nd param doesnt matter here
                        end
                    else
                        SCREENMAN:GetTopScreen():GetChild("Question"):settext(question .. "\nDo not leave this space blank. Do not use illegal characters.\nTo exit, press Esc.")
                        return false, "Response invalid."
                    end
                end,
                function()
                    -- upon exit, do nothing
                    on()
                end
            )
        end

        local t = Def.ActorFrame {
            Name = "ColorConfigPageContainer",
            ShowLeftCommand = function(self, params)
                if params and params.name == "Color Config" then
                    self:diffusealpha(1)
                    self:z(1)
                    SCUFF.showingColor = true
                    CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "ColorConfig")
                else
                    self:playcommand("HideLeft")
                end
            end,
            HideLeftCommand = function(self)
                self:diffusealpha(0)
                self:z(-1)
                SCUFF.showingColor = false
            end,
            BeginCommand = function(self)
                local snm = SCREENMAN:GetTopScreen():GetName()
                local anm = self:GetName()

                -- cursor input management for color config
                -- noteskin display is not relevant for this, just contains it for reasons
                CONTEXTMAN:RegisterToContextSet(snm, "ColorConfig", anm)
                CONTEXTMAN:ToggleContextSet(snm, "ColorConfig", false)
                SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                    -- if locked out, dont allow
                    if not CONTEXTMAN:CheckContextSet(snm, "ColorConfig") then return end
                    if event.type ~= "InputEventType_Release" then -- allow Repeat and FirstPress
                        local gameButton = event.button
                        local key = event.DeviceInput.button
                        local letter = event.char
                        local up = gameButton == "Up" or gameButton == "MenuUp"
                        local down = gameButton == "Down" or gameButton == "MenuDown"
                        local right = gameButton == "MenuRight" or gameButton == "Right"
                        local left = gameButton == "MenuLeft" or gameButton == "Left"
                        local enter = gameButton == "Start"
                        local ctrl = INPUTFILTER:IsBeingPressed("left ctrl") or INPUTFILTER:IsBeingPressed("right ctrl")
                        local alt = INPUTFILTER:IsBeingPressed("right alt") or INPUTFILTER:IsBeingPressed("left alt")
                        local back = key == "DeviceButton_escape"
                        local delete = key == "DeviceButton_delete"
                        local backspace = key == "DeviceButton_backspace"
                        local rightclick = key == "DeviceButton_right mouse button"
                        local leftclick = key == "DeviceButton_left mouse button"

                        if back then
                            if selectionstate == "editing" then
                                -- pressing back while editing moves back to element seletion
                                switchSelectionState("element")
                            else
                                -- shortcut to exit back to settings
                                -- press twice to exit back to general
                                MESSAGEMAN:Broadcast("PlayerInfoFrameTabSet", {tab = "Settings"})
                            end
                        elseif selectionstate == "editing" then
                            -- editing a color, typing only on the color
                            if letter and letter:match('[%x]') then
                                -- match all hex for inputting color
                                handleHexEntry(letter)
                            elseif delete then
                                -- pressed delete
                                if ctrl then
                                    -- holding ctrl, "pressed reset to default"
                                    resetToDefault()
                                elseif alt then
                                    -- holding alt, "pressed undo"
                                    undoChanges()
                                else
                                    hexEntryString = "#"
                                    textCursorPos = 2
                                    aboutToSave = false
                                end
                                MESSAGEMAN:Broadcast("UpdateStringDisplay")
                            elseif backspace then
                                if #hexEntryString > 1 then
                                    if textCursorPos - 1 == #hexEntryString then
                                        hexEntryString = hexEntryString:sub(1, -2)
                                    else
                                        local left = hexEntryString:sub(1, textCursorPos - 1)
                                        local right = hexEntryString:sub(textCursorPos + 1)
                                        hexEntryString = left .. "0" .. right
                                    end
                                    textCursorPos = textCursorPos + cursorCanMove(-1)
                                    aboutToSave = false
                                    MESSAGEMAN:Broadcast("UpdateStringDisplay")
                                end
                            elseif left then
                                local before = textCursorPos
                                textCursorPos = textCursorPos + cursorCanMove(-1)
                                if before ~= textCursorPos then
                                    MESSAGEMAN:Broadcast("UpdateStringDisplay")
                                end
                            elseif right then
                                local before = textCursorPos
                                textCursorPos = textCursorPos + cursorCanMove(1)
                                if before ~= textCursorPos then
                                    MESSAGEMAN:Broadcast("UpdateStringDisplay")
                                end
                            elseif enter then
                                if aboutToSave then
                                    saveColor()
                                else
                                    handleTextUpdate()
                                end
                            end
                        elseif selectionstate ~= "editing" then
                            -- all cursor movement
                            if left or up then
                                moveChoiceCursor(-1)
                            elseif right or down then
                                moveChoiceCursor(1)
                            elseif enter then
                                local itemData = displayItemDatas[cursorPosition]
                                if selectionstate == "preset" then
                                    -- clicked a preset, switching to category
                                    selectPreset(itemData)
                                elseif selectionstate == "category" then
                                    -- clicked a category, switching to element
                                    selectCategory(itemData)
                                elseif selectionstate == "element" then
                                    -- clicked an element, switching to edit mode
                                    selectElement(itemData)
                                end
                            elseif backspace then
                                -- go up one layer
                                goUpOneLayer()
                            elseif key and key == "DeviceButton_n" and ctrl then
                                -- ctrl-n makes a new preset
                                newPresetDialogue()
                            end
                        else
                            -- nothing happens
                            return
                        end
                    end
                end)

                -- hack to set up all default values
                applyHSV()
                MESSAGEMAN:Broadcast("ColorConfigSelectionStateChanged")
                MESSAGEMAN:Broadcast("UpdateColorConfigChoiceCursorDisplay")
            end,

            Def.ActorFrame {
                Name = "TopColorStuff",
                InitCommand = function(self)
                    self:xy(actuals.EdgePadding, actuals.TopLipHeight * 2)
                end,
                Def.Sprite {
                    Name = "HSVImage",
                    Texture = THEME:GetPathG("", "color_hsv"),
                    InitCommand = function(self)
                        self:halign(0):valign(0)
                        self:zoomto(boxSize, boxSize)
                    end,
                },
                UIElements.SpriteButton(1, 0, THEME:GetPathG("", "color_sat_overlay")) .. {
                    Name = "SaturationOverlay",
                    InitCommand = function(self)
                        saturationOverlay = self
                        self:halign(0):valign(0)
                        self:zoomto(boxSize, boxSize)
                        self:diffusealpha(satNum)
                    end,
                    InvokeCommand = function(self, params)
                        if not focused or not SCUFF.showingColor or selectionstate ~= "editing" then return end
                        -- normally local x and y is provided but something looks broken and im not fixing it
                        -- (has to do with button roots and nonsense)
                        local relX, relY = self:GetLocalMousePos(INPUTFILTER:GetMouseX(), INPUTFILTER:GetMouseY(), 0)
                        aboutToSave = true
                        updateColor(relX / boxSize, relY / boxSize)
                    end,
                    -- did not add hover functions because we want to be color correct
                    MouseDownCommand = function(self, params)
                        self:playcommand("Invoke", params)
                    end,
                    MouseDragCommand = function(self, params)
                        self:playcommand("Invoke", params)
                    end,
                },
                Def.ActorFrame {
                    Name = "SaturationSliderFrame",
                    InitCommand = function(self)
                        self:x(boxSize + actuals.EdgePadding)
                    end,
                    UIElements.SpriteButton(1, 0, THEME:GetPathG("", "color_sat_gradient")) .. {
                        Name = "SaturationSlider",
                        InitCommand = function(self)
                            self:halign(0):valign(0)
                            self:zoomto(sliderWidth, boxSize)
                        end,
                        InvokeCommand = function(self, params)
                            if not focused or not SCUFF.showingColor or selectionstate ~= "editing" then return end
                            -- normally local x and y is provided but something looks broken and im not fixing it
                            -- (has to do with button roots and nonsense)
                            local relX, relY = self:GetLocalMousePos(INPUTFILTER:GetMouseX(), INPUTFILTER:GetMouseY(), 0)
                            aboutToSave = true
                            updateSaturation(relY / boxSize)
                        end,
                        MouseDownCommand = function(self, params)
                            self:playcommand("Invoke", params)
                        end,
                        MouseDragCommand = function(self, params)
                            self:playcommand("Invoke", params)
                        end,
                    },
                    Def.Sprite {
                        Name = "SaturationPointer",
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:x(sliderWidth + sliderWidth / 5 - 1)
                            self:zoomto(sliderWidth / 2, sliderWidth / 2)
                            self:rotationz(-90)
                            saturationSliderPos = self
                        end,
                    }
                },
                --[[-- We will not be including alpha control. Alpha is overridden by the Actors.
                    -- I'm nice enough to provide it here (didnt test)
                Def.ActorFrame {
                    Name = "AlphaSliderFrame",
                    InitCommand = function(self)
                        self:x(boxSize + actuals.EdgePadding * 2 + sliderWidth)
                    end,
                    UIElements.QuadButton(1, 0) .. {
                        Name = "AlphaSlider",
                        InitCommand = function(self)
                            self:halign(0):valign(0)
                            self:zoomto(sliderWidth, boxSize)
                            self:diffuse(color("#666666FF"))
                        end,
                        InvokeCommand = function(self, params)
                            if not focused or not SCUFF.showingColor then return end
                            -- normally local x and y is provided but something looks broken and im not fixing it
                            -- (has to do with button roots and nonsense)
                            local relX, relY = self:GetLocalMousePos(INPUTFILTER:GetMouseX(), INPUTFILTER:GetMouseY(), 0)
                            aboutToSave = true
                            updateAlpha(relY / boxSize)
                        end,
                        MouseDownCommand = function(self, params)
                            self:playcommand("Invoke", params)
                        end,
                        MouseDragCommand = function(self, params)
                            self:playcommand("Invoke", params)
                        end,
                    },
                    Def.Sprite {
                        Name = "AlphaPointer",
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:x(sliderWidth + sliderWidth / 5 - 1)
                            self:zoomto(sliderWidth / 2, sliderWidth / 2)
                            self:rotationz(-90)
                            alphaSliderPos = self
                        end,
                    }
                },
                ]]
                Def.Sprite {
                    Name = "ColorPickPosition",
                    Texture = THEME:GetPathG("", "_thick circle"),
                    InitCommand = function(self)
                        self:zoom(0.2)
                        colorPickPosition = self
                    end,
                },
                Def.ActorFrame {
                    Name = "TopRightSide",
                    InitCommand = function(self)
                        self:x(boxSize + actuals.EdgePadding * 2 + sliderWidth)
                    end,
                    LoadFont("Common Normal") .. {
                        Name = "CurrentPreset",
                        InitCommand = function(self)
                            self:halign(0):valign(1)
                            self:y(textLineSeparation * 1)
                            self:zoom(colorConfigTextSize)
                            self:maxwidth(widthOfTheRightSide / colorConfigChoiceTextSize - textZoomFudge)
                            self:settext("Current preset:")
                            registerActorToColorConfigElement(self, "main", "PrimaryText")
                        end,
                        SetCommand = function(self)
                            self:settextf("Current preset: %s", selectedpreset)
                        end,
                        ColorConfigSelectionStateChangedMessageCommand = function(self)
                            self:playcommand("Set")
                        end,
                    },
                    LoadFont("Common Normal") .. {
                        Name = "CurrentElement",
                        InitCommand = function(self)
                            self:halign(0):valign(1)
                            self:y(textLineSeparation * 2)
                            self:zoom(colorConfigTextSize)
                            self:maxwidth(widthOfTheRightSide / colorConfigChoiceTextSize - textZoomFudge)
                            self:settext("Current element:")
                            registerActorToColorConfigElement(self, "main", "PrimaryText")
                        end,
                        SetCommand = function(self)
                            self:settextf("Current element: %s", selectedelement)
                        end,
                        ColorConfigSelectionStateChangedMessageCommand = function(self)
                            self:playcommand("Set")
                        end,
                    },
                    LoadFont("Common Normal") .. {
                        Name = "CurrentColorTitle",
                        InitCommand = function(self)
                            self:halign(0):valign(1)
                            self:y(textLineSeparation * 3)
                            self:zoom(colorConfigTextSize)
                            self:maxwidth(widthOfTheRightSide/2 / colorConfigChoiceTextSize - textZoomFudge)
                            self:settext("Current color")
                            registerActorToColorConfigElement(self, "main", "PrimaryText")
                        end
                    },
                    Def.ActorFrame {
                        Name = "CurrentColorInputTextContainer",
                        InitCommand = function(self)
                            self:xy(widthOfTheRightSide, textLineSeparation * 3)
                        end,
                        LoadFont("Common Normal") .. {
                            Name = "CurrentColorInputText",
                            InitCommand = function(self)
                                self:halign(1):valign(1)
                                self:zoom(colorConfigTextSize)
                                self:maxwidth(widthOfTheRightSide/2 / colorConfigChoiceTextSize - textZoomFudge)
                                self:settext("#")
                                -- colored white always ?
                            end,
                            SetCommand = function(self)
                                self:settext(hexEntryString)
                                self:GetParent():GetChild("CurrentColorCursorPosition"):playcommand("UpdateCursorDisplay")
                                self:GetParent():GetParent():GetChild("CurrentColorPreview"):playcommand("UpdateColorDisplay")
                                self:GetParent():GetParent():GetChild("SaveChangesButton"):playcommand("Set")
                            end,
                            UpdateStringDisplayMessageCommand = function(self)
                                self:playcommand("Set")
                            end,
                            ClickedNewColorMessageCommand = function(self)
                                self:playcommand("Set")
                            end,
                        },
                        Def.Quad {
                            Name = "CurrentColorCursorPosition",
                            InitCommand = function(self)
                                self:halign(0):valign(0)
                                self:zoomto(0,0)
                                self:y(3)
                                registerActorToColorConfigElement(self, "main", "SeparationDivider")
                            end,
                            UpdateCursorDisplayCommand = function(self)
                                local pos = 11
                                local txt = self:GetParent():GetChild("CurrentColorInputText")
                                if textCursorPos ~= #hexEntryString + 1 then -- if the cursor is under an actual char
                                    local glyphWidth = getWidthOfChar(txt, textCursorPos) - 1
                                    self:zoomto(glyphWidth, 2)
                                    pos = getXPositionInText(txt, textCursorPos)
                                else
                                    pos = getXPositionInText(txt, textCursorPos-1) + getWidthOfChar(txt, textCursorPos-1)
                                end
                                self:finishtweening()
                                self:linear(0.05)
                                self:x(-txt:GetZoomedWidth() + pos)
                            end
                        },
                    },
                    Def.Quad {
                        Name = "CurrentColorPreview",
                        InitCommand = function(self)
                            self:halign(0)
                            self:y(textLineSeparation * 4)
                            self:zoomto(widthOfTheRightSide / 2 - actuals.EdgePadding/2, textLineSeparation)
                        end,
                        SetCommand = function(self)
                            self:diffuse(currentColor)
                        end,
                        UpdateColorDisplayCommand = function(self)
                            self:playcommand("Set")
                        end,
                    },
                    Def.Quad {
                        Name = "SavedColorPreview",
                        InitCommand = function(self)
                            self:halign(1)
                            self:xy(widthOfTheRightSide, textLineSeparation * 4)
                            self:zoomto(widthOfTheRightSide / 2 - actuals.EdgePadding/2, textLineSeparation)
                        end,
                        SetCommand = function(self)
                            self:diffuse(savedColor)
                        end,
                        UpdateColorDisplayCommand = function(self)
                            self:playcommand("Set")
                        end,
                        ColorConfigSelectionStateChangedMessageCommand = function(self)
                            self:playcommand("Set")
                        end,
                    },
                    UIElements.TextButton(1, 1, "Common Normal") .. {
                        Name = "UndoButton",
                        InitCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            txt:halign(0):valign(1)
                            bg:halign(0):valign(1)
                            txt:zoom(colorConfigTextSize)
                            txt:maxwidth(widthOfTheRightSide / 2 / colorConfigTextSize)
                            self:y(textLineSeparation * 6)
                            bg:y(1)
                            txt:settext("Undo")
                            registerActorToColorConfigElement(txt, "main", "PrimaryText")
                            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                            self.alphaDeterminingFunction = function(self)
                                local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                                local disabledmultiplier = selectionstate ~= "editing" and 0.3 or 1
                                self:diffusealpha(1 * hovermultiplier * disabledmultiplier)
                            end
                        end,
                        RolloverUpdateCommand = function(self, params)
                            if self:IsInvisible() or selectionstate ~= "editing" then return end
                            self:alphaDeterminingFunction()
                        end,
                        ClickCommand = function(self, params)
                            if self:IsInvisible() or selectionstate ~= "editing" then return end
                            if params.update == "OnMouseDown" then
                                undoChanges()
                                self:alphaDeterminingFunction()
                            end
                        end,
                        ColorConfigSelectionStateChangedMessageCommand = function(self)
                            self:alphaDeterminingFunction()
                        end,
                        ClickedNewColorMessageCommand = function(self)
                            self:alphaDeterminingFunction()
                        end,
                    },
                    UIElements.TextButton(1, 1, "Common Normal") .. {
                        Name = "ResetToDefaultButton",
                        InitCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            txt:halign(1):valign(1)
                            bg:halign(1):valign(1)
                            txt:zoom(colorConfigTextSize)
                            txt:maxwidth(widthOfTheRightSide / 2 / colorConfigTextSize)
                            self:x(widthOfTheRightSide)
                            self:y(textLineSeparation * 6)
                            bg:y(1)
                            txt:settext("Reset to Default")
                            registerActorToColorConfigElement(txt, "main", "PrimaryText")
                            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                            self.alphaDeterminingFunction = function(self)
                                local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                                local disabledmultiplier = selectionstate ~= "editing" and 0.3 or 1
                                self:diffusealpha(1 * hovermultiplier * disabledmultiplier)
                            end
                        end,
                        RolloverUpdateCommand = function(self, params)
                            if self:IsInvisible() or selectionstate ~= "editing" then return end
                            self:alphaDeterminingFunction()
                        end,
                        ClickCommand = function(self, params)
                            if self:IsInvisible() or selectionstate ~= "editing" then return end
                            if params.update == "OnMouseDown" then
                                resetToDefault()
                                self:alphaDeterminingFunction()
                            end
                        end,
                        ColorConfigSelectionStateChangedMessageCommand = function(self)
                            self:alphaDeterminingFunction()
                        end,
                        ClickedNewColorMessageCommand = function(self)
                            self:alphaDeterminingFunction()
                        end,
                    },
                    UIElements.TextButton(1, 1, "Common Normal") .. {
                        Name = "SaveChangesButton",
                        InitCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            txt:halign(0):valign(1)
                            bg:halign(0):valign(1)
                            txt:zoom(colorConfigTextSize)
                            txt:maxwidth(widthOfTheRightSide / colorConfigTextSize)
                            self:y(textLineSeparation * 7)
                            bg:y(1)
                            self:playcommand("Set")
                            registerActorToColorConfigElement(txt, "main", "PrimaryText")
                            self.alphaDeterminingFunction = function(self)
                                local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                                local disabledmultiplier = selectionstate ~= "editing" and 0.3 or 1
                                self:diffusealpha(1 * hovermultiplier * disabledmultiplier)
                            end
                        end,
                        SetCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            if aboutToSave then
                                txt:settext("Save Changes (Not Saved!)")
                            else
                                txt:settext("Save Changes")
                            end
                            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        end,
                        RolloverUpdateCommand = function(self, params)
                            if self:IsInvisible() or selectionstate ~= "editing" then return end
                            self:alphaDeterminingFunction()
                        end,
                        ClickCommand = function(self, params)
                            if self:IsInvisible() or selectionstate ~= "editing" then return end
                            if params.update == "OnMouseDown" then
                                saveColor()
                                self:alphaDeterminingFunction()
                            end
                        end,
                        ColorConfigSelectionStateChangedMessageCommand = function(self)
                            self:alphaDeterminingFunction()
                        end,
                        ClickedNewColorMessageCommand = function(self)
                            self:alphaDeterminingFunction()
                        end,
                    },
                    UIElements.TextButton(1, 1, "Common Normal") .. {
                        Name = "NewPreset",
                        InitCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            txt:halign(0):valign(1)
                            bg:halign(0):valign(1)
                            self:y(textLineSeparation * 8)
                            bg:y(1)
                            txt:zoom(colorConfigTextSize)
                            txt:maxwidth(widthOfTheRightSide / colorConfigChoiceTextSize - textZoomFudge)
                            txt:settext("New Color Config Preset")
                            registerActorToColorConfigElement(txt, "main", "PrimaryText")
                            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                            self.alphaDeterminingFunction = function(self)
                                local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                                self:diffusealpha(1 * hovermultiplier)
                            end
                        end,
                        RolloverUpdateCommand = function(self, params)
                            if self:IsInvisible() then return end
                            self:alphaDeterminingFunction()
                        end,
                        ClickCommand = function(self, params)
                            if self:IsInvisible() then return end
                            if params.update == "OnMouseDown" then
                                newPresetDialogue()
                                self:alphaDeterminingFunction()
                            end
                        end,
                    },
                },
            },
        }

        local function colorConfigChoices()
            local frameY = actuals.TopLipHeight*2 + boxSize + actuals.EdgePadding
            local remainingYSpace = actuals.Height - frameY - actuals.EdgePadding

            local t = Def.ActorFrame {
                Name = "ColorConfigItemChoiceFrame",
                InitCommand = function(self)
                    self:xy(actuals.EdgePadding, frameY)
                end,

                LoadFont("Common Normal") .. {
                    Name = "ColorConfigTopItemChoice",
                    InitCommand = function(self)
                        self:halign(0)
                        self:zoom(colorConfigChoiceTextSize)
                        self:maxwidth((actuals.LeftWidth - actuals.EdgePadding*2) * 0.7 / colorConfigTextSize - textZoomFudge)
                        self:y(remainingYSpace / colorConfigItemCount / 2)
                        self:settext(" ")
                        registerActorToColorConfigElement(self, "main", "PrimaryText")
                    end,
                    UpdateColorConfigDisplayCommand = function(self)
                        if selectionstate == "category" then
                            self:settext("Browsing Color Categories")
                        elseif selectionstate == "preset" then
                            self:settext("Browsing Color Config Presets")
                        elseif selectionstate == "element" then
                            self:settext("Browsing Elements in '"..selectedcategory.."'")
                        elseif selectionstate == "editing" then
                            self:settext("Browsing Elements in '"..selectedcategory.."' (editing)")
                        end
                    end,
                    ColorConfigSelectionStateChangedMessageCommand = function(self)
                        self:playcommand("UpdateColorConfigDisplay")
                    end,
                },
                LoadFont("Common Normal") .. {
                    Name = "PageNumber",
                    InitCommand = function(self)
                        self:halign(1)
                        self:zoom(colorConfigChoiceTextSize)
                        self:maxwidth((actuals.LeftWidth - actuals.EdgePadding*2) * 0.2 / colorConfigTextSize - textZoomFudge)
                        self:x(actuals.LeftWidth - actuals.EdgePadding*2)
                        self:y(remainingYSpace - remainingYSpace / colorConfigItemCount / 2)
                        self:settext(" ")
                        registerActorToColorConfigElement(self, "main", "PrimaryText")
                    end,
                    UpdateColorConfigDisplayCommand = function(self)
                        local lb = clamp((page-1) * (colorConfigItemCount-1) + 1, 0, #displayItemDatas)
                        local ub = clamp(page * colorConfigItemCount-1, 0, #displayItemDatas)
                        self:settextf("%d-%d/%d", lb, ub, #displayItemDatas)
                    end,
                    ColorConfigSelectionStateChangedMessageCommand = function(self)
                        self:playcommand("UpdateColorConfigDisplay")
                    end,
                },
                Def.Quad {
                    Name = "MouseWheelRegion",
                    InitCommand = function(self)
                        self:halign(0):valign(0)
                        self:zoomto(actuals.LeftWidth - actuals.EdgePadding*2, remainingYSpace)
                        self:diffusealpha(0)
                    end,
                    MouseScrollMessageCommand = function(self, params)
                        if isOver(self) and SCUFF.showingColor then
                            if params.direction == "Up" then
                                movePage(-1)
                            else
                                movePage(1)
                            end
                        end
                    end,
                },
                UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "BackButton",
                    InitCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        txt:halign(1)
                        bg:halign(1)
                        txt:zoom(colorConfigChoiceTextSize)
                        txt:maxwidth((actuals.LeftWidth - actuals.EdgePadding*2) * 0.3 / colorConfigTextSize - textZoomFudge)
                        self:x(actuals.LeftWidth - actuals.EdgePadding*2)
                        self:y(remainingYSpace / colorConfigItemCount / 2)
                        txt:settext(" ")
                        registerActorToColorConfigElement(txt, "main", "PrimaryText")
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        self.alphaDeterminingFunction = function(self)
                            local statemultiplier = selectionstate ~= "preset" and 1 or 0
                            local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                            self:diffusealpha(1 * statemultiplier * hovermultiplier)
                        end
                    end,
                    UpdateColorConfigDisplayCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        if selectionstate == "category" then
                            txt:settext("Back to Presets")
                        elseif selectionstate == "preset" then
                            txt:settext(" ")
                        elseif selectionstate == "element" then
                            txt:settext("Back to Categories")
                        elseif selectionstate == "editing" then
                            txt:settext("Back to Categories")
                        end
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        self:alphaDeterminingFunction()
                    end,
                    ColorConfigSelectionStateChangedMessageCommand = function(self)
                        self:playcommand("UpdateColorConfigDisplay")
                    end,
                    RolloverUpdateCommand = function(self, params)
                        self:alphaDeterminingFunction()
                    end,
                    ClickCommand = function(self, params)
                        if params.update == "OnMouseDown" then
                            goUpOneLayer()
                            self:alphaDeterminingFunction()
                        end
                    end,
                }
            }

            local function colorConfigChoice(i)
                local index = i
                -- leaving 0.2 here for the page number
                local itemWidth = (actuals.LeftWidth - actuals.EdgePadding*2) * 0.6
                local itemColorPreviewWith = (actuals.LeftWidth - actuals.EdgePadding*2) * 0.2
                local itemData = nil

                local t = UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "ColorConfigItemChoice",
                    InitCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        -- position the item half way into the nth position like how we do horizontal choice positioning
                        -- then dont vertically align anything so it Just Works
                        self:y((remainingYSpace / colorConfigItemCount) * i + (remainingYSpace / colorConfigItemCount / 2))
                        txt:halign(0)
                        bg:halign(0)
                        txt:zoom(colorConfigChoiceTextSize)
                        txt:maxwidth(itemWidth / colorConfigChoiceTextSize - textZoomFudge)
                        txt:settext(" ")
                        registerActorToColorConfigElement(txt, "main", "SecondaryText")
                        registerActorToColorConfigElement(bg, "options", "Cursor")
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        self.alphaDeterminingFunction = function(self)
                            local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                            local visiblemultiplier = itemData == nil and 0 or 1
                            self:diffusealpha(1 * hovermultiplier * visiblemultiplier)
                        end
                    end,
                    UpdateColorConfigDisplayCommand = function(self)
                        index = (page - 1) * (colorConfigItemCount-1) + i
                        itemData = displayItemDatas[index]
                        self:finishtweening()
                        self:diffusealpha(0)
                        self:GetChild("ElementPreview"):playcommand("UpdateElementPreview")
                        self:playcommand("UpdateCursorDisplay")
                        if itemData ~= nil then
                            self:playcommand("UpdateText")
                            self:smooth(itemListAnimationSeconds * i)
                            self:alphaDeterminingFunction()
                        end
                    end,
                    UpdateTextCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        local txtstr = itemData or ""
                        txt:settextf("%d.  %s", index, txtstr)
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    end,
                    UpdateColorConfigChoiceCursorDisplayMessageCommand = function(self)
                        local bg = self:GetChild("BG")
                        if itemData ~= nil then
                            if cursorPosition == index then
                                bg:diffusealpha(0.2)
                            else
                                bg:diffusealpha(0)
                            end
                        else
                            bg:diffusealpha(0)
                        end
                    end,
                    ColorConfigSelectionStateChangedMessageCommand = function(self)
                        self:playcommand("UpdateColorConfigDisplay")
                    end,
                    RolloverUpdateCommand = function(self, params)
                        if self:IsInvisible() then return end
                        self:alphaDeterminingFunction()
                    end,
                    ClickCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "OnMouseDown" then
                            if selectionstate == "preset" then
                                -- clicked a preset, switching to category
                                selectPreset(itemData)
                            elseif selectionstate == "category" then
                                -- clicked a category, switching to element
                                selectCategory(itemData)
                            elseif selectionstate == "element" or selectionstate == "editing" then
                                -- clicked an element, switching to edit mode
                                selectElement(itemData)
                            end
                            self:alphaDeterminingFunction()
                        end
                    end
                }
                t[#t+1] = Def.Quad {
                    Name = "ElementPreview",
                    InitCommand = function(self)
                        self:halign(0)
                        self:x(itemWidth)
                        self:zoomto(itemColorPreviewWith, remainingYSpace / colorConfigItemCount * 0.6)
                    end,
                    UpdateElementPreviewCommand = function(self)
                        if itemData == nil or itemData == "" or (selectionstate ~= "editing" and selectionstate ~= "element") then
                            self:visible(false)
                            return
                        else
                            self:visible(true)
                            self:diffuse(COLORS:getColor(selectedcategory, itemData))
                        end
                    end,
                }
                return t
            end
            for i = 1, colorConfigItemCount-1 do
                t[#t+1] = colorConfigChoice(i)
            end
            return t
        end
        t[#t+1] = colorConfigChoices()
        return t
    end

    t[#t+1] = createNoteskinPage()
    t[#t+1] = createPreviewPage()
    t[#t+1] = createColorConfigPage()

    return t
end

local function rightFrame()
    -- to reach the explanation text from anywhere without all the noise
    local explanationHandle = nil
    local offscreenX = SCREEN_WIDTH
    local onscreenX = SCREEN_WIDTH - actuals.RightWidth

    local t = Def.ActorFrame {
        Name = "RightFrame",
        InitCommand = function(self)
            self:playcommand("SetPosition")
            self:diffusealpha(0)
        end,
        HideRightCommand = function(self)
            -- move off screen right and go invisible
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(0)
            self:x(offscreenX)
        end,
        ShowRightCommand = function(self)
            -- move on screen from right and go visible
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(1)
            self:x(onscreenX)
        end,
        SetPositionCommand = function(self)
            if getWheelPosition() then
                onscreenX = SCREEN_WIDTH - actuals.RightWidth
                offscreenX = SCREEN_WIDTH
            else
                onscreenX = 0
                offscreenX = -actuals.RightWidth
            end
            if focused then
                self:x(onscreenX)
            else
                self:x(offscreenX)
            end
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(actuals.RightWidth, actuals.Height)
                self:diffusealpha(0.6)
                registerActorToColorConfigElement(self, "main", "PrimaryBackground")
            end,
        },
        Def.Quad {
            Name = "TopLip",
            InitCommand = function(self)
                -- height is double normal top lip
                self:valign(0):halign(0)
                self:zoomto(actuals.RightWidth, actuals.TopLipHeight * 2)
                self:diffusealpha(0.6)
                registerActorToColorConfigElement(self, "main", "SecondaryBackground")
            end,
        },
        Def.Quad {
            Name = "BottomLip",
            InitCommand = function(self)
                -- height is double normal top lip
                self:valign(1):halign(0)
                self:y(actuals.Height)
                self:zoomto(actuals.RightWidth, actuals.BottomLipHeight)
                self:diffusealpha(0.6)
                registerActorToColorConfigElement(self, "main", "SecondaryBackground")
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "HeaderText",
            InitCommand = function(self)
                self:halign(0)
                self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
                self:zoom(titleTextSize)
                self:maxwidth((actuals.RightWidth - actuals.EdgePadding*2) / titleTextSize - textZoomFudge)
                self:settext("Options")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end
        },
        LoadFont("Common Normal") .. {
            Name = "ExplanationText",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.EdgePadding, actuals.Height - actuals.BottomLipHeight + actuals.EdgePadding)
                self:zoom(explanationTextSize)
                --self:maxwidth((actuals.RightWidth - actuals.EdgePadding*2) / explanationTextSize - textZoomFudge)
                self:wrapwidthpixels((actuals.RightWidth - actuals.EdgePadding * 2) / explanationTextSize)
                self:maxheight((actuals.BottomLipHeight - actuals.EdgePadding * 2) / explanationTextSize)
                self:settext(" ")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
                explanationHandle = self
            end,
            SetExplanationCommand = function(self, params)
                if params and params.text and #params.text > 0 then
                    -- here we go ...
                    -- editors note 5 minutes later: i cant believe this works
                    -- this begins the explainloop below which will slowly write out the desired text
                    -- it fires a finishtweening when new text is queued here in case we are in the middle of looping
                    self.txt = params.text
                    self.pos = 0
                    self:finishtweening()
                    self:settext("")
                    self:queuecommand("_explainloop")
                else
                    self.txt = ""
                    self:settext("")
                end
            end,
            _explainloopCommand = function(self)
                self.pos = self.pos + 1
                local subtxt = self.txt:sub(1, self.pos)
                self:settext(subtxt)
                self:sleep(explanationTextWriteAnimationSeconds / #self.txt)
                if self.pos < #self.txt then
                    self:queuecommand("_explainloop")
                end
            end,
        },
        UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "PreviewToggle",
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                txt:halign(0)
                txt:zoom(previewButtonTextSize)
                txt:maxwidth(actuals.RightWidth / 2 / previewButtonTextSize - textZoomFudge)
                txt:settext("Toggle Chart Preview")
                registerActorToColorConfigElement(txt, "main", "SecondaryText")

                bg:halign(0)
                bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                bg:diffusealpha(0.2)
                registerActorToColorConfigElement(bg, "options", "Cursor")

                self:xy(actuals.EdgePadding, actuals.Height - actuals.BottomLipHeight - actuals.BottomLipHeight/4)
                -- is this being lazy or being big brained? ive stored a function within an actor instance
                self.alphaDeterminingFunction = function(self)
                    local isOpened = SCUFF.showingPreview
                    local canBeToggled = SCUFF.showingPreview or (not SCUFF.showingColor and not SCUFF.showingKeybinds and not SCUFF.showingNoteskins)
                    local alphamultiplier = (isOpened and canBeToggled) and previewOpenedAlpha or 1
                    local hovermultiplier = (isOver(bg) and canBeToggled) and buttonHoverAlpha or 1
                    local finalalpha = 1 * hovermultiplier * alphamultiplier
                    self:diffusealpha(finalalpha)
                end
            end,
            PreviewPageOpenStatusChangedMessageCommand = function(self, params)
                if self:IsInvisible() then return end
                if params and params.opened ~= nil then
                    self:alphaDeterminingFunction()
                end
            end,
            RolloverUpdateCommand = function(self, params)
                if self:IsInvisible() then return end
                self:alphaDeterminingFunction()
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    if not SCUFF.showingColor and not SCUFF.showingKeybinds and not SCUFF.showingNoteskins and not SCUFF.showingPreview then
                        MESSAGEMAN:Broadcast("ShowSettingsAlt", {name = "Preview"})
                    elseif SCUFF.showingPreview then
                        MESSAGEMAN:Broadcast("PlayerInfoFrameTabSet", {tab = "Settings"})
                    end
                end
            end,
        }
    }

    -- -----
    -- Utility functions for options not necessarily needed for global use in /Scripts (could easily be put there instead though)

    -- set any mod as part of PlayerOptions at all levels in one easy function
    local function setPlayerOptionsModValueAllLevels(funcname, ...)
        -- you give a funcname like MMod, XMod, CMod and it just works
        local poptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
        local stoptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Stage")
        local soptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Song")
        local coptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Current")
        poptions[funcname](poptions, ...)
        stoptions[funcname](stoptions, ...)
        soptions[funcname](soptions, ...)
        coptions[funcname](coptions, ...)
    end
    -- set any mod as part of SongOptions at all levels in one easy function
    local function setSongOptionsModValueAllLevels(funcname, ...)
        -- you give a funcname like MusicRate and it just works
        local poptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred")
        local stoptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Stage")
        local soptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Song")
        local coptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Current")
        poptions[funcname](poptions, ...)
        stoptions[funcname](stoptions, ...)
        soptions[funcname](soptions, ...)
        coptions[funcname](coptions, ...)
    end

    --- for Speed Mods -- this has been adapted from the fallback script which does speed and mode at once
    local function getSpeedModeFromPlayerOptions()
        local poptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
        if poptions:MaxScrollBPM() > 0 then
            return "M"
        elseif poptions:TimeSpacing() > 0 then
            return "C"
        else
            return "X"
        end
    end
    local function getSpeedValueFromPlayerOptions()
        local poptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
        if poptions:MaxScrollBPM() > 0 then
            return math.round(poptions:MaxScrollBPM())
        elseif poptions:TimeSpacing() > 0 then
            return math.round(poptions:ScrollBPM())
        else
            return math.round(poptions:ScrollSpeed() * 100)
        end
    end

    -- for convenience to generate a choice table for a float interface setting
    local function floatSettingChoice(visibleName, funcName, enabledValue, offValue)
        return {
            Name = visibleName,
            ChosenFunction = function()
                local po = getPlayerOptions()
                if po[funcName](po) ~= offValue then
                    setPlayerOptionsModValueAllLevels(funcName, offValue)
                else
                    setPlayerOptionsModValueAllLevels(funcName, enabledValue)
                end
            end,
        }
    end

    -- for convenience to generate a choice table for a boolean interface setting
    local function booleanSettingChoice(visibleName, funcName)
        return {
            Name = visibleName,
            ChosenFunction = function()
                local po = getPlayerOptions()
                if po[funcName](po) == true then
                    setPlayerOptionsModValueAllLevels(funcName, false)
                else
                    setPlayerOptionsModValueAllLevels(funcName, true)
                end
            end,
        }
    end

    -- for convenience to generate a direction table for a setting which goes in either direction and wraps via PREFSMAN
    -- if the max value is reached, the min value is the next one
    local function preferenceIncrementDecrementDirections(preferenceName, minValue, maxValue, increment)
        return {
            Left = function()
                local x = clamp(PREFSMAN:GetPreference(preferenceName), minValue, maxValue)
                x = notShit.round(x - increment, 3)
                if x < minValue then x = maxValue end
                PREFSMAN:SetPreference(preferenceName, notShit.round(x, 3))
            end,
            Right = function()
                local x = clamp(PREFSMAN:GetPreference(preferenceName), minValue, maxValue)
                x = notShit.round(x + increment, 3)
                if x > maxValue then x = minValue end
                PREFSMAN:SetPreference(preferenceName, notShit.round(x, 3))
            end,
        }
    end

    local function basicNamedPreferenceChoice(preferenceName, displayName, chosenValue)
        return {
            Name = displayName,
            ChosenFunction = function()
                PREFSMAN:SetPreference(preferenceName, chosenValue)
            end,
        }
    end
    local function preferenceToggleDirections(preferenceName, trueValue, falseValue)
        return {
            Toggle = function()
                if PREFSMAN:GetPreference(preferenceName) == trueValue then
                    PREFSMAN:SetPreference(preferenceName, falseValue)
                else
                    PREFSMAN:SetPreference(preferenceName, trueValue)
                end
            end,
        }
    end
    local function preferenceToggleIndexGetter(preferenceName, oneValue)
        -- oneValue is what we expect for choice index 1 (the first one)
        return function()
            if PREFSMAN:GetPreference(preferenceName) == oneValue then
                return 1
            else
                return 2
            end
        end
    end
    -- convenience to create a Choices table for a variable number of choice names
    local function choiceSkeleton(...)
        local o = {}
        for _, name in ipairs({...}) do
            o[#o+1] = {
                Name = name,
            }
        end
        return o
    end

    local function initDisplayResolutions()
        local resolutions = {}
        local displaySpecs = GetDisplaySpecs()
        for _, spec in ipairs(displaySpecs) do
            for __, mode in ipairs(spec:GetSupportedModes()) do
                -- linear search? sure
                local add = true
                for ___, res in ipairs(resolutions) do
                    if res.w == mode:GetWidth() and res.h == mode:GetHeight() then add = false break end
                end
                if add then
                    resolutions[#resolutions+1] = {
                        w = mode:GetWidth(),
                        h = mode:GetHeight(),
                    }
                end
            end
        end
        return resolutions
    end

    local function getdataTHEME(category, propertyname)
        return function() return themeConfig:get_data()[category][propertyname] end
    end
    local function setdataTHEME(category, propertyname, val)
        themeConfig:get_data()[category][propertyname] = val
        themeConfig:set_dirty()
        themeConfig:save()
    end
    local function getdataPLAYER(propertyname)
        return function() return playerConfig:get_data()[propertyname] end
    end
    local function setdataPLAYER(propertyname, val)
        playerConfig:get_data()[propertyname] = val
        playerConfig:set_dirty()
        playerConfig:save()
    end
    local function themeoption(category, propertyname)
        return {get = getdataTHEME(category, propertyname), set = function(x) setdataTHEME(category, propertyname, x) end}
    end
    local function playeroption(propertyname)
        return {get = getdataPLAYER(propertyname), set = function(x) setdataPLAYER(propertyname, x) end}
    end

    --
    -- -----

    -- -----
    -- Extra data for option temporary storage or cross option interaction
    --
    local playerConfigData = playerConfig:get_data()
    local themeConfigData = themeConfig:get_data()
    local displaySpecs = GetDisplaySpecs()
    local optionData = {
        speedMod = {
            speed = getSpeedValueFromPlayerOptions(),
            mode = getSpeedModeFromPlayerOptions(),
        },
        noteSkins = {
            names = NOTESKIN:GetNoteSkinNames(),
        },
        gameMode = {
            modes = GAMEMAN:GetEnabledGames(),
            current = GAMESTATE:GetCurrentGame():GetName(),
        },
        language = {
            list = THEME:GetLanguages(),
            current = THEME:GetCurLanguage(),
        },
        instantSearch = themeoption("global", "InstantSearch"),
        wheelPosition = themeoption("global", "WheelPosition"),
        wheelBanners = themeoption("global", "WheelBanners"),
        showBackgrounds = themeoption("global", "ShowBackgrounds"),
        showVisualizer = themeoption("global", "ShowVisualizer"),
        tipType = themeoption("global", "TipType"),
        allowBGChanges = themeoption("global", "StaticBackgrounds"),
        videoBanners = themeoption("global", "VideoBanners"),

        -- gameplay elements
        bpmDisplay = playeroption("BPMDisplay"),
        displayPercent = playeroption("DisplayPercent"),
        errorBar = playeroption("ErrorBar"), -- has on, off, ewma
        errorBarCount = playeroption("ErrorBarCount"), -- how many bar count
        fullProgressBar = playeroption("FullProgressBar"),
        miniProgressBar = playeroption("MiniProgressBar"),
        judgeCounter = playeroption("JudgeCounter"),
        leaderboard = playeroption("leaderboardEnabled"),
        displayMean = playeroption("DisplayMean"),
        measureCounter = playeroption("MeasureCounter"),
        npsDisplay = playeroption("NPSDisplay"),
        npsGraph = playeroption("NPSGraph"),
        playerInfo = playeroption("PlayerInfo"),
        rateDisplay = playeroption("RateDisplay"),
        targetTracker = playeroption("TargetTracker"),
        targetTrackerMode = playeroption("TargetTrackerMode"), -- 0 is goal, anything else is pb
        targetTrackerGoal = playeroption("TargetGoal"), -- only valid for TargetTrackerMode 0 unless no pb
        laneCover = playeroption("LaneCover"), -- 0 off, 1 sudden, 2 hidden
        judgmentText = playeroption("JudgmentText"),
        judgmentTweens = playeroption("JudgmentTweens"),
        comboText = playeroption("ComboText"),
        comboLabel = playeroption("ComboLabel"),

        cbHighlight = playeroption("CBHighlight"),
        screenFilter = playeroption("ScreenFilter"), -- [0,1] notefield bg
        receptorSize = playeroption("ReceptorSize"),


        display = {
            ratios = { -- hardcoded aspect ratio list
                {n = 3, d = 4},
                {n = 1, d = 1},
                {n = 5, d = 4},
                {n = 4, d = 3},
                {n = 16, d = 10},
                {n = 16, d = 9},
                {n = 8, d = 3},
                {n = 21, d = 9}
            },
            refreshRates = { -- hardcoded refresh rate list (i dont know what im doing)
                -- if you put floats here it is your fault if something breaks but go nuts if you want
                --REFRESH_DEFAULT, -- skip this, it is hardcoded as the first value
                59,
                60,
                70,
                72,
                75,
                80,
                85,
                90,
                100,
                120,
                144,
                150,
                240,
            },
            -- displayspec generated "compatible" ratios
            dRatios = GetDisplayAspectRatios(displaySpecs),
            wRatios = GetWindowAspectRatios(),
            -- displayspec generated "compatible" resolutions
            resolutions = initDisplayResolutions(),
            loadedAspectRatio = PREFSMAN:GetPreference("DisplayAspectRatio")
        },
        pickedTheme = THEME:GetCurThemeName(),
    }
    --
    -- -----

    -- -----
    -- Extra utility functions that require optionData to be initialized first
    local function setSpeedValueFromOptionData()
        local mode = optionData.speedMod.mode
        local speed = optionData.speedMod.speed
        if mode == "X" then
            -- the way we store stuff, xmod must divide by 100
            -- theres no quirk to it, thats just because we store the number as an int (not necessarily an int but yeah)
            -- so 0.01x XMod would be a CMod of 1 -- this makes even more sense if you just think about it
            setPlayerOptionsModValueAllLevels("XMod", speed/100)
        elseif mode == "C" then
            setPlayerOptionsModValueAllLevels("CMod", speed)
        elseif mode == "M" then
            setPlayerOptionsModValueAllLevels("MMod", speed)
        end
    end
    local function basicNamedOptionDataChoice(optionDataPropertyName, displayName, chosenValue)
        return {
            Name = displayName,
            ChosenFunction = function()
                optionData[optionDataPropertyName] = chosenValue
            end,
        }
    end
    local function optionDataToggleDirections(optionDataPropertyName, trueValue, falseValue)
        return {
            Toggle = function()
                if optionData[optionDataPropertyName] == trueValue then
                    optionData[optionDataPropertyName] = falseValue
                else
                    optionData[optionDataPropertyName] = trueValue
                end
            end,
        }
    end
    local function optionDataToggleIndexGetter(optionDataPropertyName, oneValue)
        -- oneValue is what we expect for choice index 1 (the first one)
        return function()
            if optionData[optionDataPropertyName] == oneValue then
                return 1
            else
                return 2
            end
        end
    end
    local function optionDataToggleDirectionsFUNC(optionDataPropertyName, trueValue, falseValue)
        return {
            Toggle = function()
                if optionData[optionDataPropertyName].get() == trueValue then
                    optionData[optionDataPropertyName].set(falseValue)
                else
                    optionData[optionDataPropertyName].set(trueValue)
                end
            end,
        }
    end
    local function optionDataToggleIndexGetterFUNC(optionDataPropertyName, oneValue)
        -- oneValue is what we expect for choice index 1 (the first one)
        return function()
            if optionData[optionDataPropertyName].get() == oneValue then
                return 1
            else
                return 2
            end
        end
    end
    local function customizeGameplayButton()
        return {
            Name = "Customize Playfield",
            Type = "Button",
            Explanation = "Customize Gameplay elements.",
            Choices = {
                {
                    Name = "Customize Playfield",
                    ChosenFunction = function()
                        -- activate customize gameplay
                        -- go into gameplay
                        playerConfig:get_data().CustomizeGameplay = true

                        local wheel = SCREENMAN:GetTopScreen():GetChild("WheelFile")
                        if GAMESTATE:GetCurrentSong() ~= nil then
                            -- select current
                            wheel:playcommand("SelectCurrent")
                        else
                            -- select random
                            local group = WHEELDATA:GetRandomFolder()
                            local song = WHEELDATA:GetRandomSongInFolder(group)
                            wheel:playcommand("FindSong", {song = song})
                            wheel:playcommand("SelectCurrent")
                        end
                    end,
                }
            }
        }
    end
    --
    -- -----

    local optionRowCount = 17 -- weird behavior if you mess with this and have too many options in a category
    local maxChoicesVisibleMultiChoice = 4 -- max number of choices visible in a MultiChoice OptionRow

    -- the names and order of the option pages
    -- these values must correspond to the keys of optionPageCategoryLists
    local pageNames = {
        "Player",
        "Gameplay",
        "Graphics",
        "Sound",
        "Input",
        "Profiles",
    }

    -- mappings of option page names to lists of categories
    -- the keys in this table are option pages
    -- the values are tables -- the categories of each page in that order
    -- each category corresponds to a key in optionDefs (must be unique keys -- values of these tables have to be globally unique)
    -- the options of each category are in the order of the value tables in optionDefs
    local optionPageCategoryLists = {
        Player = {
            "Essential Options",
            "Appearance Options",
            "Invalidating Options",
        },
        Gameplay = {
            "Gameplay Elements 1",
            "Gameplay Elements 2",
        },
        Graphics = {
            "Global Options",
            "Theme Options",
        },
        Sound = {
            "Sound Options",
        },
        Input = {
            "Input Options",
        },
        Profiles = {
            "Profile Options",
        },
    }

    -- the mother of all tables.
    -- this is each option definition for every single option present in the right frame
    -- mapping option categories to lists of options
    -- LIMITATIONS: A category cannot have more sub options than the max number of lines minus the number of categories.
    --  example: 25 lines? 2 categories? up to 23 options per category.
    -- TYPE LIST:
    --  SingleChoice            -- scrolls through choices
    --  SingleChoiceModifier    -- scrolls through choices, shows 2 sets of arrows for each direction, allowing multiplier
    --  MultiChoice             -- shows all options at once, selecting any amount of them
    --  Button                  -- it's a button. you press enter on it.
    --
    -- OPTION DEFINITION EXAMPLE:
    --[[
        {
            Name = "option name" -- display name for the option
            Type = "type name" -- determines how to generate the actor to display the choices
            AssociatedOptions = {"other option name"} -- runs the index getter for these options when a choice is selected
            Choices = { -- option choice definitions -- each entry is another table -- if no choices are defined, visible choice comes from ChoiceIndexGetter
                {
                    Name = "choice1" -- display name for the choice
                    ChosenFunction = function() end -- what happens when choice is PICKED (not hovered)
                },
                {
                    Name = "choice2"
                    ...
                },
                ...
            },
            Directions = {
                -- table of direction functions -- these define what happens for each pressed direction button
                -- most options have only Left and Right
                -- if these functions are undefined and required by the option type, a default function moves the index of the choice rotationally
                -- some option types may allow for more directions or direction multipliers
                -- if Toggle is defined, this function is used for all direction presses
                Left = function() end,
                Right = function() end,
                Toggle = function() end, --- OPTIONAL -- WILL REPLACE ALL DIRECTION FUNCTIONALITY IF PRESENT
                ...
            },
            ChoiceIndexGetter = function() end -- a function to run to get the choice index or text, or return a table for multi selection options
            ChoiceGenerator = function() end -- an OPTIONAL function for generating the choices table if too long to write out (return a table)
            Explanation = "" -- an explanation that appears at the bottom of the screen
        }
    ]]
    local optionDefs = {
        -----
        -- PLAYER OPTIONS
        ["Essential Options"] = {
            {
                Name = "Scroll Type",
                Type = "SingleChoice",
                Explanation = "XMod - BPM multiplier based scrolling. CMod - Constant scrolling. MMod - BPM based with a max speed.",
                AssociatedOptions = {
                    "Scroll Speed",
                },
                Choices = choiceSkeleton("XMod", "CMod", "MMod"),
                Directions = {
                    Left = function()
                        -- traverse list left, set the speed mod again
                        -- order:
                        -- XMOD - CMOD - MMOD
                        local mode = optionData.speedMod.mode
                        if mode == "C" then
                            mode = "X"
                        elseif mode == "M" then
                            mode = "C"
                        elseif mode == "X" then
                            mode = "M"
                        end
                        optionData.speedMod.mode = mode
                        setSpeedValueFromOptionData()
                    end,
                    Right = function()
                        -- traverse list right, set the speed mod again
                        -- order:
                        -- XMOD - CMOD - MMOD
                        local mode = optionData.speedMod.mode
                        if mode == "C" then
                            mode = "M"
                        elseif mode == "M" then
                            mode = "X"
                        elseif mode == "X" then
                            mode = "C"
                        end
                        optionData.speedMod.mode = mode
                        setSpeedValueFromOptionData()
                    end,
                },
                ChoiceIndexGetter = function()
                    local mode = optionData.speedMod.mode
                    if mode == "X" then return 1
                    elseif mode == "C" then return 2
                    elseif mode == "M" then return 3 end
                end,
            },
            {
                Name = "Scroll Speed",
                Type = "SingleChoiceModifier",
                Explanation = "Change scroll speed value/modifier in increments of 1 or 50.",
                Directions = {
                    Left = function(multiplier)
                        local increment = -1
                        if multiplier then increment = -50 end
                        optionData.speedMod.speed = optionData.speedMod.speed + increment
                        if optionData.speedMod.speed <= 0 then optionData.speedMod.speed = 1 end
                        setSpeedValueFromOptionData()
                    end,
                    Right = function(multiplier)
                        local increment = 1
                        if multiplier then increment = 50 end
                        optionData.speedMod.speed = optionData.speedMod.speed + increment
                        if optionData.speedMod.speed <= 0 then optionData.speedMod.speed = 1 end
                        setSpeedValueFromOptionData()
                    end,
                },
                ChoiceIndexGetter = function()
                    local mode = optionData.speedMod.mode
                    local speed = optionData.speedMod.speed
                    if mode == "X" then
                        return mode .. notShit.round((speed/100), 2)
                    else
                        return mode .. speed
                    end
                end,
            },
            {
                Name = "Scroll Direction",
                Type = "SingleChoice",
                Explanation = "Direction of note scrolling: up or down.",
                Choices = choiceSkeleton("Upscroll", "Downscroll"),
                Directions = {
                    Toggle = function()
                        if not getPlayerOptions():UsingReverse() then
                            -- 1 is 100% reverse which means on
                            setPlayerOptionsModValueAllLevels("Reverse", 1)
                        else
                            -- 0 is 0% reverse which means off
                            setPlayerOptionsModValueAllLevels("Reverse", 0)
                        end
                        MESSAGEMAN:Broadcast("UpdateReverse")
                    end,
                },
                ChoiceIndexGetter = function()
                    if getPlayerOptions():UsingReverse() then
                        return 2
                    else
                        return 1
                    end
                end,
            },
            {
                Name = "Noteskin",
                Type = "SingleChoice",
                Explanation = "Skin of the notes.",
                ChoiceIndexGetter = function()
                    local currentSkinName = getPlayerOptions():NoteSkin()
                    for i, name in ipairs(optionData.noteSkins.names) do
                        if name == currentSkinName then
                            return i
                        end
                    end
                    -- if function gets this far, look for the default skin
                    currentSkinName = THEME:GetMetric("Common", "DefaultNoteSkinName")
                    for i, name in ipairs(optionData.noteSkins.names) do
                        if name == currentSkinName then
                            return i
                        end
                    end
                    -- if function gets this far, cant find anything so just return the first skin
                    return 1
                end,
                ChoiceGenerator = function()
                    local o = {}
                    local skinNames = NOTESKIN:GetNoteSkinNames()
                    for i, name in ipairs(skinNames) do
                        o[#o+1] = {
                            Name = name:upper(),
                            ChosenFunction = function()
                                setPlayerOptionsModValueAllLevels("NoteSkin", name)
                                MESSAGEMAN:Broadcast("UpdateVisibleSkin", {name = name})
                            end,
                        }
                    end
                    table.sort(
                        o,
                        function(a, b)
                            return a.Name:lower() < b.Name:lower()
                        end)

                    return o
                end,
            },
            {
                Name = "Receptor Size",
                Type = "SingleChoice",
                Explanation = "Size of receptors and notes. 50% Receptor Size may be called 100% Mini.",
                Directions = {
                    Left = function()
                        local sz = optionData["receptorSize"].get()
                        sz = sz - 1
                        if sz < 1 then sz = 200 end
                        optionData["receptorSize"].set(sz)
                    end,
                    Right = function()
                        local sz = optionData["receptorSize"].get()
                        sz = sz + 1
                        if sz > 200 then sz = 1 end
                        optionData["receptorSize"].set(sz)
                    end,
                },
                ChoiceIndexGetter = function()
                    return optionData["receptorSize"].get() .. "%"
                end,
            },
            {
                Name = "Judge Difficulty",
                Type = "SingleChoice",
                Explanation = "Timing Window Difficulty. Higher is harder. All scores are converted to Judge 4 later.",
                ChoiceIndexGetter = function()
                    local lowestJudgeDifficulty = 4
                    return GetTimingDifficulty() - (lowestJudgeDifficulty-1)
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i = 4, 8 do
                        o[#o+1] = {
                            Name = tostring(i),
                            ChosenFunction = function()
                                -- set judge
                                local scale = ms.JudgeScalers[i]
                                if scale == nil then scale = 1 end
                                SetTimingDifficulty(scale)
                            end,
                        }
                    end
                    o[#o+1] = {
                        Name = "Justice",
                        ChosenFunction = function()
                            -- sets j9
                            local scale = ms.JudgeScalers[9]
                            if scale == nil then scale = 1 end
                            SetTimingDifficulty(scale)
                        end,
                    }
                    return o
                end,
            },
            {
                Name = "Global Offset",
                Type = "SingleChoice",
                Explanation = "Global Audio Offset in seconds. Negative numbers are early.",
                Directions = preferenceIncrementDecrementDirections("GlobalOffsetSeconds", -5, 5, 0.001),
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("GlobalOffsetSeconds"), 3) .. "s"
                end,
            },
            {
                Name = "Visual Delay",
                Type = "SingleChoice",
                Explanation = "Visual Note Delay in seconds. May be referred to as Judge Offset. Negative numbers are early.",
                Directions = preferenceIncrementDecrementDirections("VisualDelaySeconds", -5, 5, 0.001),
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("VisualDelaySeconds"), 3) .. "s"
                end,
            },
            {
                Name = "Game Mode",
                Type = "SingleChoice",
                Explanation = "Dance - 3k/4k/8k | Solo - 6k | Pump - 5k/6k/10k | Beat - 5k+1/7k+1/10k+2/14k+2 | Kb7 - 7k | Popn - 5k/9k",
                ChoiceIndexGetter = function()
                    for i = 1, #optionData.gameMode.modes do
                        if optionData.gameMode.modes[i] == optionData.gameMode.current then
                            return i
                        end
                    end
                    return 1
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i, name in ipairs(optionData.gameMode.modes) do
                        o[#o+1] = {
                            Name = strCapitalize(name),
                            ChosenFunction = function()
                                --GAMEMAN:SetGame(name)
                                optionData.gameMode.current = name
                            end,
                        }
                    end
                    return o
                end,
            },
            {
                Name = "Fail Type",
                Type = "SingleChoice",
                Explanation = "Toggle failure in Gameplay. Setting Fail Off invalidates scores if a fail would have actually occurred.",
                ChoiceIndexGetter = function()
                    local failtypes = FailType
                    local failtype = getPlayerOptions():FailSetting()
                    for i, name in ipairs(failtypes) do
                        if name == failtype then return i end
                    end
                    return 1
                end,
                ChoiceGenerator = function()
                    -- get the list of fail types
                    local failtypes = FailType
                    local o = {}
                    for i, name in ipairs(failtypes) do
                        o[#o+1] = {
                            Name = THEME:GetString("OptionNames", ToEnumShortString(name)),
                            ChosenFunction = function()
                                setPlayerOptionsModValueAllLevels("FailSetting", name)
                            end,
                        }
                    end
                    return o
                end,
            },
            customizeGameplayButton(),
            {
                Name = "Customize Keybinds",
                Type = "Button",
                Explanation = "Customize Keybinds.",
                Choices = {
                    {
                        Name = "Customize Keybinds",
                        ChosenFunction = function()
                            -- activate keybind screen
                            MESSAGEMAN:Broadcast("ShowSettingsAlt", {name = "Customize Keybinds"})
                        end,
                    }
                }
            },
            {
                Name = "Enter Practice Mode",
                Type = "Button",
                Explanation = "Enter Practice Mode",
                Choices = {
                    {
                        Name = "Enter Practice Mode",
                        ChosenFunction = function()
                            -- activate practice mode
                            -- go into gameplay
                            playerConfig:get_data().PracticeMode = true
                            GAMESTATE:SetPracticeMode(true)

                            local wheel = SCREENMAN:GetTopScreen():GetChild("WheelFile")
                            if GAMESTATE:GetCurrentSong() ~= nil then
                                -- select current
                                wheel:playcommand("SelectCurrent")
                            else
                                -- select random
                                local group = WHEELDATA:GetRandomFolder()
                                local song = WHEELDATA:GetRandomSongInFolder(group)
                                wheel:playcommand("FindSong", {song = song})
                                wheel:playcommand("SelectCurrent")
                            end
                        end,
                    }
                }
            }
        },
        --
        -----
        -- APPEARANCE OPTIONS
        ["Appearance Options"] = {
            {
                Name = "Appearance",
                Type = "MultiChoice",
                Explanation = "Hidden - Notes disappear before receptor. Sudden - Notes appear later than usual. Stealth - Invisible notes. Blink - Notes flash.",
                Choices = {
                    -- multiple choices allowed
                    floatSettingChoice("Hidden", "Hidden", 1, 0),
                    floatSettingChoice("HiddenOffset", "HiddenOffset", 1, 0),
                    floatSettingChoice("Sudden", "Sudden", 1, 0),
                    floatSettingChoice("SuddenOffset", "SuddenOffset", 1, 0),
                    floatSettingChoice("Stealth", "Stealth", 1, 0),
                    floatSettingChoice("Blink", "Blink", 1, 0)
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Hidden() ~= 0 then o[1] = true end
                    if po:HiddenOffset() ~= 0 then o[2] = true end
                    if po:Sudden() ~= 0 then o[3] = true end
                    if po:SuddenOffset() ~= 0 then o[4] = true end
                    if po:Stealth() ~= 0 then o[5] = true end
                    if po:Blink() ~= 0 then o[6] = true end
                    return o
                end,
            },
            {
                Name = "Perspective",
                Type = "SingleChoice",
                Explanation = "Controls tilt/skew of the NoteField.",
                Choices = {
                    -- the numbers in these defs are like the percentages you would put in metrics instead
                    -- 1 is 100%
                    -- Overhead does not use percentages.
                    -- adding an additional parameter to these functions does do something (approach rate) but is functionally useless
                    -- you are free to try these untested options for possible weird results:
                    -- setPlayerOptionsModValueAllLevels("Skew", x)
                    -- setPlayerOptionsModValueAllLevels("Tilt", x)
                    {
                        Name = "Overhead",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Overhead", true)
                        end,
                    },
                    {
                        Name = "Incoming",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Incoming", 1)
                        end,
                    },
                    {
                        Name = "Space",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Space", 1)
                        end,
                    },
                    {
                        Name = "Hallway",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Hallway", 1)
                        end,
                    },
                    {
                        Name = "Distant",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Distant", 1)
                        end,
                    },
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    -- we unfortunately choose to hardcode these options and not allow an additional custom one
                    -- but the above choice definitions allow customizing the specific Perspective to whatever extent you want
                    local o = {}
                    if po:Overhead() then return 1
                    elseif po:Incoming() ~= nil then return 2
                    elseif po:Space() ~= nil then return 3
                    elseif po:Hallway() ~= nil then return 4
                    elseif po:Distant() ~= nil then return 5
                    end
                    return o
                end,
            },
            {
                Name = "Mirror",
                Type = "SingleChoice",
                Explanation = "Horizontally flip Notedata.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = {
                    Toggle = function()
                        local po = getPlayerOptions()
                        if po:Mirror() then
                            setPlayerOptionsModValueAllLevels("Mirror", false)
                        else
                            setPlayerOptionsModValueAllLevels("Mirror", true)
                        end
                    end,
                },
                ChoiceIndexGetter = function()
                    if getPlayerOptions():Mirror() then
                        return 1
                    else
                        return 2
                    end
                end,
            },
            {
                Name = "Hide Player UI",
                Type = "MultiChoice",
                Explanation = "Hide certain sets of elements from the Gameplay UI.",
                Choices = {
                    floatSettingChoice("Hide Receptors", "Dark", 1, 0),
                    floatSettingChoice("Hide Judgment & Combo", "Blind", 1, 0),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Dark() ~= 0 then o[1] = true end
                    if po:Blind() ~= 0 then o[2] = true end
                    return o
                end,
            },
            {
                Name = "Hidenote Judgment",
                Type = "SingleChoice",
                Explanation = "Notes must be hit with this judgment or better to disappear.",
                Choices = {
                    {
                        Name = "Miss",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "Miss")
                            setPlayerOptionsModValueAllLevels("MinTNSToHideNotes", "Miss")
                        end,
                    },
                    {
                        Name = "Bad",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "W5")
                            setPlayerOptionsModValueAllLevels("MinTNSToHideNotes", "W5")
                        end,
                    },
                    {
                        Name = "Good",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "W4")
                            setPlayerOptionsModValueAllLevels("MinTNSToHideNotes", "W4")
                        end,
                    },
                    {
                        Name = "Great",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "W3")
                            setPlayerOptionsModValueAllLevels("MinTNSToHideNotes", "W3")
                        end,
                    },
                    {
                        Name = "Perfect",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "W2")
                            setPlayerOptionsModValueAllLevels("MinTNSToHideNotes", "W2")
                        end,
                    },
                    {
                        Name = "Marvelous",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "W1")
                            setPlayerOptionsModValueAllLevels("MinTNSToHideNotes", "W1")
                        end,
                    },
                },
                ChoiceIndexGetter = function()
                    local opt = PREFSMAN:GetPreference("MinTNSToHideNotes")
                    if opt == "TapNoteScore_Miss" then return 1
                    elseif opt == "TapNoteScore_W5" then return 2
                    elseif opt == "TapNoteScore_W4" then return 3
                    elseif opt == "TapNoteScore_W3" then return 4
                    elseif opt == "TapNoteScore_W2" then return 5
                    elseif opt == "TapNoteScore_W1" then return 6
                    else
                        return 4 -- this is the default option so default to this
                    end
                end,
            },
            {
                Name = "Default Centered NoteField",
                Type = "SingleChoice",
                Explanation = "Horizontally center the NoteField in Gameplay (Legacy Shortcut).",
                Choices = choiceSkeleton("Yes", "No"),
                Directions = preferenceToggleDirections("Center1Player", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("Center1Player", true),
            },
            {
                Name = "NoteField BG Opacity",
                Type = "SingleChoice",
                Explanation = "Set the opacity of the board behind the NoteField in Gameplay.",
                ChoiceGenerator = function()
                    local o = {}
                    for i = 0, 10 do -- 11 choices
                        o[#o+1] = {
                            Name = notShit.round(i*10,0).."%",
                            ChosenFunction = function()
                                optionData["screenFilter"].set(notShit.round(i / 10, 1))
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local v = notShit.round(optionData["screenFilter"].get(), 1)
                    local ind = notShit.round(v * 10, 0) + 1
                    if ind > 0 and ind < 11 then -- this 11 should match the number of choices above
                        return ind
                    else
                        if ind <= 0 then
                            return 1
                        else
                            return 11
                        end
                    end
                end,
            },
            {
                Name = "Background Brightness",
                Type = "SingleChoice",
                Explanation = "Set the brightness of the background in Gameplay. 0% will disable background loading.",
                ChoiceGenerator = function()
                    local o = {}
                    for i = 0, 10 do -- 11 choices
                        o[#o+1] = {
                            Name = notShit.round(i*10,0).."%",
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("BGBrightness", notShit.round(i / 10, 1))
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local v = notShit.round(PREFSMAN:GetPreference("BGBrightness"))
                    local ind = notShit.round(v * 10, 0) + 1
                    if ind > 0 and ind < 11 then -- this 11 should match the nubmer of choices above
                        return ind
                    else
                        if ind <= 0 then
                            return 1
                        else
                            return 11
                        end
                    end
                end,
            },
            {
                Name = "Replay Mod Emulation",
                Type = "SingleChoice",
                Explanation = "Toggle temporarily using compatible mods that replays used when watching them.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("ReplaysUseScoreMods", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("ReplaysUseScoreMods", true),
            },
            {
                Name = "Extra Scroll Mods",
                Type = "MultiChoice",
                Explanation = "Change scroll direction in more interesting ways.",
                Choices = {
                    floatSettingChoice("Split", "Split", 1, 0),
                    floatSettingChoice("Alternate", "Alternate", 1, 0),
                    floatSettingChoice("Cross", "Cross", 1, 0),
                    floatSettingChoice("Centered", "Centered", 1, 0),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Split() ~= 0 then o[1] = true end
                    if po:Alternate() ~= 0 then o[2] = true end
                    if po:Cross() ~= 0 then o[3] = true end
                    if po:Centered() ~= 0 then o[4] = true end
                    return o
                end,
            },
            {
                Name = "Fun Effects",
                Type = "MultiChoice",
                Explanation = "Visual scroll mods that are not for practical use.",
                Choices = {
                    floatSettingChoice("Drunk", "Drunk", 1, 0),
                    floatSettingChoice("Confusion", "Confusion", 1, 0),
                    floatSettingChoice("Tiny", "Tiny", 1, 0),
                    floatSettingChoice("Flip", "Flip", 1, 0),
                    floatSettingChoice("Invert", "Invert", 1, 0),
                    floatSettingChoice("Tornado", "Tornado", 1, 0),
                    floatSettingChoice("Tipsy", "Tipsy", 1, 0),
                    floatSettingChoice("Bumpy", "Bumpy", 1, 0),
                    floatSettingChoice("Beat", "Beat", 1, 0),
                    -- X-Mode is dead because it relies on player 2!! -- floatSettingChoice("X-Mode"),
                    floatSettingChoice("Twirl", "Twirl", 1, 0),
                    floatSettingChoice("Roll", "Roll", 1, 0),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Drunk() ~= 0 then o[1] = true end
                    if po:Confusion() ~= 0 then o[2] = true end
                    if po:Tiny() ~= 0 then o[3] = true end
                    if po:Flip() ~= 0 then o[4] = true end
                    if po:Invert() ~= 0 then o[5] = true end
                    if po:Tornado() ~= 0 then o[6] = true end
                    if po:Tipsy() ~= 0 then o[7] = true end
                    if po:Bumpy() ~= 0 then o[8] = true end
                    if po:Beat() ~= 0 then o[9] = true end
                    if po:Twirl() ~= 0 then o[10] = true end
                    if po:Roll() ~= 0 then o[11] = true end
                    return o
                end,
            },
            {
                Name = "Acceleration",
                Type = "MultiChoice",
                Explanation = "Scroll speed mods usually not for practical use.",
                Choices = {
                    floatSettingChoice("Boost", "Boost", 1, 0),
                    floatSettingChoice("Brake", "Brake", 1, 0),
                    floatSettingChoice("Wave", "Wave", 1, 0),
                    floatSettingChoice("Expand", "Expand", 1, 0),
                    floatSettingChoice("Boomerang", "Boomerang", 1, 0),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Boost() ~= 0 then o[1] = true end
                    if po:Brake() ~= 0 then o[2] = true end
                    if po:Wave() ~= 0 then o[3] = true end
                    if po:Expand() ~= 0 then o[4] = true end
                    if po:Boomerang() ~= 0 then o[5] = true end
                    return o
                end,
            }
        },
        --
        -----
        -- INVALIDATING OPTIONS
        ["Invalidating Options"] = {
            {
                Name = "Mines",
                Type = "SingleChoice",
                Explanation = "Toggle Mines. Extra Mines will replace entire rows of notes with mines.",
                Choices = {
                    {
                        Name = "On",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("NoMines", false)
                            setPlayerOptionsModValueAllLevels("Mines", false)
                        end,
                    },
                    {
                        Name = "Off",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("NoMines", true)
                            setPlayerOptionsModValueAllLevels("Mines", false)
                        end,
                    },
                    {
                        Name = "Extra Mines",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("NoMines", false)
                            setPlayerOptionsModValueAllLevels("Mines", true)
                        end,
                    }
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    if po:Mines() then
                        -- additive mines, invalidating
                        return 3
                    elseif po:NoMines() then
                        -- nomines, invalidating
                        return 2
                    else
                        -- regular mines, not invalidating
                        return 1
                    end
                end,
            },
            {
                Name = "Turn",
                Type = "MultiChoice",
                Explanation = "Modify Notedata by either shifting all notes or randomizing them.",
                Choices = {
                    booleanSettingChoice("Backwards", "Backwards"),
                    booleanSettingChoice("Left", "Left"),
                    booleanSettingChoice("Right", "Right"),
                    booleanSettingChoice("Shuffle", "Shuffle"),
                    booleanSettingChoice("Soft Shuffle", "SoftShuffle"),
                    booleanSettingChoice("Super Shuffle", "SuperShuffle"),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Backwards() then o[1] = true end
                    if po:Left() then o[2] = true end
                    if po:Right() then o[3] = true end
                    if po:Shuffle() then o[4] = true end
                    if po:SoftShuffle() then o[5] = true end
                    if po:SuperShuffle() then o[6] = true end
                    return o
                end,
            },
            {
                Name = "Pattern Transform",
                Type = "MultiChoice",
                Explanation = "Modify Notedata by inserting extra notes to create certain patterns.",
                Choices = {
                    booleanSettingChoice("Echo", "Echo"),
                    booleanSettingChoice("Stomp", "Stomp"),
                    booleanSettingChoice("Jack JS", "JackJS"),
                    booleanSettingChoice("Anchor JS", "AnchorJS"),
                    booleanSettingChoice("IcyWorld", "IcyWorld"),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Echo() then o[1] = true end
                    if po:Stomp() then o[2] = true end
                    if po:JackJS() then o[3] = true end
                    if po:AnchorJS() then o[4] = true end
                    if po:IcyWorld() then o[5] = true end
                    return o
                end,
            },
            {
                Name = "Hold Transform",
                Type = "MultiChoice",
                Explanation = "Modify holds in Notedata.",
                Choices = {
                    booleanSettingChoice("Planted", "Planted"),
                    booleanSettingChoice("Floored", "Floored"),
                    booleanSettingChoice("Twister", "Twister"),
                    booleanSettingChoice("Holds To Rolls", "HoldRolls"),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Planted() then o[1] = true end
                    if po:Floored() then o[2] = true end
                    if po:Twister() then o[3] = true end
                    if po:HoldRolls() then o[4] = true end
                    return o
                end,
            },
            {
                Name = "Remove",
                Type = "MultiChoice",
                Explanation = "Remove certain notes, patterns, or types of notes.",
                Choices = {
                    booleanSettingChoice("No Holds", "NoHolds"),
                    booleanSettingChoice("No Rolls", "NoRolls"),
                    booleanSettingChoice("No Jumps", "NoJumps"),
                    booleanSettingChoice("No Hands", "NoHands"),
                    booleanSettingChoice("No Lifts", "NoLifts"),
                    booleanSettingChoice("No Fakes", "NoFakes"),
                    booleanSettingChoice("No Quads", "NoQuads"),
                    booleanSettingChoice("No Stretch", "NoStretch"),
                    booleanSettingChoice("Little", "Little"),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:NoHolds() then o[1] = true end
                    if po:NoRolls() then o[2] = true end
                    if po:NoJumps() then o[3] = true end
                    if po:NoHands() then o[4] = true end
                    if po:NoLifts() then o[5] = true end
                    if po:NoFakes() then o[6] = true end
                    if po:NoQuads() then o[7] = true end
                    if po:NoStretch() then o[8] = true end
                    if po:Little() then o[9] = true end
                    return o
                end,
            },
            {
                Name = "Insert",
                Type = "MultiChoice",
                Explanation = "Modify Notedata by inserting extra notes to provide a certain feeling.",
                Choices = {
                    booleanSettingChoice("Wide", "Wide"),
                    booleanSettingChoice("Big", "Big"),
                    booleanSettingChoice("Quick", "Quick"),
                    booleanSettingChoice("BMR-ize", "BMRize"),
                    booleanSettingChoice("Skippy", "Skippy"),
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    local o = {}
                    if po:Wide() then o[1] = true end
                    if po:Big() then o[2] = true end
                    if po:Quick() then o[3] = true end
                    if po:BMRize() then o[4] = true end
                    if po:Skippy() then o[5] = true end
                    return o
                end,
            }
        },
        --
        -----
        -- GAMEPLAY ELEMENTS P1
        ["Gameplay Elements 1"] = {
            customizeGameplayButton(),
            {
                Name = "BPM Display",
                Type = "SingleChoice",
                Explanation = "Toggle the BPM display.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("bpmDisplay", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("bpmDisplay", true),
            },
            {
                Name = "Rate Display",
                Type = "SingleChoice",
                Explanation = "Toggle the music rate display.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("rateDisplay", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("rateDisplay", true),
            },
            {
                Name = "Percent Display",
                Type = "SingleChoice",
                Explanation = "Toggle the wife percent display.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("displayPercent", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("displayPercent", true),
            },
            {
                Name = "Mean Display",
                Type = "SingleChoice",
                Explanation = "Toggle the tap mean display.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("displayMean", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("displayMean", true),
            },
            {
                Name = "Error Bar",
                Type = "SingleChoice",
                Explanation = "Toggle the error bar. Regular displays your recent tap offsets. EWMA displays the exponential mean weighted average of recent taps.",
                Choices = {
                    {
                        Name = "Regular",
                        ChosenFunction = function()
                            optionData["errorBar"].set(1)
                        end,
                    },
                    {
                        Name = "EWMA",
                        ChosenFunction = function()
                            optionData["errorBar"].set(2)
                        end,
                    },
                    {
                        Name = "Off",
                        ChosenFunction = function()
                            optionData["errorBar"].set(0)
                        end,
                    },
                },
                ChoiceIndexGetter = function()
                    local v = optionData["errorBar"].get()
                    if v == 0 then
                        return 3 -- off
                    elseif v == 1 then
                        return 1 -- on
                    else
                        return 2 -- ewma
                    end
                end,
            },
            {
                Name = "Error Bar Count",
                Type = "SingleChoice",
                Explanation = "Choose either how many taps are allowed to show for the Regular error bar, or how many taps are considered for the EWMA error bar.",
                ChoiceGenerator = function()
                    local o = {}
                    for i = 1, 50 do
                        o[#o+1] = {
                            Name = i,
                            ChosenFunction = function()
                                optionData["errorBarCount"].set(i)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local v = optionData["errorBarCount"].get()
                    if v < 1 or v > 50 then
                        return 1
                    else
                        return v
                    end
                end,
            },
            {
                Name = "Full Progress Bar",
                Type = "SingleChoice",
                Explanation = "Toggle the large progress bar.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("fullProgressBar", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("fullProgressBar", true),
            },
            {
                Name = "Mini Progress Bar",
                Type = "SingleChoice",
                Explanation = "Toggle the small progress bar.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("miniProgressBar", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("miniProgressBar", true),
            },

            {
                Name = "Leaderboard",
                Type = "SingleChoice",
                Explanation = "Toggle the gameplay leaderboard.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("leaderboard", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("leaderboard", true),
            },

            {
                Name = "Player Info",
                Type = "SingleChoice",
                Explanation = "Toggle the miscellaneous player info display.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("playerInfo", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("playerInfo", true),
            },
            {
                Name = "Target Tracker",
                Type = "SingleChoice",
                Explanation = "Toggle the target tracker. This displays your score and points relative to a target.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("targetTracker", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("targetTracker", true),
            },
            {
                Name = "Target Tracker Mode",
                Type = "SingleChoice",
                Explanation = "Toggle the target tracker mode. PB uses your PB unless it is does not exist. Then it falls back to Goal Percent.",
                Choices = {
                    {
                        Name = "Personal Best",
                        ChosenFunction = function()
                            optionData["targetTrackerMode"].set(1)
                        end,
                    },
                    {
                        Name = "Goal Percent",
                        ChosenFunction = function()
                            optionData["targetTrackerMode"].set(0)
                        end,
                    },
                },
                ChoiceIndexGetter = function()
                    local v = optionData["targetTrackerMode"].get()
                    if v == 0 then
                        return 2 -- goal
                    else
                        return 1 -- pb
                    end
                end,
            },
            {
                Name = "Target Tracker Goal",
                Type = "SingleChoice",
                Explanation = "Pick a goal percent for the target tracker.",
                ChoiceGenerator = function()
                    local o = {}
                    local function cf(i)
                        return {
                            Name = tostring(notShit.round(i, 4)),
                            ChosenFunction = function() optionData["targetTrackerGoal"].set(notShit.round(i, 4)) end
                        }
                    end
                    for i = 0, 46 do
                        o[#o+1] = cf(i + 50)
                    end
                    -- extra choices to fit grades
                    o[#o+1] = cf(96.65) -- AA.
                    o[#o+1] = cf(97)
                    o[#o+1] = cf(98)
                    o[#o+1] = cf(99) -- AA:
                    o[#o+1] = cf(99.7) -- AAA
                    o[#o+1] = cf(99.8)
                    o[#o+1] = cf(99.9)
                    o[#o+1] = cf(99.955) -- AAAA
                    o[#o+1] = cf(99.97)
                    o[#o+1] = cf(99.98)
                    o[#o+1] = cf(99.9935) -- AAAAA
                    return o
                end,
                ChoiceIndexGetter = function()
                    local v = optionData["targetTrackerGoal"].get()
                    if v < 50 or v > 99.995 then
                        return 1
                    elseif v <= 96 then
                        return v - 50 + 1
                    elseif v > 96 then
                        local extra = {
                            96.65,
                            97,
                            98,
                            99,
                            99.7,
                            99.8,
                            99.9,
                            99.955,
                            99.97,
                            99.98,
                            99.9935,
                        }
                        for i,vv in ipairs(extra) do
                            if v == vv then
                                return 47 + i
                            end
                        end
                        return 47
                    end
                end,
            },
            {
                Name = "Lane Cover",
                Type = "SingleChoice",
                Explanation = "Toggle the lane cover. Hidden is on top of the receptors. Sudden is away from the receptors.",
                Choices = {
                    {
                        Name = "Hidden",
                        ChosenFunction = function()
                            optionData["laneCover"].set(2)
                        end,
                    },
                    {
                        Name = "Sudden",
                        ChosenFunction = function()
                            optionData["laneCover"].set(1)
                        end,
                    },
                    {
                        Name = "Off",
                        ChosenFunction = function()
                            optionData["laneCover"].set(0)
                        end,
                    }
                },
                ChoiceIndexGetter = function()
                    local v = optionData["laneCover"].get()
                    if v == 0 then
                        return 3 -- off
                    elseif v == 1 then
                        return 2 -- sudden
                    else
                        return 1 -- hidden
                    end
                end,
            },
        },
        --
        -----
        -- GAMEPLAY ELEMENTS P2
        ["Gameplay Elements 2"] = {
            customizeGameplayButton(),
            {
                Name = "Judge Counter",
                Type = "SingleChoice",
                Explanation = "Toggle the judgment counter.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("judgeCounter", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("judgeCounter", true),
            },
            {
                Name = "Judgment Text",
                Type = "SingleChoice",
                Explanation = "Toggle the judgment text.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("judgmentText", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("judgmentText", true),
            },
            {
                Name = "Judgment Animations",
                Type = "SingleChoice",
                Explanation = "Toggle the judgment text animations.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("judgmentTweens", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("judgmentTweens", true),
            },
            {
                Name = "Combo Text",
                Type = "SingleChoice",
                Explanation = "Toggle the combo text.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("comboText", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("comboText", true),
            },
            {
                Name = "Combo Label",
                Type = "SingleChoice",
                Explanation = "Toggle the word 'Combo' as part of the combo text.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("comboLabel", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("comboLabel", true),
            },
            {
                Name = "Combo Breaker Highlights",
                Type = "SingleChoice",
                Explanation = "Toggle showing which column a combo breaker occurs.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("cbHighlight", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("cbHighlight", true),
            },
            {
                Name = "Measure Counter",
                Type = "SingleChoice",
                Explanation = "Toggle the measure counter. This shows up for longer runs of relatively high NPS.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("measureCounter", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("measureCounter", true),
            },
            {
                Name = "NPS Display",
                Type = "SingleChoice",
                Explanation = "Toggle the notes per second display. Displays just NPS and max NPS.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("npsDisplay", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("npsDisplay", true),
            },
            {
                Name = "NPS Graph",
                Type = "SingleChoice",
                Explanation = "Toggle the notes per second graph. Displays the NPS over time.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("npsGraph", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("npsGraph", true),
            },
        },
        --
        -----
        -- GLOBAL GRAPHICS OPTIONS
        ["Global Options"] = {
            {
                Name = "Language",
                Type = "SingleChoice",
                Explanation = "Modify the game language.",
                ChoiceGenerator = function()
                    local o = {}
                    for i, l in ipairs(optionData.language.list) do
                        o[#o+1] = {
                            Name = l:upper(),
                            ChosenFunction = function()
                                optionData.language.current = l
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    for i, l in ipairs(optionData.language.list) do
                        if l == optionData.language.current then return i end
                    end
                    return 1
                end,
            },
            {
                Name = "Display Mode",
                Type = "SingleChoice",
                Explanation = "Change the game display mode. Borderless requires that you select your native fullscreen resolution.",
                -- the idea behind Display Mode is to also allow selecting a Display to show the game
                -- it is written into the lua side of the c++ options conf but unused everywhere as far as i know except maybe in linux
                -- so here lets just hardcode windowed/fullscreen until that feature becomes a certain reality
                -- and lets add borderless here so that the options are simplified just a bit
                Choices = {
                    {
                        Name = "Windowed",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("Windowed", true)
                            PREFSMAN:SetPreference("FullscreenIsBorderlessWindow", false)
                        end,
                    },
                    {
                        Name = "Fullscreen",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("Windowed", false)
                            PREFSMAN:SetPreference("FullscreenIsBorderlessWindow", false)
                        end,
                    },
                    {
                        -- funny thing about this preference is that it doesnt force fullscreen
                        -- so you have to pick the right resolution for it to work
                        Name = "Borderless",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("Windowed", false)
                            PREFSMAN:SetPreference("FullscreenIsBorderlessWindow", true)
                        end,
                    }

                },
                ChoiceIndexGetter = function()
                    if PREFSMAN:GetPreference("FullscreenIsBorderlessWindow") then
                        return 3
                    elseif PREFSMAN:GetPreference("Windowed") then
                        return 1
                    else
                        -- fullscreen exclusive
                        return 2
                    end
                end,
            },
            {
                Name = "Aspect Ratio",
                Type = "SingleChoice",
                Explanation = "Change the game aspect ratio.",
                ChoiceGenerator = function()
                    local o = {}
                    for _, ratio in ipairs(optionData.display.ratios) do
                        -- ratio is a fraction, d is denominator and n is numerator
                        local v = ratio.n / ratio.d
                        o[#o+1] = {
                            Name = ratio.n .. ":" .. ratio.d,
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("DisplayAspectRatio", v)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local closestdiff = 100
                    local closestindex = 1
                    local curRatio = PREFSMAN:GetPreference("DisplayAspectRatio")
                    for i, ratio in ipairs(optionData.display.ratios) do
                        -- ratio is a fraction, d is denominator and n is numerator
                        local v = ratio.n / ratio.d
                        local diff = math.abs(v - curRatio)
                        if diff < closestdiff then
                            closestdiff = diff
                            closestindex = i
                        end
                    end
                    return closestindex
                end,
            },
            {
                Name = "Display Resolution",
                Type = "SingleChoice",
                Explanation = "Change the game display resolution.",
                ChoiceGenerator = function()
                    local o = {}

                    -- i trust we didnt generate any duplicates but ....
                    -- ....... hope not
                    for _, resolution in ipairs(optionData.display.resolutions) do
                        -- resolution is a rectangle and contains a width w and a height h
                        o[#o+1] = {
                            Name = resolution.w .. "x" .. resolution.h,
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("DisplayWidth", resolution.w)
                                PREFSMAN:SetPreference("DisplayHeight", resolution.h)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local closestindex = 1
                    local mindist = -1
                    local w = PREFSMAN:GetPreference("DisplayWidth")
                    local h = PREFSMAN:GetPreference("DisplayHeight")
                    for i, resolution in ipairs(optionData.display.resolutions) do
                        -- resolution is a rectangle and contains a width w and a height h
                        local dist = math.sqrt((resolution.w - w)^2 + (resolution.h - h)^2)
                        if mindist == -1 or dist < mindist then
                            mindist = dist
                            closestindex = i
                        end
                    end
                    return closestindex
                end,
            },
            {
                Name = "Refresh Rate",
                Type = "SingleChoice",
                Explanation = "Change the game refresh rate. Set to default in most cases or if any issue occurs.",
                ChoiceGenerator = function()
                    local o = {
                        {
                            Name = "Default",
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("RefreshRate", REFRESH_DEFAULT)
                            end,
                        }
                    }
                    for _, rate in ipairs(optionData.display.refreshRates) do
                        o[#o+1] = {
                            Name = tostring(rate),
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("RefreshRate", rate)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local rate = PREFSMAN:GetPreference("RefreshRate")
                    -- first choice element is this
                    if rate == REFRESH_DEFAULT then return 1 end

                    -- add 1 to index if found
                    for i, r in ipairs(optionData.display.refreshRates) do
                        if rate == r then return i+1 end
                    end

                    -- default
                    return 1
                end,
            },
            {
                Name = "Display Color Depth",
                Type = "SingleChoice",
                Explanation = "Change the color depth of the game according to your display. Usually not worth changing.",
                Choices = {
                    basicNamedPreferenceChoice("DisplayColorDepth", "16bit", 16),
                    basicNamedPreferenceChoice("DisplayColorDepth", "32bit", 32),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("DisplayColorDepth")
                    if v == 16 then return 1
                    elseif v == 32 then return 2
                    end
                    return 1
                end,
            },
            {
                Name = "Force High Resolution Textures",
                Type = "SingleChoice",
                Explanation = "Force high resolution textures. Turning this off disables the (doubleres) image tag.",
                Choices = choiceSkeleton("Yes", "No"),
                Directions = preferenceToggleDirections("HighResolutionTextures", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("HighResolutionTextures", true),
            },
            {
                Name = "Texture Resolution",
                Type = "SingleChoice",
                Explanation = "Modify general texture resolution. Lower number will lower quality but may increase FPS.",
                Choices = {
                    basicNamedPreferenceChoice("MaxTextureResolution", "256", 256),
                    basicNamedPreferenceChoice("MaxTextureResolution", "512", 512),
                    basicNamedPreferenceChoice("MaxTextureResolution", "1024", 1024),
                    basicNamedPreferenceChoice("MaxTextureResolution", "2048", 2048),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("MaxTextureResolution")
                    if v == 256 then return 1
                    elseif v == 512 then return 2
                    elseif v == 1024 then return 3
                    elseif v == 2048 then return 4
                    end
                    return 1
                end,
            },
            {
                Name = "Texture Color Depth",
                Type = "SingleChoice",
                Explanation = "Change the color depth of the textures in the game. Usually not worth changing.",
                Choices = {
                    basicNamedPreferenceChoice("TextureColorDepth", "16bit", 16),
                    basicNamedPreferenceChoice("TextureColorDepth", "32bit", 32),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("TextureColorDepth")
                    if v == 16 then return 1
                    elseif v == 32 then return 2
                    end
                    return 1
                end,
            },
            {
                Name = "Movie Color Depth",
                Type = "SingleChoice",
                Explanation = "Change the color depth of the movie textures in the game. Usually not worth changing.",
                Choices = {
                    basicNamedPreferenceChoice("MovieColorDepth", "16bit", 16),
                    basicNamedPreferenceChoice("MovieColorDepth", "32bit", 32),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("MovieColorDepth")
                    if v == 16 then return 1
                    elseif v == 32 then return 2
                    end
                    return 1
                end,
            },
            {
                Name = "VSync",
                Type = "SingleChoice",
                Explanation = "Restrict the game refresh rate and FPS to the refresh rate you have set.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("Vsync", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("Vsync", true),
            },
            {
                Name = "Instant Search",
                Type = "SingleChoice",
                Explanation = "Song search behavior - turning this on will instantly update the song wheel as you type in song search.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirections("instantSearch", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetter("instantSearch", true),
            },
            {
                Name = "Fast Note Rendering",
                Type = "SingleChoice",
                Explanation = "Optimize gameplay note rendering. Disable snap based noteskin features (not snaps themselves). Major boost to FPS.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("FastNoteRendering", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("FastNoteRendering", true),
            },
            {
                Name = "Show Stats",
                Type = "SingleChoice",
                Explanation = "Show FPS display on screen.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("ShowStats", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("ShowStats", true),
            },
        },
        --
        -----
        -- THEME OPTIONS
        ["Theme Options"] = {
            --[[
            {
                Name = "Theme",
                Type = "SingleChoice",
                Explanation = "Change the overall skin of the game.",
                ChoiceGenerator = function()
                    local o = {}
                    for _, name in ipairs(THEME:GetSelectableThemeNames()) do
                        o[#o+1] = {
                            Name = name,
                            ChosenFunction = function()
                                optionData.pickedTheme = name
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local cur = optionData.pickedTheme
                    for i, name in ipairs(THEME:GetSelectableThemeNames()) do
                        if name == cur then return i end
                    end
                    return 1
                end,
            },
            ]]
            {
                Name = "Music Wheel Position",
                Type = "SingleChoice",
                Explanation = "Set the side of the screen for the music wheel.",
                Choices = choiceSkeleton("Left", "Right"),
                Directions = optionDataToggleDirectionsFUNC("wheelPosition", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("wheelPosition", true),
            },
            {
                Name = "Music Wheel Banners",
                Type = "SingleChoice",
                Explanation = "Toggle the banners on the music wheel.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("wheelBanners", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("wheelBanners", true),
            },
            {
                Name = "Video Banners",
                Type = "SingleChoice",
                Explanation = "Toggle allowing video banners to play at all on the wheel and other locations in music select.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("videoBanners", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("videoBanners", true),
            },
            {
                Name = "Show Backgrounds",
                Type = "SingleChoice",
                Explanation = "Toggle showing backgrounds everywhere.",
                Choices = choiceSkeleton("Yes", "No"),
                Directions = optionDataToggleDirectionsFUNC("showBackgrounds", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("showBackgrounds", true),
            },
            {
                Name = "Allow Background Changes",
                Type = "SingleChoice",
                Explanation = "Toggle gameplay backgrounds changing.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("allowBGChanges", false, true),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("allowBGChanges", false),
            },
            {
                Name = "Easter Eggs & Toasties",
                Type = "SingleChoice",
                Explanation = "Toggle showing secret jokes and toasties.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("EasterEggs", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("EasterEggs", true),
            },
            {
                Name = "Music Visualizer",
                Type = "SingleChoice",
                Explanation = "Toggle showing the visualizer in the song select screen.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirectionsFUNC("showVisualizer", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("showVisualizer", true),
            },
            {
                Name = "Mid Grades",
                Type = "SingleChoice",
                Explanation = "Toggle showing the grades in between the major grades. Requires game restart.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("UseMidGrades", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("UseMidGrades", true),
            },
            {
                Name = "SSRNorm Sort",
                Type = "SingleChoice",
                Explanation = "Toggle automatically sorting by and defaulting to the SSRNorm globally. The SSRNorm is the Judge 4 value of a highscore. Requires game restart.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("SortBySSRNormPercent", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("SortBySSRNormPercent", true),
            },
            {
                Name = "Show Lyrics",
                Type = "SingleChoice",
                Explanation = "Toggle showing lyrics for songs which contain compatible .lrc files.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("ShowLyrics", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("ShowLyrics", true),
            },
            {
                Name = "Transliteration",
                Type = "SingleChoice",
                Explanation = "Toggle showing author-defined translations on song metadata fields.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = {
                    Toggle = function()
                        if PREFSMAN:GetPreference("ShowNativeLanguage") then
                            PREFSMAN:SetPreference("ShowNativeLanguage", false)
                        else
                            PREFSMAN:SetPreference("ShowNativeLanguage", true)
                        end
                        MESSAGEMAN:Broadcast("DisplayLanguageChanged")
                    end,
                },
                ChoiceIndexGetter = preferenceToggleIndexGetter("ShowNativeLanguage", true),
            },
            {
                Name = "Tip Type",
                Type = "SingleChoice",
                Explanation = "Change the quips shown at the bottom of the evaluation screen.",
                Choices = choiceSkeleton("Tips", "Quotes"),
                Directions = optionDataToggleDirectionsFUNC("tipType", 1, 2),
                ChoiceIndexGetter = optionDataToggleIndexGetterFUNC("tipType", 1),
            },
            {
                Name = "Set BG Fit Mode",
                Type = "SingleChoice",
                Explanation = "Change the cropping strategy of background images.",
                ChoiceGenerator = function()
                    local o = {}
                    for _, fit in ipairs(BackgroundFitMode) do
                        o[#o+1] = {
                            Name = THEME:GetString("ScreenSetBGFit", ToEnumShortString(fit)),
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("BackgroundFitMode", ToEnumShortString(fit))
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local cur = PREFSMAN:GetPreference("BackgroundFitMode")
                    for i, fit in ipairs(BackgroundFitMode) do
                        if ToEnumShortString(fit) == cur then
                            return i
                        end
                    end
                    return 1
                end,
            },
            {
                Name = "Color Config",
                Type = "Button",
                Explanation = "Modify the colors of this theme.",
                Choices = {
                    {
                        Name = "Color Config",
                        ChosenFunction = function()
                            -- activate color config screen
                            MESSAGEMAN:Broadcast("ShowSettingsAlt", {name = "Color Config"})
                        end,
                    },
                }
            },
            {
                Name = "Asset Settings",
                Type = "Button",
                Explanation = "Set your avatar, judgments, and toasty.",
                Choices = {
                    {
                        Name = "Asset Settings",
                        ChosenFunction = function()
                            -- activate asset settings screen
                            MESSAGEMAN:Broadcast("PlayerInfoFrameTabSet", {tab = "AssetSettings", prevScreen = "Settings"})
                        end,
                    },
                }
            },
        },
        --
        -----
        -- SOUND OPTIONS
        ["Sound Options"] = {
            {
                Name = "Volume",
                Type = "SingleChoice",
                Explanation = "All sound volume.",
                Directions = {
                    Left = function()
                        local x = PREFSMAN:GetPreference("SoundVolume")
                        x = notShit.round(x - 0.01, 3)
                        if x < 0 then x = 1 end
                        SOUND:SetVolume(notShit.round(x, 3))
                    end,
                    Right = function()
                        local x = PREFSMAN:GetPreference("SoundVolume")
                        x = notShit.round(x + 0.01, 3)
                        if x > 1 then x = 0 end
                        SOUND:SetVolume(notShit.round(x, 3))
                    end,
                },
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("SoundVolume") * 100, 0) .. "%"
                end,
            },
            {
                Name = "Menu Sounds",
                Type = "SingleChoice",
                Explanation = "Toggle sounds on menu items.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("MuteActions", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("MuteActions", false),
            },
            {
                Name = "Mine Sounds",
                Type = "SingleChoice",
                Explanation = "Toggle sounds for mine explosions.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("EnableMineHitSound", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("EnableMineHitSound", true),
            },
            {
                Name = "Pitch on Rates",
                Type = "SingleChoice",
                Explanation = "Toggle pitch changes for songs when using rates.",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("EnablePitchRates", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("EnablePitchRates", true),
            },
            {
                Name = "Calibrate Audio Sync",
                Type = "Button",
                Explanation = "Calibrate the audio sync for the entire game.",
                Choices = {
                    {
                        Name = "Calibrate Audio Sync",
                        ChosenFunction = function()
                            -- go to machine sync screen
                            SCUFF.screenAfterSyncMachine = SCREENMAN:GetTopScreen():GetName()
                            SCREENMAN:set_input_redirected(PLAYER_1, false)
                            SCREENMAN:SetNewScreen("ScreenGameplaySyncMachine")
                        end,
                    },
                },
            },
        },
        --
        -----
        -- INPUT OPTIONS
        ["Input Options"] = {
            {
                Name = "Back Delayed",
                Type = "SingleChoice",
                Explanation = "Modify the behavior of the back button in gameplay.",
                Choices = choiceSkeleton("Hold", "Instant"),
                Directions = preferenceToggleDirections("DelayedBack", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("DelayedBack", true),
            },
            {
                Name = "Input Debounce Time",
                Type = "SingleChoice",
                Explanation = "Set the amount of time required between each repeated input.",
                Directions = preferenceIncrementDecrementDirections("InputDebounceTime", 0, 1, 0.01),
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("InputDebounceTime"), 2) .. "s"
                end,
            },
            {
                Name = "Test Input",
                Type = "Button",
                Explanation = "Enter a screen to test all input devices.",
                Choices = {
                    {
                        Name = "Test Input",
                        ChosenFunction = function()
                            -- go to test input screen
                            SCUFF.screenAfterSyncMachine = SCREENMAN:GetTopScreen():GetName()
                            SCREENMAN:set_input_redirected(PLAYER_1, false)
                            SCREENMAN:SetNewScreen("ScreenTestInput")
                        end,
                    }
                }
            },
        },
        --
        -----
        -- PROFILE OPTIONS
        ["Profile Options"] = {
            {
                Name = "Create Profile",
                Type = "Button",
                Explanation = "Create a new profile.",
                Choices = {
                    {
                        Name = "Create Profile",
                        ChosenFunction = function()
                            -- make a profile
                        end,
                    }
                }
            },
            {
                Name = "Rename Profile",
                Type = "Button",
                Explanation = "Rename an existing profile.",
                Choices = {
                    {
                        Name = "Rename Profile",
                        ChosenFunction = function()
                            -- rename a profile
                        end,
                    }
                }
            },
        },
    }
    -- check for choice generators on any option definitions and execute them
    for categoryName, categoryDefinition in pairs(optionDefs) do
        for i, optionDef in ipairs(categoryDefinition) do
            if optionDef.Choices == nil and optionDef.ChoiceGenerator ~= nil then
                optionDefs[categoryName][i].Choices = optionDef.ChoiceGenerator()
            end
        end
    end

    -- internal tracker for where the cursor can be and has been within a row
    -- the index of each entry is simply the row number on the right side of the screen
    -- for a context switch to the left, those are managed by each respective panel separately
    -- format: (each entry)
    --[[{
            NumChoices = x, -- number of choices, simply. 0 means this is a button to press. 1 is a SingleChoice. N is MultiChoice
            HighlightedChoice = x, -- position of the highlighted choice. 1 for Single/Button. N for MultiChoice. Account for the pagination (in other visual representations, not here).
            LinkedItem = x, -- either a category name or an optionDef
        } ]]
    local availableCursorPositions = {}
    local rightPaneCursorPosition = 1 -- current index of the above table

    -- container function/frame for all option rows
    local function createOptionRows()
        -- Unfortunate design choice:
        -- For every option row, we are going to place every single possible row type.
        -- This means there's a ton of invisible elements.
        -- Is this worth doing? This is better than telling the C++ to let us generate and destroy arbitrary Actors at runtime
        -- (I consider this dangerous and also too complex to implement)
        -- So instead we "carefully" manage all pieces of an option row...
        -- Luckily we can be intelligent about wasting space.
        -- First, we parse all of the optionData to see which choices need what elements.
        -- We pass that information on to the rows (we can precalculate which rows have what choices)
        -- This way we can avoid generating Actor elements which will never be used in a row

        -- Alternative to doing the above and below:
        -- just use ActorFrame.RemoveChild and ActorFrame.AddChildFromPath

        -- table of row index keys to lists of row types
        -- valid row types are in the giant option definition comment block
        local rowTypes = {}
        -- table of row index keys to counts of how many text objects to generate
        -- this should correlate to how many choices are possible in a row on any option page
        local rowChoiceCount = {}
        for _, optionPage in ipairs(pageNames) do
            for i, categoryName in ipairs(optionPageCategoryLists[optionPage]) do
                local categoryDefinition = optionDefs[categoryName]

                -- declare certain rows are categories
                -- (current row and the remaining rows after the set of options in this category)
                if rowTypes[i] ~= nil then
                    rowTypes[i]["Category"] = true
                else
                    rowTypes[i] = {Category = true}
                end
                for ii = (i+1), (#optionPageCategoryLists[optionPage]) do
                    local categoryRowIndex = ii + #categoryDefinition
                    if rowTypes[categoryRowIndex] ~= nil then
                        rowTypes[categoryRowIndex]["Category"] = true
                    else
                        rowTypes[categoryRowIndex] = {Category = true}
                    end
                end

                for j, optionDef in ipairs(categoryDefinition) do
                    local rowIndex = j + i -- skip the rows for option category names

                    -- option types for every row
                    if rowTypes[rowIndex] ~= nil then
                        rowTypes[rowIndex][optionDef.Type] = true
                    else
                        rowTypes[rowIndex] = {[optionDef.Type] = true}
                    end

                    -- option choice count for every row
                    local rcc = rowChoiceCount[rowIndex]
                    if rcc == nil then
                        rowChoiceCount[rowIndex] = 0
                        rcc = 0
                    end
                    local defcount = #(optionDef.Choices or {})
                    -- the only case we should show multiple choices is for MultiChoice...
                    if optionDef.Type ~= "MultiChoice" then defcount = 1 end
                    if rcc < defcount then
                        rowChoiceCount[rowIndex] = defcount
                    end
                end
            end
        end

        -- updates the explanation text.
        local function updateExplainText(self)
            if self.defInUse ~= nil and self.defInUse.Explanation ~= nil then
                if explanationHandle ~= nil then
                    if explanationHandle.txt ~= self.defInUse.Explanation then
                        explanationHandle:playcommand("SetExplanation", {text = self.defInUse.Explanation})
                    end
                else
                    explanationHandle:playcommand("SetExplanation", {text = ""})
                end
            else
                explanationHandle:playcommand("SetExplanation", {text = ""})
            end
        end

        ----- state variables, dont mess
        -- currently selected options page - from pageNames
        local selectedPageName = pageNames[1] -- default to first
        local selectedPageDef = optionPageCategoryLists[selectedPageName]
        -- currently opened option category - from optionPageCategoryLists
        local openedCategoryName = selectedPageDef[1] -- default to first
        local openedCategoryDef = optionDefs[openedCategoryName]
        -- index of the opened option category to know the index of the first valid option row to assign option defs
        local openedCategoryIndex = 1
        local optionRowContainer = nil

        -- fills out availableCursorPositions based on current conditions of the above variables
        local function generateCursorPositionMap()
            availableCursorPositions = {}
            rightPaneCursorPosition = 1

            -- theres a list of categories on the page (selectedPageDef)
            -- theres a category that is opened on this page (openedCategoryDef)
            -- add each category up to and including the opened category to the list
            -- then add each option to the list
            -- then add the rest of the categories to the list
            -- (this is the same as how we display the options below somewhere)
            -- we assume openedCategoryIndex is correct at all times
            -- also assume you cannot close an opened Category except by opening a different category or page

            -- add each category up to and including the opened category
            for i = 1, openedCategoryIndex do
                local opened = false
                if i == openedCategoryIndex then opened = true end
                availableCursorPositions[#availableCursorPositions+1] = {
                    NumChoices = 0,
                    HighlightedChoice = 1,
                    LinkedItem = {
                        Opened = opened,
                        Name = selectedPageDef[i],
                    },
                }
            end

            -- put the cursor on the first option after the opened category
            rightPaneCursorPosition = openedCategoryIndex+1

            -- add each option in the category
            for i = 1, #openedCategoryDef do
                local def = openedCategoryDef[i]
                local nchoices = 0
                if def.Type == "Button" then
                    nchoices = 0
                elseif def.Type == "SingleChoice" or def.Type == "SingleChoiceModifier" then
                    -- naturally we would let people hover and press the second set of buttons in SingleChoiceModifier but i would rather force that to be a ctrl+direction instead
                    -- that seems a little more fluid than moving to the directional button and pressing it
                    nchoices = 1
                elseif def.Type == "MultiChoice" then
                    nchoices = #(def.Choices or {})
                end
                availableCursorPositions[#availableCursorPositions+1] = {
                    NumChoices = nchoices,
                    HighlightedChoice = 1,
                    LinkedItem = def,
                }
            end

            -- add each category remaining after the last option
            for i = openedCategoryIndex+1, #selectedPageDef do
                availableCursorPositions[#availableCursorPositions+1] = {
                    NumChoices = 0,
                    HighlightedChoice = 1,
                    LinkedItem = {
                        Opened = false,
                        Name = selectedPageDef[i],
                    }
                }
            end

            -- and if things turn out broken at this point it isnt my fault
        end

        -- find the ActorFrame for an OptionRow by an Option Name
        local function getRowForCursorByName(name)
            if optionRowContainer == nil then return nil end

            for i, row in ipairs(optionRowContainer:GetChildren()) do
                if row.defInUse ~= nil and row.defInUse.Name == name then
                    return row
                end
            end
            return nil
        end

        -- find the (cursor) index of an OptionRow by an Option Name
        local function getRowIndexByName(name)
            if availableCursorPositions == nil then return nil end

            for i, cursorRowDef in ipairs(availableCursorPositions) do
                if cursorRowDef.LinkedItem ~= nil and cursorRowDef.LinkedItem.Name == name then
                    return i
                end
            end
            return 1
        end

        -- find the ActorFrame for the OptionRow that is currently hovered by the cursor
        local function getRowForCursorByCurrentPosition()
            -- correct error or just do index wrap around
            if rightPaneCursorPosition > #availableCursorPositions then rightPaneCursorPosition = 1 end
            if rightPaneCursorPosition < 1 then rightPaneCursorPosition = #availableCursorPositions end

            return optionRowContainer:GetChild("OptionRow_"..rightPaneCursorPosition)
        end

        local function getActorForCursorToHoverByCurrentConditions()
            local optionRowFrame = getRowForCursorByCurrentPosition()
            if optionRowFrame == nil then ms.ok("BAD CURSOR REPORT TO DEVELOPER") return end
            local optionRowDef = optionRowFrame.defInUse
            if optionRowDef == nil then ms.ok("BAD CURSOR ROWDEF REPORT TO DEVELOPER") return end

            -- place the cursor to highlight this item (usually ActorFrame containing BitmapText as child "Text")
            local actorToHover = nil

            -- based on the type, place the cursor in specific positions (the positions are memorized in availableCursorPositions too)
            if optionRowDef.Type == nil then
                -- optionDefs without Type should always be Category defs
                -- simply hover the title in this case
                -- pressing enter would open the category unless it is already opened
                actorToHover = optionRowFrame:GetChild("TitleText")
            else
                -- these are Option defs, not Categories
                if optionRowDef.Type == "Button" then
                    -- Button hovers the title text
                    -- pressing enter on it is a single action
                    actorToHover = optionRowFrame:GetChild("TitleText")
                elseif optionRowDef.Type == "SingleChoice" or optionRowDef.Type == "SingleChoiceModifier" then
                    -- SingleChoice[Modifier] hovers the single visible choice
                    -- pressing enter does nothing, only left and right function
                    actorToHover = optionRowFrame:safeGetChild("ChoiceFrame", "Choice_1")
                elseif optionRowDef.Type == "MultiChoice" then
                    -- MultiChoice hovers one of the visible choices
                    -- the visible choice is dependent on the value of availableCursorPositions[i].HighlightedChoice
                    -- account here, rather than in stored data, for pagination of the choices
                    -- otherwise a dead choice is picked and we look dumb
                    local cursorPosDef = availableCursorPositions[rightPaneCursorPosition]
                    local pagesize = math.min(maxChoicesVisibleMultiChoice, cursorPosDef.NumChoices)
                    if pagesize > cursorPosDef.HighlightedChoice then
                        -- if the cursor is on the first page no special math required
                        actorToHover = optionRowFrame:safeGetChild("ChoiceFrame", "Choice_"..cursorPosDef.HighlightedChoice)
                    else
                        -- if the cursor is not on the first page check to see where it lands
                        -- (i already spent 5 minutes thinking on the math for this and i got bored so what follows is the best you get)
                        local choiceIndex = cursorPosDef.HighlightedChoice % pagesize
                        if choiceIndex == 0 then choiceIndex = pagesize end -- really intuitive, right?
                        actorToHover = optionRowFrame:safeGetChild("ChoiceFrame", "Choice_"..choiceIndex)
                    end
                else
                    ms.ok("BAD CURSOR ROWDEF TYPE REPORT TO DEVELOPER")
                    return nil
                end
            end
            return actorToHover
        end

        -- place the cursor based on the current conditions of rightPaneCursorPosition and availableCursorPositions
        local function setCursorPositionByCurrentConditions()
            local optionRowFrame = getRowForCursorByCurrentPosition()
            if optionRowFrame == nil then ms.ok("BAD CURSOR REPORT TO DEVELOPER") return end
            local optionRowDef = optionRowFrame.defInUse
            if optionRowDef == nil then ms.ok("BAD CURSOR ROWDEF REPORT TO DEVELOPER") return end
            local actorToHover = getActorForCursorToHoverByCurrentConditions()

            if actorToHover == nil then
                ms.ok("BAD CURSOR PLACEMENT LOGIC OR DEF REPORT TO DEVELOPER")
                return
            end

            -- at the time of writing all actorToHover should be an ActorFrame with a child "Text"
            -- this is a TextButton
            local txt = actorToHover:GetChild("Text")
            local cursorActor = optionRowContainer:GetChild("OptionCursor")
            local xp = txt:GetTrueX() - optionRowContainer:GetTrueX()
            local beforeYPos = cursorActor:GetY()

            -- these positions should be relative to optionRowContainer so it should work out fine
            cursorActor:finishtweening()
            cursorActor:smooth(animationSeconds)
            cursorActor:xy(xp, optionRowFrame:GetY() + actorToHover:GetY() + txt:GetY())
            cursorActor:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * 1.5)

            -- tell the game that we moved the option cursor to this row
            -- dont care if it didnt move
            MESSAGEMAN:Broadcast("OptionCursorUpdated", {name = optionRowDef.Name, choiceName = txt:GetText()})
        end

        -- function for pressing enter wherever the cursor is
        local function invokeCurrentCursorPosition()
            local actorToHover = getActorForCursorToHoverByCurrentConditions()
            local cursorPosDef = availableCursorPositions[rightPaneCursorPosition]

            if actorToHover == nil or cursorPosDef == nil or cursorPosDef.LinkedItem == nil then return end
            local linkdef = cursorPosDef.LinkedItem

            if linkdef.Opened == true then
                -- this means it is an opened category
                -- do nothing.
            elseif (linkdef.Opened ~= nil and linkdef.Opened == false) or linkdef.Type == "Button" then
                -- this means it is a closed category or it is a Button
                -- invoke on the text
                actorToHover:playcommand("Invoke")
            elseif linkdef.Type == "SingleChoice" or linkdef.Type == "SingleChoiceModifier" then
                -- this means it is a SingleChoice or SingleChoiceModifier
                -- do nothing.
            elseif linkdef.Type == "MultiChoice" then
                -- this means it is a MultiChoice
                -- invoke on the hovered Choice
                actorToHover:playcommand("Invoke")
            else
                -- ????
            end
        end

        -- function to set the cursor VERTICAL position
        local function setCursorPos(n)
            -- do nothing if not moving cursor
            if rightPaneCursorPosition == n then return end
            rightPaneCursorPosition = n

            local rowframe = getRowForCursorByCurrentPosition()
            updateExplainText(rowframe)

            -- update visible cursor
            setCursorPositionByCurrentConditions()
        end

        -- move the cursor position by a distance if needed
        local function changeCursorPos(n)
            local newpos = rightPaneCursorPosition + n
            -- not worth doing math to figure out if you moved 5 down from the last slide to put you on the 4th option from the top ......
            if newpos > #availableCursorPositions then newpos = 1 end
            if newpos < 1 then newpos = #availableCursorPositions end
            setCursorPos(newpos)
        end

        -- move the cursor left or right (IM OUT OF FUNCTION NAMES AND DIDNT PLAN TO MAKE THIS ONE UNTIL RIGHT NOW DONT KNOW WHAT I WAS THINKING NOT SORRY)
        local function cursorLateralMovement(n, useMultiplier)
            local currentCursorRowDef = availableCursorPositions[rightPaneCursorPosition]
            if currentCursorRowDef == nil then return end
            local currentCursorRowOptionDef = currentCursorRowDef.LinkedItem

            if currentCursorRowOptionDef == nil or currentCursorRowOptionDef.Type == nil or currentCursorRowOptionDef.Type == "Button" then
                -- Buttons and Categories dont have lateral movement actions
                return
            elseif currentCursorRowOptionDef.Type == "SingleChoice" then
                -- moving a SingleChoice left or right actually invokes it (same as clicking the arrows)
                local optionRowFrame = getRowForCursorByCurrentPosition()
                local invoker = nil
                if n > 0 then
                    -- run invoke on the right single arrow
                    invoker = optionRowFrame:GetChild("RightBigTriangleFrame")
                else
                    -- run invoke on the left single arrow
                    invoker = optionRowFrame:GetChild("LeftBigTriangleFrame")
                end
                if invoker == nil then ms.ok("TRIED TO MOVE OPTION WITHOUT ARROWS. HOW? CONTACT DEVELOPER") return end
                invoker:playcommand("Invoke")
            elseif currentCursorRowOptionDef.Type == "SingleChoiceModifier" then
                -- moving a SingleChoiceModifier left or right actually invokes it (same as clicking the arrows)
                local optionRowFrame = getRowForCursorByCurrentPosition()
                local invoker = nil
                if useMultiplier then
                    if n > 0 then
                        -- run invoke on the right double arrow
                        invoker = optionRowFrame:GetChild("RightTrianglePairFrame")
                    else
                        -- run invoke on the left double arrow
                        invoker = optionRowFrame:GetChild("LeftTrianglePairFrame")
                    end
                else
                    if n > 0 then
                        -- run invoke on the right single arrow
                        invoker = optionRowFrame:GetChild("RightBigTriangleFrame")
                    else
                        -- run invoke on the left single arrow
                        invoker = optionRowFrame:GetChild("LeftBigTriangleFrame")
                    end
                end
                if invoker == nil then ms.ok("TRIED TO MOVE OPTION WITHOUT ARROWS. HOW? CONTACT DEVELOPER") return end
                invoker:playcommand("Invoke")
            elseif currentCursorRowOptionDef.Type == "MultiChoice" then
                -- moving a MultiChoice does not invoke it, only moves the cursor. Enter would invoke on a Choice instead
                local newpos = currentCursorRowDef.HighlightedChoice + n
                -- wrap around
                if newpos > currentCursorRowDef.NumChoices then newpos = 1 end
                if newpos < 1 then newpos = currentCursorRowDef.NumChoices end
                currentCursorRowDef.HighlightedChoice = newpos

                -- heres the weird thing:
                -- if we move the cursor here so that it ends up on another page, we need to redraw the stuff
                -- so do a big brain and invoke the appropriate big triangle if that scenario arises
                local optionRowFrame = getRowForCursorByCurrentPosition()
                local validLower = 1 + (optionRowFrame.choicePage-1) * maxChoicesVisibleMultiChoice
                local validUpper = optionRowFrame.choicePage * maxChoicesVisibleMultiChoice
                if validUpper > #currentCursorRowOptionDef.Choices then validUpper = #currentCursorRowOptionDef.Choices end -- if last page missing elements
                if newpos < validLower or newpos > validUpper then
                    -- changed page, find it
                    local newpage = math.ceil(newpos / math.min(#currentCursorRowOptionDef.Choices, maxChoicesVisibleMultiChoice))
                    optionRowFrame:playcommand("SetChoicePage", {page = newpage})
                else
                    -- didnt change page probably
                end
            else
                -- impossible?
                return
            end

            -- update visible cursor
            setCursorPositionByCurrentConditions()
        end

        -- shortcuts for changeCursorPos
        local function cursorUp(n)
            changeCursorPos(-n)
        end
        local function cursorDown(n)
            changeCursorPos(n)
        end
        -- shortcuts for cursorLateralMovement
        local function cursorLeft(n, useMultiplier)
            cursorLateralMovement(-n, useMultiplier)
        end
        local function cursorRight(n, useMultiplier)
            cursorLateralMovement(n, useMultiplier)
        end

        -- function specifically for mouse hovering moving the cursor to run logic found in the above functions and more
        local function setCursorVerticalHorizontalPos(rowFrame, choice)
            if rowFrame == nil or rowFrame.defInUse == nil then return end -- apparently these can be nil? DONT KNOW HOW THATS PROBABLY REALLY BAD
            local n = getRowIndexByName(rowFrame.defInUse.Name)
            if choice == nil then choice = availableCursorPositions[n].HighlightedChoice end

            -- dont needlessly update
            if rightPaneCursorPosition == n and availableCursorPositions[n].HighlightedChoice == choice then
                return
            end

            rightPaneCursorPosition = n
            local rowframe = getRowForCursorByCurrentPosition()
            updateExplainText(rowframe)
            availableCursorPositions[n].HighlightedChoice = choice
            setCursorPositionByCurrentConditions()
        end

        -- putting these functions here to save on space below, less copy pasting, etc
        local function onHover(self)
            if self:IsInvisible() then return end
            self:diffusealpha(buttonHoverAlpha)
            local rowframe = self:GetParent()
            updateExplainText(rowframe)

            -- only the category triangle uses this which means the choice is 1
            setCursorVerticalHorizontalPos(rowframe, 1)
        end
        local function onUnHover(self)
            if self:IsInvisible() then return end
            self:diffusealpha(1)
        end
        local function onHoverParent(self)
            if self:GetParent():IsInvisible() then return end
            self:GetParent():diffusealpha(buttonHoverAlpha)
            local rowframe = self:GetParent():GetParent()
            updateExplainText(rowframe)

            -- only triangles use this which means use the choice that is already set
            setCursorVerticalHorizontalPos(rowframe, nil)
        end
        local function onUnHoverParent(self)
            if self:GetParent():IsInvisible() then return end
            self:GetParent():diffusealpha(1)
        end
        local function broadcastOptionUpdate(optionDef, choiceIndex)
            if type(choiceIndex) == "number" then
                if optionDef.Choices ~= nil and optionDef.Choices[choiceIndex] ~= nil then
                    -- a normal SingleChoice or SingleChoiceModifier
                    MESSAGEMAN:Broadcast("OptionUpdated", {name = optionDef.Name, choiceName = optionDef.Choices[choiceIndex].Name})
                else
                    -- a non-indexed option being updated directly
                    MESSAGEMAN:Broadcast("OptionUpdated", {name = optionDef.Name, choiceName = choiceIndex})
                end
            elseif type(choiceIndex) == "string" then
                -- a non-indexed option being updated directly
                MESSAGEMAN:Broadcast("OptionUpdated", {name = optionDef.Name, choiceName = choiceIndex})
            elseif type(choiceIndex) == "table" then
                -- in this case it is a MultiChoice being selected
                if choiceIndex.Name ~= nil then
                    MESSAGEMAN:Broadcast("OptionUpdated", {name = optionDef.Name, choiceName = choiceIndex.Name})
                end
            end
        end
        --

        local t = Def.ActorFrame {
            Name = "OptionRowContainer",
            InitCommand = function(self)
                self:y(actuals.TopLipHeight * 2 + actuals.OptionTextListTopGap)
                optionRowContainer = self
                self:playcommand("OpenPage", {page = 1})
            end,
            BeginCommand = function(self)
                local snm = SCREENMAN:GetTopScreen():GetName()
                local anm = self:GetName()

                -- cursor input management
                CONTEXTMAN:RegisterToContextSet(snm, "Settings", anm)
                CONTEXTMAN:ToggleContextSet(snm, "Settings", false)

                SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                    -- if locked out, dont allow
                    if not CONTEXTMAN:CheckContextSet(snm, "Settings") then return end
                    if event.type ~= "InputEventType_Release" then -- allow Repeat and FirstPress
                        local gameButton = event.button
                        local key = event.DeviceInput.button
                        local up = gameButton == "Up" or gameButton == "MenuUp"
                        local down = gameButton == "Down" or gameButton == "MenuDown"
                        local right = gameButton == "MenuRight" or gameButton == "Right"
                        local left = gameButton == "MenuLeft" or gameButton == "Left"
                        local enter = gameButton == "Start"
                        local ctrl = INPUTFILTER:IsBeingPressed("left ctrl") or INPUTFILTER:IsBeingPressed("right ctrl")
                        local previewbutton = key == "DeviceButton_space"
                        local back = key == "DeviceButton_escape"

                        if up then
                            cursorUp(1)
                        elseif down then
                            cursorDown(1)
                        elseif left then
                            cursorLeft(1, ctrl)
                        elseif right then
                            cursorRight(1, ctrl)
                        elseif enter then
                            invokeCurrentCursorPosition()
                        elseif previewbutton then
                            -- allow turning off chart preview if on
                            -- allow turning it on if not in a position where doing so is impossible
                            if SCUFF.showingPreview then
                                MESSAGEMAN:Broadcast("PlayerInfoFrameTabSet", {tab = "Settings"})
                            elseif not SCUFF.showingPreview and not SCUFF.showingKeybinds and not SCUFF.showingNoteskins and not SCUFF.showingColor then
                                MESSAGEMAN:Broadcast("ShowSettingsAlt", {name = "Preview"})
                            end
                        elseif back then
                            -- shortcut to exit back to general
                            MESSAGEMAN:Broadcast("GeneralTabSet")
                        else
                            -- nothing happens
                            return
                        end
                    end
                end)

                -- initial cursor load
                generateCursorPositionMap()
                setCursorPositionByCurrentConditions()
                updateExplainText(getRowForCursorByCurrentPosition())
            end,
            OptionTabSetMessageCommand = function(self, params)
                self:playcommand("OpenPage", params)
            end,
            OpenPageCommand = function(self, params)
                local pageIndexToOpen = params.page
                selectedPageName = pageNames[pageIndexToOpen]
                selectedPageDef = optionPageCategoryLists[selectedPageName]
                self:playcommand("OpenCategory", {categoryName = selectedPageDef[1]})
            end,
            OpenCategoryCommand = function(self, params)
                local categoryNameToOpen = params.categoryName
                openedCategoryName = categoryNameToOpen
                openedCategoryDef = optionDefs[openedCategoryName]
                self:playcommand("UpdateRows")
            end,
            UpdateRowsCommand = function(self)
                openedCategoryIndex = 1
                for i = 1, #selectedPageDef do
                    if selectedPageDef[i] == openedCategoryName then
                        openedCategoryIndex = i
                    end
                end

                -- update all rows, redraw
                for i = 1, optionRowCount do
                    local row = self:GetChild("OptionRow_"..i)
                    row:playcommand("UpdateRow")
                end

                -- redrawing the rows means need to update the mapping of cursor positions
                -- this resets the cursor position also
                -- must take place after UpdateRow because cursor position is reliant on the row choice positions
                generateCursorPositionMap()
                setCursorPositionByCurrentConditions()
                updateExplainText(getRowForCursorByCurrentPosition())
            end,

            Def.Quad {
                Name = "OptionCursor",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoomto(100,100)
                    self:diffusealpha(0.6)
                    registerActorToColorConfigElement(self, "options", "Cursor")
                end,
            }
        }
        local function createOptionRow(i)
            local types = rowTypes[i] or {}
            -- SingleChoice             1 arrow, 1 choice
            -- SingleChoiceModifier     2 arrow, 1 choice
            -- MultiChoice              2 arrow, N choices
            -- Button                   no arrow, 1 choice
            -- generate elements based on how many choices and how many directional arrows are needed
            local arrowCount = (types["SingleChoiceModifier"] or types["MultiChoice"]) and 2 or (types["SingleChoice"] and 1 or 0)
            local choiceCount = rowChoiceCount[i] or 0

            local optionDef = nil
            local categoryDef = nil
            local previousDef = nil -- for tracking def changes to animate things in a slightly more intelligent way
            local previousPage = 1 -- for tracking page changes to animate things in a slightly more intelligent way
            local rowHandle = {} -- for accessing the row frame from other points of reference (within this function) instantly
            -- MultiChoice pagination
            rowHandle.choicePage = 1
            rowHandle.maxChoicePage = 1

            -- convenience to hit the AssociatedOptions optionDef stuff (primarily for speed mods but can be used for whatever)
            -- hyper inefficient function (dont care) (yes i do)
            local function updateAssociatedElements(thisDef)
                if thisDef ~= nil and thisDef.AssociatedOptions ~= nil then
                    -- for each option
                    for _, optionName in ipairs(thisDef.AssociatedOptions) do
                        -- for each possible row to match
                        for rowIndex = 1, optionRowCount do
                            local row = rowHandle:GetParent():GetChild("OptionRow_"..rowIndex)
                            if row ~= nil then
                                if row.defInUse ~= nil and row.defInUse.Name == optionName then
                                    row:playcommand("DrawRow")

                                    -- update cursor sizing and stuff
                                    -- (i know without testing it that this will break if the associated element is a MultiChoice. please dont do that thanks)
                                    local cursorRow = getRowForCursorByCurrentPosition()
                                    if cursorRow ~= nil and cursorRow:GetName() == row:GetName() then
                                        setCursorPositionByCurrentConditions()
                                    end
                                end
                            end
                        end
                    end
                end
            end

            -- convenience
            local function redrawChoiceRelatedElements()
                local rightpair = rowHandle:GetChild("RightTrianglePairFrame")
                local right = rowHandle:GetChild("RightBigTriangleFrame")
                local choices = rowHandle:GetChild("ChoiceFrame")
                if choices ~= nil then
                    choices:finishtweening()
                    choices:diffusealpha(0)
                    -- only animate the redraw for non single choices
                    -- the choice item shouldnt move so this isnt so weird
                    if optionDef ~= nil and optionDef.Type ~= "SingleChoice" and optionDef.Type ~= "SingleChoiceModifier" then
                        choices:smooth(optionRowQuickAnimationSeconds)
                    end
                    choices:diffusealpha(1)
                    choices:playcommand("DrawElement")
                end
                if right ~= nil then
                    right:playcommand("DrawElement")
                end
                if rightpair ~= nil then
                    rightpair:playcommand("DrawElement")
                end
                updateAssociatedElements(optionDef)

                -- if the cursor is on this row, update it because the width may have changed or something
                -- and for a multichoice if the cursor was in some position and we changed page, move it to a sane position
                local cursorRow = getRowForCursorByCurrentPosition()
                if cursorRow == nil then return end
                if cursorRow:GetName() == rowHandle:GetName() then
                    -- at this point we can assume rightPaneCursorPosition is the current cursor position
                    if optionDef.Type == "MultiChoice" then
                        local choicesPerPage = math.min(choiceCount, maxChoicesVisibleMultiChoice)
                        local cursorChoicePos = availableCursorPositions[rightPaneCursorPosition].HighlightedChoice
                        -- only have to take action if there is more than 1 page implied
                        if choicesPerPage < #optionDef.Choices then
                            local validLower = 1 + (rowHandle.choicePage-1) * maxChoicesVisibleMultiChoice
                            local validUpper = rowHandle.choicePage * maxChoicesVisibleMultiChoice
                            if validUpper > #optionDef.Choices then validUpper = #optionDef.Choices end -- if last page missing elements
                            if cursorChoicePos < validLower then
                                -- highlight is too high, move to the last one
                                availableCursorPositions[rightPaneCursorPosition].HighlightedChoice = validLower
                            elseif cursorChoicePos > validUpper then
                                -- highlight is too low, move to first one
                                availableCursorPositions[rightPaneCursorPosition].HighlightedChoice = validUpper
                            else
                                -- probably dont have to do anything? its in valid range...
                            end
                        end
                    end

                    setCursorPositionByCurrentConditions()
                end
            end

            -- index of the choice for this option, if no choices then this is useless
            -- this can also be a table of indices for MultiChoice
            -- this can also just be a random number or text for some certain implementations of optionDefs as long as they conform
            local currentChoiceSelection = 1
            -- move SingleChoice selection index (assuming a list of choices is present -- if not, another methodology is used)
            local function moveChoiceSelection(n)
                if optionDef == nil then return end

                -- make selection loop both directions
                local nn = currentChoiceSelection + n
                if nn <= 0 then
                    nn = n > 0 and 1 or #optionDef.Choices
                elseif nn > #optionDef.Choices then
                    nn = 1
                end
                currentChoiceSelection = nn
                if optionDef.Choices ~= nil and optionDef.Choices[currentChoiceSelection] ~= nil then
                    optionDef.Choices[currentChoiceSelection].ChosenFunction()
                    broadcastOptionUpdate(optionDef, currentChoiceSelection)
                end
                if rowHandle ~= nil then
                    redrawChoiceRelatedElements()
                end
            end

            -- paginate choices according to maxChoicesVisibleMultiChoice
            local function moveChoicePage(n)
                if rowHandle.maxChoicePage <= 1 then
                    return
                end

                -- math to make pages loop both directions
                local nn = (rowHandle.choicePage + n) % (rowHandle.maxChoicePage + 1)
                if nn == 0 then
                    nn = n > 0 and 1 or rowHandle.maxChoicePage
                end
                rowHandle.choicePage = nn
                if rowHandle ~= nil then
                    redrawChoiceRelatedElements()
                end
            end

            -- getter for all relevant children of the row
            -- expects that self is OptionRow_i
            local function getRowElements(self)
                -- directional arrows
                local leftpair = self:GetChild("LeftTrianglePairFrame")
                local left = self:GetChild("LeftBigTriangleFrame")
                local rightpair = self:GetChild("RightTrianglePairFrame")
                local right = self:GetChild("RightBigTriangleFrame")
                -- choices
                local choices = self:GetChild("ChoiceFrame")
                local title = self:GetChild("TitleText")
                local categorytriangle = self:GetChild("CategoryTriangle")
                return leftpair, left, rightpair, right, choices, title, categorytriangle
            end

            local t = Def.ActorFrame {
                Name = "OptionRow_"..i,
                InitCommand = function(self)
                    self:x(actuals.EdgePadding)
                    self:y((actuals.OptionAllottedHeight / #rowChoiceCount) * (i-1) + (actuals.OptionAllottedHeight / #rowChoiceCount / 2))
                    rowHandle = self
                end,
                SetChoicePageCommand = function(self, params)
                    local newpage = clamp(params.page, 1, rowHandle.maxChoicePage)
                    rowHandle.choicePage = newpage
                    redrawChoiceRelatedElements()
                end,
                UpdateRowCommand = function(self)
                    -- update row information, draw (this will reset the state of the row according to "global" conditions)
                    local firstOptionRowIndex = openedCategoryIndex + 1
                    local lastOptionRowIndex = firstOptionRowIndex + #openedCategoryDef - 1

                    -- track previous definition
                    previousDef = nil
                    if optionDef ~= nil then previousDef = optionDef end
                    if categoryDef ~= nil then previousDef = categoryDef end
                    previousPage = rowHandle.choicePage

                    -- reset state
                    optionDef = nil
                    categoryDef = nil
                    self.defInUse = nil
                    rowHandle.choicePage = 1
                    rowHandle.maxChoicePage = 1

                    if i >= firstOptionRowIndex and i <= lastOptionRowIndex then
                        -- this is an option and has an optionDef
                        local optionDefIndex = i - firstOptionRowIndex + 1
                        optionDef = openedCategoryDef[optionDefIndex]
                        if optionDef.Choices ~= nil then
                            rowHandle.maxChoicePage = math.ceil(#optionDef.Choices / maxChoicesVisibleMultiChoice)
                        end
                        self.defInUse = optionDef
                    else
                        -- this is a category or nothing at all
                        -- maybe generate a "categoryDef" which is really just a summary of what to display instead
                        local lastValidPossibleIndex = lastOptionRowIndex + (#selectedPageDef - openedCategoryIndex)
                        if i > lastValidPossibleIndex then
                            -- nothing.
                        else
                            -- this has a categoryDef
                            local adjustedCategoryIndex = i
                            -- subtract the huge list of optionDefs to grab the position of the category in the original list
                            if i > lastOptionRowIndex then
                                adjustedCategoryIndex = (i) - #openedCategoryDef
                            end
                            categoryDef = {
                                Opened = (openedCategoryIndex == i) and true or false,
                                Name = selectedPageDef[adjustedCategoryIndex]
                            }
                            self.defInUse = categoryDef
                        end
                    end

                    self:playcommand("DrawRow")
                end,
                DrawRowCommand = function(self)
                    -- redraw row
                    local leftPairArrows, leftArrow, rightPairArrows, rightArrow, choiceFrame, titleText, categoryTriangle = getRowElements(self)

                    if optionDef ~= nil and optionDef.ChoiceIndexGetter ~= nil then
                        currentChoiceSelection = optionDef.ChoiceIndexGetter()
                    end

                    -- blink the row if it updated
                    self:finishtweening()
                    self:diffusealpha(0)
                    -- if def was just defined, or def just changed, or choice page just changed -- show animation
                    if previousDef == nil or (optionDef ~= nil and optionDef.Name ~= previousDef.Name) or (categoryDef ~= nil and categoryDef.Name ~= previousDef.Name) or previousPage ~= rowHandle.choicePage then
                        self:smooth(optionRowAnimationSeconds)
                    end
                    self:diffusealpha(1)

                    -- this is done so that the redraw can be done in a particular order, left to right
                    -- also, not all of these actors are guaranteed to exist
                    -- and each actor may or may not rely on the previous one to be positioned in order to correctly draw
                    -- the strict ordering is required as a result
                    if categoryTriangle ~= nil then
                        categoryTriangle:playcommand("DrawElement")
                    end

                    if titleText ~= nil then
                        titleText:playcommand("DrawElement")
                    end

                    if leftPairArrows ~= nil then
                        leftPairArrows:playcommand("DrawElement")
                    end

                    if leftArrow ~= nil then
                        leftArrow:playcommand("DrawElement")
                    end

                    if choiceFrame ~= nil then
                        choiceFrame:playcommand("DrawElement")
                    end

                    if rightArrow ~= nil then
                        rightArrow:playcommand("DrawElement")
                    end

                    if rightPairArrows ~= nil then
                        rightPairArrows:playcommand("DrawElement")
                    end
                end,

                -- category title and option name
                UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "TitleText",
                    InitCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        txt:halign(0)
                        txt:zoom(optionTitleTextSize)
                        txt:settext(" ")
                        registerActorToColorConfigElement(txt, "main", "PrimaryText")

                        bg:halign(0)
                        bg:zoomto(0, txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    end,
                    DrawElementCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")

                        if optionDef ~= nil then
                            self:x(0)
                            txt:settext(optionDef.Name)
                            txt:maxwidth(actuals.OptionTextWidth / optionTitleTextSize - textZoomFudge)
                        elseif categoryDef ~= nil then
                            local newx = actuals.OptionBigTriangleWidth + actuals.OptionTextBuffer / 2
                            self:x(newx)
                            txt:settext(categoryDef.Name)
                            txt:maxwidth((actuals.OptionTextWidth - newx) / optionTitleTextSize - textZoomFudge)
                        else
                            txt:settext("")
                        end
                        bg:zoomx(txt:GetZoomedWidth())
                    end,
                    InvokeCommand = function(self)
                        -- behavior for interacting with the Option Row Title Text
                        if categoryDef ~= nil then
                            rowHandle:GetParent():playcommand("OpenCategory", {categoryName = categoryDef.Name})
                        elseif optionDef ~= nil then
                            if optionDef.Type == "Button" then
                                -- button
                                if optionDef.Choices and #optionDef.Choices >= 1 then
                                    optionDef.Choices[1].ChosenFunction()
                                    broadcastOptionUpdate(optionDef, 1)
                                end
                            else
                                -- ?
                            end
                        end
                    end,
                    RolloverUpdateCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "in" then
                            self:diffusealpha(buttonHoverAlpha)
                            updateExplainText(rowHandle)
                            setCursorVerticalHorizontalPos(rowHandle, nil)
                        else
                            self:diffusealpha(1)
                        end
                    end,
                    ClickCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "OnMouseDown" then
                            if optionDef ~= nil or categoryDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end
                    end,
                },
                UIElements.QuadButton(0, 1) .. {
                    Name = "MouseWheelRegion",
                    InitCommand = function(self)
                        self:halign(0)
                        self:diffusealpha(0)
                        self:zoomto(500, actuals.OptionAllottedHeight / optionRowCount)
                    end,
                    MouseScrollMessageCommand = function(self, params)
                        if isOver(self) and focused and (optionDef ~= nil or categoryDef ~= nil) then
                            if optionDef ~= nil then
                                if optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" or optionDef.Type == "MultiChoice" then
                                    if params.direction == "Up" then
                                        rowHandle:GetChild("RightBigTriangleFrame"):playcommand("Invoke")
                                    else
                                        rowHandle:GetChild("LeftBigTriangleFrame"):playcommand("Invoke")
                                    end
                                end
                            end
                        end
                    end,
                    MouseOverCommand = function(self)
                        if not focused or optionDef == nil then return end
                        updateExplainText(rowHandle)
                        -- uncomment to update cursor position when hovering the invisible area
                        -- seems like an annoying and buggy looking behavior
                        -- although it is correct, it is just weird
                        --setCursorVerticalHorizontalPos(rowHandle, nil)
                    end,
                }
            }

            -- category arrow
            if types["Category"] then
                t[#t+1] = UIElements.SpriteButton(1, 1, THEME:GetPathG("", "_triangle")) .. {
                    Name = "CategoryTriangle",
                    InitCommand = function(self)
                        self:x(actuals.OptionBigTriangleWidth/2)
                        self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        registerActorToColorConfigElement(self, "options", "Arrows")
                    end,
                    DrawElementCommand = function(self)
                        if categoryDef ~= nil then
                            if categoryDef.Opened then
                                self:rotationz(180)
                            else
                                self:rotationz(90)
                            end
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,
                    InvokeCommand = function(self)
                        -- behavior for interacting with the Option Row Title Text
                        if categoryDef ~= nil and not categoryDef.Opened then
                            rowHandle:GetParent():playcommand("OpenCategory", {categoryName = categoryDef.Name})
                        end
                    end,
                    MouseOverCommand = onHover,
                    MouseOutCommand = onUnHover,
                    MouseDownCommand = function(self, params)
                        if self:IsInvisible() then return end
                        self:playcommand("Invoke")
                    end,
                }
            end

            -- smaller double arrow, left/right
            if arrowCount == 2 then
                -- copy paste territory
                t[#t+1] = Def.ActorFrame {
                    Name = "LeftTrianglePairFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and optionDef.Type == "SingleChoiceModifier" then
                            -- only visible in this case
                            -- offset by half the triangle size due to center aligning
                            self:x(actuals.OptionTextWidth + actuals.OptionTextBuffer + actuals.OptionSmallTriangleHeight/2)
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "LeftTriangle", -- outermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(-90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            registerActorToColorConfigElement(self, "options", "Arrows")
                        end,
                    },
                    Def.Sprite {
                        Name = "RightTriangle", -- innermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(-90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            -- subtract by 25% triangle height because image is 25% invisible
                            self:x(actuals.OptionSmallTriangleHeight + actuals.OptionSmallTriangleGap - actuals.OptionSmallTriangleHeight/4)
                            registerActorToColorConfigElement(self, "options", "Arrows")
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "LeftTrianglePairButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:x(actuals.OptionSmallTriangleHeight/2)
                            self:zoomto(actuals.OptionSmallTriangleHeight * 2 + actuals.OptionSmallTriangleGap, actuals.OptionBigTriangleWidth)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoiceModifier selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoice (multiplier)
                                        optionDef.Directions.Toggle(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Left ~= nil then
                                        -- Move Left (multiplier)
                                        optionDef.Directions.Left(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(-2)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
                t[#t+1] = Def.ActorFrame {
                    Name = "RightTrianglePairFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and optionDef.Type == "SingleChoiceModifier" then
                            -- only visible in this case
                            local optionRowChoiceFrame = rowHandle:GetChild("ChoiceFrame")
                            if choiceCount < 1 then self:diffusealpha(0):z(-1) return end
                            -- offset by the position of the choice text and the size of the big triangles
                            -- the logic/ordering of the positioning is visible in the math
                            -- choice xpos + width + buffer + big triangle size + buffer
                            -- we pick choice 1 because only SingleChoice is allowed to show these arrows
                            -- offset by half triangle size due to center aligning (edit: nvm?)
                            -- okay actually im gonna be honest I DONT KNOW WHAT IS HAPPENING HERE
                            -- but it completely mirrors the behavior of the other side so it works
                            -- help
                            self:x(optionRowChoiceFrame:GetX() + optionRowChoiceFrame:GetChild("Choice_1"):GetChild("Text"):GetZoomedWidth() + actuals.OptionChoiceDirectionGap + actuals.OptionBigTriangleHeight*0.9 + actuals.OptionChoiceDirectionGap)
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "RightTriangle", -- outermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            -- subtract by 25% triangle height because image is 25% invisible
                            self:x(actuals.OptionSmallTriangleHeight + actuals.OptionSmallTriangleGap - actuals.OptionSmallTriangleHeight/4)
                            registerActorToColorConfigElement(self, "options", "Arrows")
                        end,
                    },
                    Def.Sprite {
                        Name = "LeftTriangle", -- innermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            self:x(0)
                            registerActorToColorConfigElement(self, "options", "Arrows")
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "RightTrianglePairButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:x(actuals.OptionSmallTriangleHeight/2)
                            self:zoomto(actuals.OptionSmallTriangleHeight * 2 + actuals.OptionSmallTriangleGap, actuals.OptionBigTriangleWidth)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoiceModifier selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoice (multiplier)
                                        optionDef.Directions.Toggle(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Right ~= nil then
                                        -- Move Right (multiplier)
                                        optionDef.Directions.Right(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(2)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
            end

            -- single large arrow, left/right
            if arrowCount >= 1 then
                t[#t+1] = Def.ActorFrame {
                    Name = "LeftBigTriangleFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and (optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" or (optionDef.Type == "MultiChoice" and rowHandle.maxChoicePage > 1)) then
                            -- visible for SingleChoice(Modifier) and MultiChoice
                            -- only visible on MultiChoice if we need to paginate the choices
                            -- offset by half height due to center aligning
                            local minXPos = actuals.OptionTextWidth + actuals.OptionTextBuffer + actuals.OptionBigTriangleHeight/2
                            if optionDef.Type == "SingleChoice" or optionDef.Type == "MultiChoice" then
                                -- SingleChoice/MultiChoice is on the very left
                                self:x(minXPos)
                            else
                                -- SingleChoiceModifier is to the right of the LeftTrianglePairFrame
                                -- subtract by 25% triangle height twice because 25% of the image is invisible
                                self:x(minXPos + actuals.OptionSmallTriangleHeight * 2 - actuals.OptionSmallTriangleHeight/2 + actuals.OptionSmallTriangleGap + actuals.OptionChoiceDirectionGap)
                            end
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "Triangle",
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(-90)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                            registerActorToColorConfigElement(self, "options", "Arrows")
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "TriangleButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    -- MultiChoice pagination
                                    moveChoicePage(-1)
                                elseif optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoice selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoices
                                        optionDef.Directions.Toggle()
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Left ~= nil then
                                        -- Move Left (no multiplier)
                                        optionDef.Directions.Left(false)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(-1)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
                t[#t+1] = Def.ActorFrame {
                    Name = "RightBigTriangleFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and (optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" or (optionDef.Type == "MultiChoice" and rowHandle.maxChoicePage > 1)) then
                            -- visible for SingleChoice(Modifier) and MultiChoice
                            local optionRowChoiceFrame = rowHandle:GetChild("ChoiceFrame")
                            if choiceCount < 1 then self:diffusealpha(0):z(-1) return end
                            -- offset by the position of the choice text and appropriate buffer
                            -- the logic/ordering of the positioning is visible in the math
                            -- choice xpos + width + buffer
                            -- we pick choice 1 because only SingleChoice is allowed to show these arrows
                            -- subtract by 25% triangle height because 25% of the image is invisible
                            -- offset by half height due to center aligning
                            if optionDef.Type == "MultiChoice" then
                                -- offset to the right of the last visible choice (up to the 4th one)
                                local lastChoiceIndex = math.min(maxChoicesVisibleMultiChoice, #optionDef.Choices) -- last choice if not on first or last page
                                if rowHandle.choicePage > 1 and rowHandle.choicePage >= rowHandle.maxChoicePage then
                                    -- last if on last (first) page
                                    lastChoiceIndex = #optionDef.Choices % maxChoicesVisibleMultiChoice
                                    if lastChoiceIndex == 0 then lastChoiceIndex = maxChoicesVisibleMultiChoice end
                                end
                                local lastChoice = optionRowChoiceFrame:GetChild("Choice_"..lastChoiceIndex)
                                local finalX = optionRowChoiceFrame:GetX() + lastChoice:GetX() + lastChoice:GetChild("Text"):GetZoomedWidth() + actuals.OptionChoiceDirectionGap + actuals.OptionBigTriangleHeight/4
                                self:x(finalX)
                            else
                                self:x(optionRowChoiceFrame:GetX() + optionRowChoiceFrame:GetChild("Choice_1"):GetChild("Text"):GetZoomedWidth() + actuals.OptionChoiceDirectionGap + actuals.OptionBigTriangleHeight/4)
                            end
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "Triangle",
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(90)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                            registerActorToColorConfigElement(self, "options", "Arrows")
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "TriangleButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    -- MultiChoice pagination
                                    moveChoicePage(1)
                                elseif optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoice selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoices
                                        optionDef.Directions.Toggle()
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Right ~= nil then
                                        -- Move Right (no multiplier)
                                        optionDef.Directions.Right(false)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(1)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
            end

            -- choice text
            local function createOptionRowChoices()
                local t = Def.ActorFrame {
                    Name = "ChoiceFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil then
                            self:diffusealpha(1)

                            local minXPos = actuals.OptionTextWidth + actuals.OptionTextBuffer
                            local finalXPos = minXPos
                            -- triangle width buffer thing .... the distance from minX to ... the choices ... across the one big triangle ...
                            local triangleWidthBufferThing = actuals.OptionBigTriangleHeight + actuals.OptionChoiceDirectionGap - actuals.OptionBigTriangleHeight/4
                            if optionDef.Type == "SingleChoice" or (optionDef.Type == "MultiChoice" and rowHandle.maxChoicePage > 1) then
                                -- leftmost xpos + big triangle + gap
                                -- subtract by 25% of the big triangle size because the image is actually 25% invisible
                                finalXPos = finalXPos + triangleWidthBufferThing
                            elseif optionDef.Type == "SingleChoiceModifier" then
                                -- leftmost xpos + big triangle + gap + 2 small triangles + gap between 2 small triangles + last gap
                                -- subtract by 25% of big triangle and 25% of small triangle twice because the image is 25% invisible
                                finalXPos = finalXPos + triangleWidthBufferThing + actuals.OptionSmallTriangleHeight * 2 - actuals.OptionSmallTriangleHeight/2 + actuals.OptionSmallTriangleGap + actuals.OptionChoiceDirectionGap
                            end
                            self:x(finalXPos)

                            -- to force the choices to update left to right
                            -- update the text of all of them first to see what the width would be
                            local lastFilledChoiceIndex = 1
                            for i = 1, math.min(choiceCount, maxChoicesVisibleMultiChoice) do
                                local child = self:GetChild("Choice_"..i)
                                child:playcommand("SetChoiceText")
                                if #child:GetChild("Text"):GetText() > 0 then
                                    lastFilledChoiceIndex = i
                                end
                            end

                            -- so basically this bad line of math evenly splits the given area including the buffer zones in between
                            -- it also takes into account whether or not we have the triangles on the edges (so if missing, take up more room to equal in width)
                            -- (it doesnt produce a great result and all this garbage is for nothing if you think about it)
                            -- (leaving it here anyways in case this method of setting text and then drawing can be used)
                            local allowedWidth = (actuals.OptionChoiceAllottedWidth - (lastFilledChoiceIndex-1) * actuals.OptionTextBuffer) / lastFilledChoiceIndex + (rowHandle.maxChoicePage <= 1 and triangleWidthBufferThing or 0)
                            for i = 1, math.min(choiceCount, maxChoicesVisibleMultiChoice) do
                                local child = self:GetChild("Choice_"..i)
                                child:GetChild("Text"):maxwidth(allowedWidth / choiceTextSize)
                                child:playcommand("DrawChoice")
                            end


                        else
                            -- missing optionDef means no choices possible
                            self:diffusealpha(0)
                        end
                    end,
                }
                for n = 1, math.min(choiceCount, maxChoicesVisibleMultiChoice) do
                    -- each of these tt's are ActorFrames named Choice_n
                    -- they have 3 children, Text, BG, Underline
                    local tt = UIElements.TextButton(1, 1, "Common Normal") .. {
                        Name = "Choice_"..n,
                        InitCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            txt:halign(0)
                            txt:zoom(optionChoiceTextSize)
                            txt:settext(" ")
                            registerActorToColorConfigElement(txt, "main", "SecondaryText")

                            bg:halign(0)
                            bg:zoomto(0, txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        end,
                        SetChoiceTextCommand = function(self)
                            -- THIS DOES NOT DO BUTTON WORK
                            -- RUN COMMANDS IN THIS ORDER: SetChoiceText -> ??? -> DrawChoice
                            -- That will properly update the text and choices and everything "nicely"
                            local txt = self:GetChild("Text")
                            txt:maxwidth(actuals.OptionChoiceAllottedWidth / choiceTextSize)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    local choiceIndex = n + (rowHandle.choicePage-1) * maxChoicesVisibleMultiChoice
                                    local choice = optionDef.Choices[choiceIndex]
                                    if choice ~= nil then
                                        txt:settext(choice.Name)
                                    else
                                        txt:settext("")
                                    end
                                elseif optionDef.Type == "Button" then
                                    txt:settext("")
                                else
                                    if n == 1 then
                                        -- several cases involving the ChoiceIndexGetter for single choices...
                                        if optionDef.ChoiceIndexGetter ~= nil and optionDef.Choices == nil then
                                            -- getter with no choices means the getter supplies the visible information
                                            txt:settext(currentChoiceSelection)
                                        elseif optionDef.Choices ~= nil then
                                            -- choices present means the getter supplies the choice index that contains the information
                                            txt:settext(optionDef.Choices[currentChoiceSelection].Name)
                                        else
                                            txt:settext("INVALID CONTACT DEVELOPER")
                                        end
                                    else
                                        txt:settext("")
                                    end
                                end
                            else
                                txt:settext("")
                            end
                        end,
                        DrawChoiceCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    -- for Multi choice mode
                                    local choiceIndex = n + (rowHandle.choicePage-1) * maxChoicesVisibleMultiChoice
                                    local choice = optionDef.Choices[choiceIndex]
                                    if choice ~= nil then
                                        local txt = self:GetChild("Text")
                                        local bg = self:GetChild("BG")

                                        -- get the x position of this element using the position of the element to the left
                                        -- this requires all elements be updated in order, left to right
                                        local xPos = 0
                                        if n > 1 then
                                            local choiceJustToTheLeftOfThisOne = self:GetParent():GetChild("Choice_"..(n-1))
                                            xPos = choiceJustToTheLeftOfThisOne:GetX() + choiceJustToTheLeftOfThisOne:GetChild("Text"):GetZoomedWidth() + actuals.OptionTextBuffer
                                        end
                                        self:x(xPos)
                                        bg:zoomx(txt:GetZoomedWidth())
                                        bg:diffusealpha(0.1)

                                        self:diffusealpha(1)
                                        self:z(1)
                                    else
                                        -- choice does not exist for this option but does for another
                                        self:x(0)
                                        self:diffusealpha(0)
                                        self:z(-1)
                                    end
                                elseif optionDef.Type == "Button" then
                                    -- Button is just one choice but lets use the option title as the choice (hide all choices)
                                    self:x(0)
                                    self:diffusealpha(0)
                                    self:z(-1)
                                else
                                    -- for Single choice mode only show first choice
                                    if n == 1 then
                                        local txt = self:GetChild("Text")
                                        local bg = self:GetChild("BG")

                                        bg:zoomx(txt:GetZoomedWidth())
                                        bg:diffusealpha(0)
                                        self:x(0) -- for consistency but makes no difference
                                        self:diffusealpha(1)
                                        self:z(1)
                                    else
                                        self:x(0)
                                        self:diffusealpha(0)
                                        self:z(-1)
                                    end
                                end
                            end
                        end,
                        InvokeCommand = function(self, params)
                            if optionDef ~= nil then
                                if optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoice left clicks will move the option forward
                                    -- SingleChoice right clicks will move the option backward
                                    if params and params.direction then
                                        local fwd = params.direction == "forward"
                                        local bwd = params.direction == "backward"

                                        -- SingleChoice selection mover
                                        if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                            -- Toggle SingleChoices
                                            optionDef.Directions.Toggle()
                                            if optionDef.ChoiceIndexGetter ~= nil then
                                                currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                            end
                                            broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                            redrawChoiceRelatedElements()
                                            return
                                        elseif fwd and optionDef.Directions ~= nil and optionDef.Directions.Right ~= nil then
                                            -- Move Right (no multiplier)
                                            optionDef.Directions.Right(false)
                                            if optionDef.ChoiceIndexGetter ~= nil then
                                                currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                            end
                                            broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                            redrawChoiceRelatedElements()
                                            return
                                        elseif bwd and optionDef.Directions ~= nil and optionDef.Directions.Left ~= nil then
                                            -- Move Left (no multiplier)
                                            optionDef.Directions.Left(false)
                                            if optionDef.ChoiceIndexGetter ~= nil then
                                                currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                            end
                                            broadcastOptionUpdate(optionDef, currentChoiceSelection)
                                            redrawChoiceRelatedElements()
                                            return
                                        end

                                        if optionDef.Choices ~= nil then
                                            moveChoiceSelection(1 * (fwd and 1 or -1))
                                        else
                                            ms.ok("ERROR REPORT TO DEVELOPER")
                                        end
                                    end
                                elseif optionDef.Type == "MultiChoice" then
                                    -- multichoice clicks will toggle the option
                                    local choiceIndex = n + (rowHandle.choicePage-1) * maxChoicesVisibleMultiChoice
                                    local choice = optionDef.Choices[choiceIndex]
                                    if choice ~= nil then
                                        choice.ChosenFunction()
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        broadcastOptionUpdate(optionDef, choice)
                                        self:playcommand("DrawChoice")
                                    end
                                end
                            end
                        end,
                        RolloverUpdateCommand = function(self, params)
                            if self:IsInvisible() then return end
                            if params.update == "in" then
                                self:diffusealpha(buttonHoverAlpha)
                                updateExplainText(rowHandle)
                                if optionDef.Type == "MultiChoice" then
                                    setCursorVerticalHorizontalPos(rowHandle, n + (rowHandle.choicePage-1) * maxChoicesVisibleMultiChoice)
                                else
                                    setCursorVerticalHorizontalPos(rowHandle, 1)
                                end
                            else
                                self:diffusealpha(1)
                            end
                        end,
                        ClickCommand = function(self, params)
                            if self:IsInvisible() then return end
                            if params.update == "OnMouseDown" then
                                if optionDef ~= nil then
                                    local direction = params.event == "DeviceButton_left mouse button" and "forward" or "backward"
                                    self:playcommand("Invoke", {direction = direction})
                                end
                            end
                        end,
                    }
                    tt[#tt+1] = Def.Quad {
                        Name = "Underline",
                        InitCommand = function(self)
                            self:halign(0):valign(0)
                            self:zoomto(0,actuals.OptionChoiceUnderlineThickness)
                            self:diffusealpha(0)
                            registerActorToColorConfigElement(self, "main", "SeparationDivider")
                        end,
                        DrawChoiceCommand = function(self)
                            -- assumption: this Actor is later in the command execution order than the rest of the frame
                            -- that should let it use the attributes after they are set
                            if optionDef == nil or optionDef.Type ~= "MultiChoice" then
                                self:diffusealpha(0)
                            else
                                -- optionDef present and is MultiChoice
                                -- determine if this choice is selected
                                local choiceIndex = n + (rowHandle.choicePage-1) * maxChoicesVisibleMultiChoice
                                local isSelected = currentChoiceSelection[choiceIndex]
                                if isSelected == true then
                                    local bg = self:GetParent():GetChild("BG")
                                    local text = self:GetParent():GetChild("Text")
                                    self:diffusealpha(1)
                                    self:y(bg:GetZoomedHeight()/2 + bg:GetY())
                                    self:zoomx(bg:GetZoomedWidth())
                                else
                                    self:diffusealpha(0)
                                end
                            end
                        end,
                    }
                    t[#t+1] = tt
                end
                return t
            end
            t[#t+1] = createOptionRowChoices()
            return t
        end
        for i = 1, optionRowCount do
            t[#t+1] = createOptionRow(i)
        end
        return t
    end

    local function createOptionPageChoices()
        local selectedIndex = 1

        local function createChoice(i)
            return UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "ButtonTab_"..pageNames[i],
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")

                    -- this position is the center of the text
                    -- divides the space into slots for the choices then places them half way into them
                    -- should work for any count of choices
                    -- and the maxwidth will make sure they stay nonoverlapping
                    self:x((actuals.RightWidth / #pageNames) * (i-1) + (actuals.RightWidth / #pageNames / 2))
                    txt:zoom(choiceTextSize)
                    txt:maxwidth(actuals.RightWidth / #pageNames / choiceTextSize - textZoomFudge)
                    txt:settext(pageNames[i])
                    self:playcommand("ColorConfigUpdated")
                    bg:zoomto(actuals.RightWidth / #pageNames, actuals.TopLipHeight)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    local txt = self:GetChild("Text")
                    txt:diffuse(COLORS:getMainColor("PrimaryText"))
                    txt:diffusealpha(1)
                    self:playcommand("UpdateSelectedIndex")
                end,
                UpdateSelectedIndexCommand = function(self)
                    local txt = self:GetChild("Text")
                    if selectedIndex == i then
                        txt:strokecolor(Brightness(COLORS:getMainColor("PrimaryText"), 0.75))
                    else
                        txt:strokecolor(color("0,0,0,0"))
                    end
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        selectedIndex = i
                        MESSAGEMAN:Broadcast("OptionTabSet", {page = i})
                        self:GetParent():playcommand("UpdateSelectedIndex")
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
                self:y(actuals.TopLipHeight * 1.5)
                self:playcommand("UpdateSelectedIndex")
            end,
            BeginCommand = function(self)
                local snm = SCREENMAN:GetTopScreen():GetName()
                local anm = self:GetName()

                CONTEXTMAN:RegisterToContextSet(snm, "Settings", anm)
                CONTEXTMAN:ToggleContextSet(snm, "Settings", false)

                -- enable the possibility to press the keyboard to switch tabs
                SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                    -- if locked out, dont allow
                    -- pressing a number with ctrl should lead to the general tab stuff
                    -- otherwise, typing numbers will put you into that settings context and reposition the cursor
                    if not CONTEXTMAN:CheckContextSet(snm, "Settings") then return end
                    if event.type == "InputEventType_FirstPress" then
                        local char = inputToCharacter(event)
                        local num = nil

                        -- if ctrl is pressed with a number, let the general tab input handler deal with it
                        if char ~= nil and tonumber(char) and INPUTFILTER:IsControlPressed() then
                            return
                        end

                        if tonumber(char) then
                            num = tonumber(char)
                        end

                        -- cope with number presses to change option pages
                        if num ~= nil then
                            if num == 0 then num = 10 end
                            if num == selectedIndex then return end
                            if num < 1 or num > #pageNames then return end
                            selectedIndex = num
                            MESSAGEMAN:Broadcast("OptionTabSet", {page = num})
                            self:playcommand("UpdateSelectedIndex")
                        end
                    end
                end)
            end
        }
        for i = 1, #pageNames do
            t[#t+1] = createChoice(i)
        end
        return t
    end

    t[#t+1] = createOptionRows()
    t[#t+1] = createOptionPageChoices()

    return t
end

t[#t+1] = leftFrame()
t[#t+1] = rightFrame()

return t
