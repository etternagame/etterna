-- a large amount of this file is copy pasted and adapted from til death and spawncamping-wallhack
-- things are cleaned up a bit to try to match the format of the rest of this theme
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
local hiddenframeX = SCREEN_WIDTH
local animationSeconds = 0.1
local focused = false
local prevScreen = ""

local translations = {
    Title = THEME:GetString("AssetSettings", "Title"),
    HoveredItem = THEME:GetString("AssetSettings", "HoveredItem"),
    SelectedItem = THEME:GetString("AssetSettings", "SelectedItem"),
    ToastyPageDisplay = THEME:GetString("AssetSettings", "ToastyPageDisplay"),
    AvatarPageDisplay = THEME:GetString("AssetSettings", "AvatarPageDisplay"),
    JudgmentPageDisplay = THEME:GetString("AssetSettings", "JudgmentPageDisplay"),
}

local t = Def.ActorFrame {
    Name = "AssetSettingsFile",
    InitCommand = function(self)
        self:playcommand("SetPosition")
        self:y(visibleframeY)
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:x(hiddenframeX)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "AssetSettings" then
            -- allow exiting out of this screen in a specific ... direction
            prevScreen = params.prevScreen or ""

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
            self:x(hiddenframeX)
            focused = false
        end
    end,
    FinishFocusingCommand = function(self)
        -- the purpose of this is to delay the act of focusing the screen
        -- in case any input events cause breakage on this screen
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "AssetSettings")
    end,
    SetPositionCommand = function(self)
        if getWheelPosition() then
            visibleframeX = SCREEN_WIDTH - actuals.Width
            hiddenframeX = SCREEN_WIDTH
        else
            visibleframeX = 0
            hiddenframeX = -actuals.Width
        end
        if focused then
            self:x(visibleframeX)
        else
            self:x(hiddenframeX)
        end
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetPosition")
    end,
}

local titleTextSize = 0.8
local choiceTextSize = 1
local assetPathTextSize = 0.7

local pageTextSize = 0.7
local textZoomFudge = 5
local buttonHoverAlpha = 0.6

t[#t+1] = Def.Quad {
    Name = "AssetSettingsBGQuad",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "PrimaryBackground")
    end
}

t[#t+1] = Def.Quad {
    Name = "AssetSettingsLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.TopLipHeight)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "SecondaryBackground")
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "AssetSettingsTitle",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
        self:zoom(titleTextSize)
        self:maxwidth(actuals.Width / titleTextSize - textZoomFudge)
        self:settext(translations["Title"])
        registerActorToColorConfigElement(self, "main", "PrimaryText")
    end
}

