local ratios = {
    Width = 782 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything
    PageTextRightGap = 33 / 1920, -- right of frame, right of text
    PageTextUpperGap = 48 / 1080, -- literally a guess
}

local actuals = {
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    PageTextRightGap = ratios.PageTextRightGap * SCREEN_WIDTH,
    PageTextUpperGap = ratios.PageTextUpperGap * SCREEN_HEIGHT,
}

local visibleframeX = SCREEN_WIDTH - actuals.Width
local visibleframeY = SCREEN_HEIGHT - actuals.Height
local animationSeconds = 0.1
local focused = false

local t = Def.ActorFrame {
    Name = "AssetSettingsFile",
    InitCommand = function(self)
        -- lets just say uh ... despite the fact that this file might want to be portable ...
        -- lets ... just .... assume it always goes in the same place ... and the playerInfoFrame is the same size always
        self:x(SCREEN_WIDTH)
        self:y(visibleframeY)
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:x(SCREEN_WIDTH)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "AssetSettings" then
            self:diffusealpha(1)
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
            self:smooth(animationSeconds)
            self:x(visibleframeX)
        else
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(0)
            self:x(SCREEN_WIDTH)
            focused = false
        end
    end,
    FinishFocusingCommand = function(self)
        -- the purpose of this is to delay the act of focusing the screen
        -- in case any input events cause breakage on this screen
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "AssetSettings")
    end
}

local titleTextSize = 0.8

local pageTextSize = 0.5
local textZoomFudge = 5
local pageAnimationSeconds = 0.01
local buttonHoverAlpha = 0.6
local buttonEnabledAlphaMultiplier = 0.8 -- this is multiplied to the current alpha (including the hover alpha) if "clicked"