-- produces all the fun stuff in the asset settings
local function assetList()
    local settingsframe = nil

    local curType = 2 -- start on Avatar page
    local assetTypes = {
        "toasty",
        "avatar",
        "judgment",
    }

    -- state
    local maxPage = 1
    local curPage = 1
    local maxRows = 5
    local maxColumns = 5
    local curIndex = 1
    local selectedIndex = 0
    local profile = PROFILEMAN:GetProfile(PLAYER_1)
    local GUID = profile:GetGUID()
    local curPath = ""
    local lastClickedIndex = 0

    local assetTable = {}

    -- sizing
    local frameWidth = actuals.Width - actuals.EdgePadding*2
    local frameHeight = actuals.Height - actuals.TopLipHeight*2
    local aspectRatioProportion = (16/9) / (SCREEN_WIDTH / SCREEN_HEIGHT) -- this was designed for 16:9 so compensate
    local squareWidth = 50 / aspectRatioProportion -- adjust for aspect ratio
    local judgmentWidth = 75 / aspectRatioProportion -- same
    local assetWidth = squareWidth
    local assetHeight = 50 / aspectRatioProportion -- same
    local assetXSpacing = (frameWidth + assetWidth/2) / (maxColumns + 1)
    local assetYSpacing = (frameHeight - 20 / aspectRatioProportion) / (maxRows + 1) -- same

    local co = nil -- for async loading images

    -------------------
    -- utility functions
    -- self explanatory
    local function findIndexForCurPage()
        local type = assetTypes[curType]
        for i = 1+((curPage-1)*maxColumns*maxRows), 1+((curPage)*maxColumns*maxRows) do
            if assetTable[i] == nil then return nil end
            if assetFolders[type] .. assetTable[i] == curPath then
                return i
            end
        end
    end
    local function findPickedIndexForCurPage()
        local type = assetTypes[curType]
        for i = 1, #assetTable do
            if assetTable[i] == nil then return nil end
            if assetFolders[type] .. assetTable[i] == selectedPath then
                return i
            end
        end
    end
    local function isImage(filename)
        local extensions = {".png", ".jpg", "jpeg"} -- lazy list
        local ext = string.sub(filename, #filename-3)
        for i=1, #extensions do
            if extensions[i] == ext then return true end
        end
        return false
    end
    local function isAudio(filename)
        local extensions = {".wav", ".mp3", ".ogg", ".mp4"} -- lazy to check and put in names
        local ext = string.sub(filename, #filename-3)
        for i=1, #extensions do
            if extensions[i] == ext then return true end
        end
        return false
    end
    local function getImagePath(path, assets) -- expecting a table of asset paths where fallbacks are default
        for i=1, #assets do
            if isImage(assets[i]) then
                return path .. "/" .. assets[i]
            end
        end
        return assetsFolder .. assetFolders[assetTypes[curType]] .. getDefaultAssetByType(assetType[curType]) .. "/default.png"
    end
    local function getSoundPath(path, assets) -- expecting a table of asset paths where fallbacks are default
        for i=1, #assets do
            if isAudio(assets[i]) then
                return path .. "/" .. assets[i]
            end
        end
        return assetsFolder .. assetFolders[assetTypes[curType]] .. getDefaultAssetByType(assetTypes[curType]) .. "/default.ogg"
    end
    local function containsDirsOnly(dirlisting)
        if #dirlisting == 0 then return true end
        for i=1, #dirlisting do
            if isImage(dirlisting[i]) or isAudio(dirlisting[i]) then
                return false
            end
        end
        return true
    end
    --
    --
    -------------------

    -------------------
    -- these are also utility
    -- load asset table for current type
    local function loadAssetTable()
        local type = assetTypes[curType]
        curPath = getAssetByType(type, GUID)
        selectedPath = getAssetByType(type, GUID)
        local dirlisting = FILEMAN:GetDirListing(assetFolders[type])
        if containsDirsOnly(dirlisting) then
            assetTable = dirlisting
        else
            assetTable = filter(isImage, dirlisting)
        end
        maxPage = math.max(1, math.ceil(#assetTable/(maxColumns * maxRows)))
        local ind = findIndexForCurPage()
        local pickind = findPickedIndexForCurPage()
        if pickind ~= nil then selectedIndex = pickind end
        if ind ~= nil then curIndex = ind end
    end

    -- select the asset in the current index for use ingame
    local function confirmPick()
        if curIndex == 0 then return end
        local type = assetTypes[curType]
        local name = assetTable[lastClickedIndex+((curPage-1)*maxColumns*maxRows)]
        if name == nil then return end
        local path = assetFolders[type] .. name
        curPath = path
        selectedPath = path
        selectedIndex = lastClickedIndex+((curPage-1)*maxColumns*maxRows)

        setAssetsByType(type, GUID, path)

        MESSAGEMAN:Broadcast("PickChanged")
        if type == "avatar" then
            MESSAGEMAN:Broadcast("AvatarChanged")
        end
    end

    -- Update all image actors (sprites)
    local function updateImages()
        loadAssetTable()
        MESSAGEMAN:Broadcast("UpdatingAssets", {name = assetTypes[curType]})
        for i=1, math.min(maxRows * maxColumns, #assetTable) do
            MESSAGEMAN:Broadcast("UpdateAsset", {index = i})
            coroutine.yield()
        end
        MESSAGEMAN:Broadcast("UpdateFinished")
    end

    -- move and load asset type forward/backward
    local function loadAssetType(n)
        if n < 1 then n = #assetTypes end
        if n > #assetTypes then n = 1 end
        lastClickedIndex = 0
        curPage = 1
        curType = n
        co = coroutine.create(updateImages)
    end

    -- compatibility function to cope with copy pasta and lack of care
    -- make sure this roughly follows the assetTypes list constructed above
    local function groupSet(groupname)
        local groupnameorder = {
            toasty = 1,
            avatar = 2,
            judgment = 3,
        }
        if groupnameorder[groupname] == nil then return end
        local cur = curType
        local goal = groupnameorder[groupname]
        local movementamount = goal - cur
        loadAssetType(cur + movementamount)
    end

    -- Get cursor index
    local function getIndex()
        local out = ((curPage-1) * maxColumns * maxRows) + curIndex
        return out
    end

    -- Get not the cursor index (really this doesnt work)
    local function getSelectedIndex()
        local out = ((curPage-1) * maxColumns * maxRows) + selectedIndex
        return out
    end

    -- Move n pages forward/backward
    local function movePage(n)
        local nextPage = curPage + n
        if nextPage > maxPage then
            nextPage = maxPage
        elseif nextPage < 1 then
            nextPage = 1
        end

        -- This loads all images again if we actually move to a new page.
        if nextPage ~= curPage then
            curIndex = n < 0 and math.min(#assetTable, maxRows * maxColumns) or 1
            lastClickedIndex = 0
            curPage = nextPage
            MESSAGEMAN:Broadcast("PageMoved",{index = curIndex, page = curPage})
            co = coroutine.create(updateImages)
        end
    end

    -- move the cursor
    local function moveCursor(x, y)
        local move = x + y * maxColumns
        local nextPage = curPage
        local oldIndex = curIndex

        if curPage > 1 and curIndex == 1 and move < 0 then
            curIndex = math.min(#assetTable, maxRows * maxColumns)
            nextPage = curPage - 1
        elseif curPage < maxPage and curIndex == maxRows * maxColumns and move > 0 then
            curIndex = 1
            nextPage = curPage + 1
        else
            curIndex = curIndex + move
            if curIndex < 1 then
                curIndex = 1
            elseif curIndex > math.min(maxRows * maxColumns, #assetTable - (maxRows * maxColumns * (curPage-1))) then
                curIndex = math.min(maxRows * maxColumns, #assetTable - (maxRows * maxColumns * (curPage-1)))
            end
        end
        lastClickedIndex = curIndex
        if curPage == nextPage then
            MESSAGEMAN:Broadcast("CursorMoved",{index = curIndex, prevIndex = oldIndex})
        else
            curPage = nextPage
            MESSAGEMAN:Broadcast("PageMoved",{index = curIndex, page = curPage})
            co = coroutine.create(updateImages)
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

            -- update function for coroutines
            -- this is totally unnecessary but not really but yeah really
            -- the coroutines are responsible for "parallel processing" functions or whatever
            -- but the reality is thats literally a lie because lua is forced to be single threaded here
            co = coroutine.create(updateImages)
            self:SetUpdateFunction(function(self, delta)
                if not focused then return end
                if coroutine.status(co) ~= "dead" then
                    coroutine.resume(co)
                end
            end)

            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                -- if context is set to AssetSettings, passthrough unless not holding ctrl and a number
                -- pressing a number with ctrl should lead to the general tab
                -- otherwise, typing numbers is allowed
                if CONTEXTMAN:CheckContextSet(snm, "AssetSettings") then
                    if event.type ~= "InputEventType_Release" then
                        local btn = event.DeviceInput.button
                        local gbtn = event.button
                        if btn == "DeviceButton_escape" then
                            -- shortcut to exit back to general
                            -- or back to settings screen
                            if prevScreen == "Settings" then
                                MESSAGEMAN:Broadcast("PlayerInfoFrameTabSet", {tab = "Settings"})
                            else
                                MESSAGEMAN:Broadcast("GeneralTabSet")
                            end
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
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdatingAssetsMessageCommand = function(self, params)
                local lb = clamp((curPage-1) * (maxColumns*maxRows) + 1, 0, #assetTable)
                local ub = clamp(curPage * maxColumns * maxRows, 0, #assetTable)
                self:settextf("%d-%d/%d", lb, ub, #assetTable)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "CurrentPath",
            InitCommand = function(self)
                self:zoom(assetPathTextSize)
                self:halign(0)
                self:xy(actuals.EdgePadding, actuals.TopLipHeight + actuals.PageTextUpperGap)
                self:maxwidth((actuals.Width - actuals.EdgePadding) / assetPathTextSize - textZoomFudge)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                local type = assetTable[getIndex()]
                local out = ""
                if type ~= nil then
                    out = type:gsub("^%l", string.upper)
                end
                self:settextf("%s: %s", translations["HoveredItem"], out)
            end,
            CursorMovedMessageCommand = function(self)
                self:queuecommand("Set")
            end,
            UpdateFinishedMessageCommand = function(self)
                self:queuecommand("Set")
            end
        },
        LoadFont("Common Normal") .. {
            Name = "SelectedPath",
            InitCommand = function(self)
                self:zoom(assetPathTextSize)
                self:halign(0)
                -- extremely scuffed y position
                self:xy(actuals.EdgePadding, actuals.TopLipHeight + actuals.PageTextUpperGap + actuals.PageTextUpperGap/2)
                self:maxwidth((actuals.Width - actuals.EdgePadding) / assetPathTextSize - textZoomFudge)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                local type = assetTable[selectedIndex]
                local out = ""
                if type ~= nil then
                    out = type:gsub("^%l", string.upper)
                end
                self:settextf("%s: %s", translations["SelectedItem"], out)
            end,
            PickChangedMessageCommand = function(self)
                self:queuecommand("Set")
            end,
            UpdateFinishedMessageCommand = function(self)
                self:queuecommand("Set")
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
                            self:GetChild("Border"):diffuse(COLORS:getColor("assetSettings", "HoveredItem")):diffusealpha(0.8)
                        else
                            self:GetChild("Image"):zoomto(assetHeight,assetWidth)
                            self:GetChild("Border"):zoomto(assetHeight+4,assetWidth+4)
                            self:GetChild("Border"):diffuse(COLORS:getColor("assetSettings", "HoveredItem")):diffusealpha(0)
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
                self:diffusealpha(0)
                registerActorToColorConfigElement(self, "assetSettings", "SavedItem")
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
                self:diffusealpha(0.8)
                registerActorToColorConfigElement(self, "assetSettings", "HoveredItem")
            end,
            SelectCommand = function(self)
                self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
                self:zoomto(assetWidth+12, assetHeight+12)
                self:diffusealpha(0.8)
            end,
            DeselectCommand = function(self)
                self:smooth(0.2)
                self:zoomto(assetWidth+4, assetHeight+4)
                self:diffusealpha(0)
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
        }
        
        t[#t+1] = UIElements.SpriteButton(1, 1, nil) .. {
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
            end,
            MouseDownCommand = function(self)
                if assetTable[i+((curPage-1)*maxColumns*maxRows)] ~= nil then
                    if lastClickedIndex == i then
                        confirmPick()
                    end
                    local prev = curIndex
                    lastClickedIndex = i
                    curIndex = i
                    MESSAGEMAN:Broadcast("CursorMoved",{index = i, prevIndex = prev})
                end
            end,
            MouseOverCommand = function(self)
                if not self:IsInvisible() then
                    self:diffusealpha(buttonHoverAlpha)
                end
            end,
            MouseOutCommand = function(self)
                if not self:IsInvisible() then
                    self:diffusealpha(1)
                end
            end,
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


    -- this is a copy paste adaptation from the downloads.lua choices
    -- its good and bad for this application for a few reasons, but whatever works is good enough
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
                Name = "toasty",
                Type = "Exclusive",
                Display = {translations["ToastyPageDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    groupSet("toasty")
                    page = 1
                end,
            },
            {   -- Set to Avatar Select Page
                Name = "avatar",
                Type = "Exclusive",
                Display = {translations["AvatarPageDisplay"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    groupSet("avatar")
                    page = 1
                end,
            },
            {   -- Set to Judgment Select Page
                Name = "judgment",
                Type = "Exclusive",
                Display = {translations["JudgmentPageDisplay"]},
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
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                    bg:zoomto(actuals.Width / #choiceDefinitions, actuals.TopLipHeight)
                    self:playcommand("UpdateText")
                end,
                UpdatingAssetsMessageCommand = function(self, params)
                    if params.name == definition.Name then
                        activeChoices[i] = true
                    else
                        activeChoices[i] = false
                    end
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
                        txt:strokecolor(Brightness(COLORS:getMainColor("PrimaryText"), 0.8))
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
                    self:diffusealpha(0.6)
                    registerActorToColorConfigElement(self, "main", "SecondaryBackground")
                end
            }
        }
        for i = 1, #choiceDefinitions do
            t[#t+1] = createChoice(i)
        end
        return t
    end
    for i = 1, maxRows * maxColumns do
        t[#t+1] = assetBox(i)
    end
    t[#t+1] = tabChoices()
    return t
end

t[#t+1] = assetList()

return t