t[#t+1] = Def.Quad {
    Name = "AssetSettingsBGQuad",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = Def.Quad {
    Name = "AssetSettingsLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.TopLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "AssetSettingsTitle",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
        self:zoom(titleTextSize)
        self:maxwidth(actuals.Width / titleTextSize - textZoomFudge)
        self:settext("Asset Settings")
    end
}

local function toolTipOn(msg)
    TOOLTIP:SetText(msg)
    TOOLTIP:Show()
end

-- produces all the fun stuff in the asset settings
local function assetList()
    local page = 1
    local maxPage = 1
    local settingsframe = nil

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
        if settingsframe ~= nil then
            settingsframe:playcommand("UpdateItemList")
        end
    end

    local t = Def.ActorFrame {
        Name = "AssetSettingsInternalFrame",
        InitCommand = function(self)
            settingsframe = self
        end,
        BeginCommand = function(self)
            self:playcommand("UpdateItemList")

            local snm = SCREENMAN:GetTopScreen():GetName()
            local anm = self:GetName()
            -- init the input context but start it out false
            CONTEXTMAN:RegisterToContextSet(snm, "AssetSettings", anm)
            CONTEXTMAN:ToggleContextSet(snm, "AssetSettings", false)

            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                -- if context is set to AssetSettings, passthrough unless not holding ctrl and a number
                -- pressing a number with ctrl should lead to the general tab
                -- otherwise, typing numbers is allowed
                if CONTEXTMAN:CheckContextSet(snm, "AssetSettings") then
                    if inBundles then return end
                    if event.type ~= "InputEventType_Release" then
                        local btn = event.DeviceInput.button
                        local gbtn = event.button
                        if btn == "DeviceButton_escape" then
                            -- shortcut to exit back to general
                            MESSAGEMAN:Broadcast("GeneralTabSet")
                        else
                            local del = btn == "DeviceButton_delete"
                            local bs = btn == "DeviceButton_backspace"
                            local char = inputToCharacter(event)
                            local up = gbtn == "MenuUp" or gbtn == "Up"
                            local down = gbtn == "MenuDown" or gbtn == "Down"
                            local left = gbtn == "MenuLeft" or gbtn == "Left"
                            local right = gbtn == "MenuRight" or gbtn == "Right"
                            local enter = gbtn == "Start"
                            local pageup = gbtn == "EffectUp"
                            local pagedown = gbtn == "EffectDown"
                            
                            -- if ctrl is pressed with a number, let the general tab input handler deal with this
                            if char ~= nil and tonumber(char) and INPUTFILTER:IsControlPressed() then
                                return
                            end

                            if enter then
                                confirmPick()
                            elseif left then
                                moveCursor(-1, 0)
                            elseif right then
                                moveCursor(1, 0)
                            elseif up then
                                moveCursor(0, -1)
                            elseif down then
                                moveCursor(0, 1)
                            elseif pageup then
                                loadAssetType(curType + 1)
                            elseif pagedown then
                                loadAssetType(curType - 1)
                            end

                            self:playcommand("UpdateItemList")
                        end
                    end
                end
            
            end)
        end,
        UpdateItemListCommand = function(self)
            TOOLTIP:Hide()
            -- maxPage = math.ceil(#packlisting / itemCount)
        end,

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
                self:xy(actuals.Width - actuals.PageTextRightGap, actuals.TopLipHeight + actuals.PageTextUpperGap)
                self:zoom(pageTextSize)
                self:maxwidth((actuals.Width - actuals.PageTextRightGap) / pageTextSize - textZoomFudge)
            end,
            UpdateItemListCommand = function(self)
                -- local lb = clamp((page-1) * (itemCount) + 1, 0, #packlisting)
                -- local ub = clamp(page * itemCount, 0, #packlisting)
                -- self:settextf("%d-%d/%d", lb, ub, #packlisting)
            end
        },
    }


    local function assetBox(i)
        local name = assetTable[i]
        local t = Def.ActorFrame {
            Name = tostring(i),
            InitCommand = function(self)
                self:x((((i-1) % maxColumns)+1)*assetXSpacing)
                self:y(((math.floor((i-1)/maxColumns)+1)*assetYSpacing)-10+50)
                self:diffusealpha(0)
            end,
            PageMovedMessageCommand = function(self)
                self:finishtweening()
                self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                self:diffusealpha(0)
            end,
            UpdateAssetMessageCommand = function(self, params)
                if params.index == i then
                    if i+((curPage-1)*maxColumns*maxRows) > #assetTable then
                        self:finishtweening()
                        self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                        self:diffusealpha(0)
                    else
                        local type = assetTypes[curType]
                        name = assetFolders[type] .. assetTable[i+((curPage-1)*maxColumns*maxRows)]
                        if name == curPath then
                            curIndex = i
                        end
    
                        if curType == 3 then
                            assetWidth = judgmentWidth
                        else
                            assetWidth = squareWidth
                        end
    
                        -- Load the asset image
                        self:GetChild("Image"):playcommand("LoadAsset")
                        self:GetChild("Sound"):playcommand("LoadAsset")
                        self:GetChild("SelectedAssetIndicator"):playcommand("Set")
                        if i == curIndex then
                            self:GetChild("Image"):finishtweening()
                            self:GetChild("Image"):zoomto(assetHeight+8,assetWidth+8)
                            self:GetChild("Border"):zoomto(assetHeight+12,assetWidth+12)
                            self:GetChild("Border"):diffuse(getMainColor("highlight")):diffusealpha(0.8)
                        else
                            self:GetChild("Image"):zoomto(assetHeight,assetWidth)
                            self:GetChild("Border"):zoomto(assetHeight+4,assetWidth+4)
                            self:GetChild("Border"):diffuse(getMainColor("positive")):diffusealpha(0)
                        end
    
                        self:y(((math.floor((i-1)/maxColumns)+1)*assetYSpacing)-10+50)
                        self:finishtweening()
                        self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                        self:diffusealpha(1)
                        self:y((math.floor((i-1)/maxColumns)+1)*assetYSpacing+50)
                                
                    end
                end
            end,
            UpdateFinishedMessageCommand = function(self)
                if assetTable[i+((curPage-1)*maxColumns*maxRows)] == nil then
                    self:finishtweening()
                    self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                    self:diffusealpha(0)
                end
                if curType == 3 then
                    MESSAGEMAN:Broadcast("CursorMoved",{index = findPickedIndexForCurPage()})
                end
            end
        }
        
        t[#t+1] = Def.Quad {
            Name = "SelectedAssetIndicator",
            InitCommand = function(self)
                self:zoomto(assetWidth+14, assetHeight+14)
                self:diffuse(color("#AAAAAA")):diffusealpha(0)
            end,
            SetCommand = function(self)
                self:zoomto(assetWidth+14, assetHeight+14)
                self:finishtweening()
                if selectedPath == name then
                    self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                    self:diffusealpha(0.8)
                else
                    self:smooth(0.2)
                    self:diffusealpha(0)
                end
            end,
            PageMovedMessageCommand = function(self)
                self:queuecommand("Set")
            end,
            PickChangedMessageCommand = function(self)
                self:queuecommand("Set")
            end
        }
    
        t[#t+1] = Def.Quad {
            Name = "Border",
            InitCommand = function(self)
                self:zoomto(assetWidth+4, assetHeight+4)
                self:diffuse(getMainColor("positive")):diffusealpha(0.8)
            end,
            SelectCommand = function(self)
                self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                self:zoomto(assetWidth+12, assetHeight+12)
                self:diffuse(getMainColor("highlight")):diffusealpha(0.8)
            end,
            DeselectCommand = function(self)
                self:smooth(0.2)
                self:zoomto(assetWidth+4, assetHeight+4)
                self:diffuse(getMainColor("positive")):diffusealpha(0)
            end,
            CursorMovedMessageCommand = function(self, params)
                self:finishtweening()
                if params.index == i then
                    self:playcommand("Select")
                else
                    self:playcommand("Deselect")
                end
            end,
            PageMovedMessageCommand = function(self, params)
                self:finishtweening()
                if params.index == i then
                    self:playcommand("Select")
                else
                    self:playcommand("Deselect")
                end
            end,
            MouseLeftClickMessageCommand = function(self)
                if isOver(self) and assetTable[i+((curPage-1)*maxColumns*maxRows)] ~= nil then
                    if lastClickedIndex == i then
                        confirmPick()
                    end
                    local prev = curIndex
                    lastClickedIndex = i
                    curIndex = i
                    MESSAGEMAN:Broadcast("CursorMoved",{index = i, prevIndex = prev})
                end
            end
        }
        
        t[#t+1] = Def.Sprite {
            Name = "Image",
            LoadAssetCommand = function(self)
                local assets = findAssetsForPath(name)
                if #assets > 1 then
                    local image = getImagePath(name, assets)
                    self:LoadBackground(image)
                else
                    self:LoadBackground(name)
                end
            end,
            CursorMovedMessageCommand = function(self, params)
                self:finishtweening()
                if params.index == i then
                    self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                    self:zoomto(assetWidth+8, assetHeight+8)
                else
                    self:smooth(0.2)
                    self:zoomto(assetWidth, assetHeight)
                end
            end,
            PageMovedMessageCommand = function(self, params)
                self:finishtweening()
                if params.index == i then
                    self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                    self:zoomto(assetWidth+8, assetHeight+8)
                else
                    self:smooth(0.2)
                    self:zoomto(assetWidth, assetHeight)
                end
            end
        }
        
        t[#t+1] = Def.Sound {
            Name = "Sound",
            LoadAssetCommand = function(self)
                local assets = findAssetsForPath(name)
                if #assets > 1 then
                    local soundpath = getSoundPath(name, assets)
                    self:load(soundpath)
                else
                    self:load("")
                end
    
            end,
            CursorMovedMessageCommand = function(self, params)
                if params.index == i and curType == 1 and params.prevIndex ~= i then
                    self:play()
                end
            end
        }
        
        return t
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
            {   -- Set to Toasty Select Page
                Name = "toastyselect",
                Type = "Tap",
                Display = {"Toasty"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    groupSet("toasty")
                    page = 1
                end,
            },
            {   -- Set to Avatar Select Page
                Name = "avatarselect",
                Type = "Tap",
                Display = {"Avatar"},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    groupSet("avatar")
                    page = 1
                end,
            },
            {   -- Set to Judgment Select Page
            Name = "judgmentselect",
            Type = "Tap",
            Display = {"Judgment"},
            IndexGetter = function() return 1 end,
            Condition = function() return true end,
            TapFunction = function()
                groupSet("judgment")
                page = 1
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
                    txt:maxwidth(actuals.Width / #choiceDefinitions / choiceTextSize - textZoomFudge)
                    bg:zoomto(actuals.Width / #choiceDefinitions, actuals.TopLipHeight)
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
                self:y(actuals.Height - actuals.TopLipHeight / 2)
            end,
            Def.Quad {
                Name = "BG",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoomto(actuals.Width, actuals.TopLipHeight)
                    self:diffuse(color("#111111"))
                    self:diffusealpha(0.6)
                end
            }
        }
        for i = 1, #choiceDefinitions do
            t[#t+1] = createChoice(i)
        end
        return t
    end
    t[#t+1] = tabChoices()
    return t
end

t[#t+1] = assetList()

return t