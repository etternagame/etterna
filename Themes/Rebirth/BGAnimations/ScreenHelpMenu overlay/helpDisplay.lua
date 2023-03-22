
local ratios = {
    InfoTopGap = 25 / 1080, -- top edge screen to top edge box
    InfoLeftGap = 60 / 1920, -- left edge screen to left edge box
    InfoWidth = 1799 / 1920, -- small box width
    InfoHeight = 198 / 1080, -- small box height
    InfoHorizontalBuffer = 40 / 1920, -- from side of box to side of text
    InfoVerticalBuffer = 28 / 1080, -- from top/bottom edge of box to top/bottom edge of text

    MainDisplayTopGap = 250 / 1080, -- top edge screen to top edge box
    MainDisplayLeftGap = 60 / 1920, -- left edge screen to left edge box
    MainDisplayWidth = 1799 / 1920, -- big box width
    MainDisplayHeight = 800 / 1080, -- big box height

    ScrollerWidth = 15 / 1920, -- width of the scroll bar and its area (height dependent on items)
    ListWidth = 405 / 1920, -- from right edge of scroll bar to left edge of separation gap
    SeparationGapWidth = 82 / 1920, -- width of the separation area between selection list and the info area

    TopBuffer = 45 / 1080, -- buffer from the top of any section to any item within the section
    TopBuffer2 = 119 / 1080, -- from top edge of section to the subtitle text
    TopBuffer3 = 200 / 1080, -- from top edge of section to the description text
    EdgeBuffer = 25 / 1920, -- buffer from the edge of any section to any item within the section

    IconExitWidth = 47 / 1920,
    IconExitHeight = 36 / 1080,
}

local actuals = {
    InfoTopGap = ratios.InfoTopGap * SCREEN_HEIGHT,
    InfoLeftGap = ratios.InfoLeftGap * SCREEN_WIDTH,
    InfoWidth = ratios.InfoWidth * SCREEN_WIDTH,
    InfoHeight = ratios.InfoHeight * SCREEN_HEIGHT,
    InfoHorizontalBuffer = ratios.InfoHorizontalBuffer * SCREEN_WIDTH,
    InfoVerticalBuffer = ratios.InfoVerticalBuffer * SCREEN_HEIGHT,
    MainDisplayTopGap = ratios.MainDisplayTopGap * SCREEN_HEIGHT,
    MainDisplayLeftGap = ratios.MainDisplayLeftGap * SCREEN_WIDTH,
    MainDisplayWidth = ratios.MainDisplayWidth * SCREEN_WIDTH,
    MainDisplayHeight = ratios.MainDisplayHeight * SCREEN_HEIGHT,
    ScrollerWidth = ratios.ScrollerWidth * SCREEN_WIDTH,
    ListWidth = ratios.ListWidth * SCREEN_WIDTH,
    SeparationGapWidth = ratios.SeparationGapWidth * SCREEN_WIDTH,
    TopBuffer = ratios.TopBuffer * SCREEN_HEIGHT,
    TopBuffer2 = ratios.TopBuffer2 * SCREEN_HEIGHT,
    TopBuffer3 = ratios.TopBuffer3 * SCREEN_HEIGHT,
    EdgeBuffer = ratios.EdgeBuffer * SCREEN_WIDTH,
    IconExitWidth = ratios.IconExitWidth * SCREEN_WIDTH,
    IconExitHeight = ratios.IconExitHeight * SCREEN_HEIGHT,
}

local translations = {
    Title = THEME:GetString("ScreenHelpMenu", "Title"),
    Exit = THEME:GetString("ScreenHelpMenu", "Exit"),
    Instruction = THEME:GetString("ScreenHelpMenu", "Instruction"),
}

local infoTextSize = 0.65
local listTextSize = 0.4
local titleTextSize = 0.95
local subtitleTextSize = 0.55
local descTextSize = 0.4

local textZoomFudge = 5
local buttonHoverAlpha = 0.3
local cursorAlpha = 0.3
local cursorAnimationSeconds = 0.1
local animationSeconds = 0.1

-- special handling to make sure our beautiful icon doesnt get tarnished
local logosourceHeight = 133
local logosourceWidth = 102
local logoratio = math.min(1920 / SCREEN_WIDTH, 1080 / SCREEN_HEIGHT)
local logoH, logoW = getHWKeepAspectRatio(logosourceHeight, logosourceWidth, logosourceWidth / logosourceWidth)

local function helpMenu()

    -- describe each category
    -- this appears in the scroll list and large above the main screen
    -- the point of this table is solely to maintain the order of option categories that show up
    local categoryDefs = {
        "Common Pattern Terminology",
        "Hotkeys",
        "How-To",
        "Information",
        "Troubleshooting",
    }

    -- describe each option in each category
    -- each option within each category shows up in exactly that order. the categories do not use this order (refer to the above table instead)
    -- the definition is:
    --[[
    ["CategoryDef Entry"] = {
        [1] = {
            Name = "OptionName", -- this appears in the scroll list and large in the main area
            ShortDescription = "1 sentence", -- this appears as a subtext to the large text in the main area
            Description = "a paragraph", -- this appears as regular text in the remaining area
            Image = "path to an image", -- OPTIONAL -- if supplied, this takes up the right half of the main area
            -- NOTE:::
            -- Alternative way to construct this data:
            -- missing required fields will be filled in with GetString("ScreenHelpMenu", "item name")
            Name = "optionname",
            ShortDescription = THEME:GetString("ScreenHelpMenu", "optionnameshortdescription")
            Description = THEME:GetString("ScreenHelpMenu", "optionnamedescription")
        },
        [2] = {}, ....
    }
    ]]
    local optionDefs = {
        ["Common Pattern Terminology"] = {
            {
                Name = "RollAscending",
                Image = THEME:GetPathG("", "Patterns/1234 roll"),
            },
            {
                Name = "RollInward",
                Image = THEME:GetPathG("", "Patterns/1243 roll"),
            },
            {
                Name = "RollSplit",
                Image = THEME:GetPathG("", "Patterns/1423 roll"),
            },
            {
                Name = "Gluts",
                Image = THEME:GetPathG("", "Patterns/gluts"),
            },
            {
                Name = "Chordjack",
                Image = THEME:GetPathG("", "Patterns/chordjacks"),
            },
            {
                Name = "DenseChordjack",
                Image = THEME:GetPathG("", "Patterns/dense chordjack"),
            },
            {
                Name = "Stream",
                Image = THEME:GetPathG("", "Patterns/streams"),
            },
            {
                Name = "Jumpstream",
                Image = THEME:GetPathG("", "Patterns/jumpstream"),
            },
            {
                Name = "Handstream",
                Image = THEME:GetPathG("", "Patterns/handstream"),
            },
            {
                Name = "Quadstream",
                Image = THEME:GetPathG("", "Patterns/quadstream"),
            },
            {
                Name = "Trill",
                Image = THEME:GetPathG("", "Patterns/trill"),
            },
            {
                Name = "Jumptrill",
                Image = THEME:GetPathG("", "Patterns/jumptrill"),
            },
            {
                Name = "SplitJumptrill",
                Image = THEME:GetPathG("", "Patterns/13 split jt"),
            },
            {
                Name = "SplitJumptrillTrainTrack",
                Image = THEME:GetPathG("", "Patterns/14 split jt"),
            },
            {
                Name = "Minijacks",
                Image = THEME:GetPathG("", "Patterns/minijacks"),
            },
            {
                Name = "Longjack",
                Image = THEME:GetPathG("", "Patterns/longjack"),
            },
            {
                Name = "Anchor",
                Image = THEME:GetPathG("", "Patterns/anchor"),
            },
            {
                Name = "Minedodge",
                Image = THEME:GetPathG("", "Patterns/minedodge"),
            },
            {
                Name = "Hold",
                Image = THEME:GetPathG("", "Patterns/hold"),
            },
            {
                Name = "Rolld",
                Image = THEME:GetPathG("", "Patterns/rolld"),
            },
            {
                Name = "Burst",
                Image = THEME:GetPathG("", "Patterns/burst"),
            },
            {
                Name = "Polyrhythm",
                Image = THEME:GetPathG("", "Patterns/polyrhythms"),
            },
            {
                Name = "Graces",
                Image = THEME:GetPathG("", "Patterns/graces"),
            },
            {
                Name = "Runningman",
                Image = THEME:GetPathG("", "Patterns/runningman"),
            },
        },
        ["Hotkeys"] = {
            {
                Name = "GlobalHotkeys",
            },
            {
                Name = "SelectMusicHotkeys",
            },
            {
                Name = "GameplayHotkeys",
            },
            {
                Name = "CustomizeGameplayHotkeys",
            },
            {
                Name = "PracticeHotkeys",
            },
            {
                Name = "ReplayHotkeys",
            },
            {
                Name = "EvaluationHotkeys",
            },
            {
                Name = "MultiplayerHotkeys",
            }
        },
        ["How-To"] = {
            {
                Name = "HowToCreateCharts",
            },
            {
                Name = "HowToManualSongInstall",
            },
            {
                Name = "HowToDownloadPacks",
            },
            {
                Name = "HowToSongSearch",
            },
            {
                Name = "HowToSongFilter",
            },
            {
                Name = "HowToSortmode",
            },
            {
                Name = "HowToCustomizeGameplay",
            },
            {
                Name = "HowToUpdate",
            },
            {
                Name = "HowToSetSync",
            },
            {
                Name = "HowToChangeTheme",
            },
            {
                Name = "HowToChangeLanguage",
            },
            {
                Name = "HowToToggleMenuSound",
            },
            {
                Name = "HowToTogglePitchRates",
            },
            {
                Name = "HowToSwapWheelSide",
            },
            {
                Name = "HowToLogin",
            },
            {
                Name = "HowToUpload",
            },
            {
                Name = "HowToFavoriteSongs",
            },
            {
                Name = "HowToPermamirror",
            },
        },
        ["Information"] = {
            {
                Name = "AboutKeymodes",
            },
            {
                Name = "AboutCleartypes",
            },
            {
                Name = "LegacyKeyConfig",
            },
            {
                Name = "ReplayInfo",
            },
            {
                Name = "SelectMusicTips",
            },
            {
                Name = "ProfileTab",
            },
            {
                Name = "GoalsTab",
            },
            {
                Name = "PlaylistsTab",
            },
            {
                Name = "TagsTab",
            },
            {
                Name = "GettingSupport",
            },
        },
        ["Troubleshooting"] = {
            {
                Name = "Softlocked",
            },
            {
                Name = "InputBroke",
            },
            {
                Name = "GeneralTips",
            },
            {
                Name = "SongDoesntLoad",
            },
            {
                Name = "SongDoesntUpdate",
            },
            {
                Name = "ScoresDontSave",
            },
            {
                Name = "ScoresWorth0",
            },
            {
                Name = "RandomCrashes",
            },
        },
    }

    -- generated table
    -- basically the data representation of the scroller thing
    -- categories are top level items
    -- options are slightly shifted over
    -- pagination and indexing is based on this table
    local items = {}
    for _, cat in ipairs(categoryDefs) do
        items[#items+1] = {
            isCategory = true,
            Name = THEME:HasString("ScreenHelpMenu", "Category"..cat) and THEME:GetString("ScreenHelpMenu", "Category"..cat) or cat,
        }
        for __, optionDef in ipairs(optionDefs[cat]) do
            local defname = optionDef.Name
            items[#items+1] = {
                isCategory = false,
                Parent = cat,
                Name = THEME:HasString("ScreenHelpMenu", defname) and THEME:GetString("ScreenHelpMenu", defname) or defname,
                Def = {
                    Name = THEME:HasString("ScreenHelpMenu", defname) and THEME:GetString("ScreenHelpMenu", defname) or defname,
                    ShortDescription = THEME:HasString("ScreenHelpMenu", defname.."ShortDescription") and THEME:GetString("ScreenHelpMenu", defname.."ShortDescription") or optionDef.ShortDescription,
                    Description = THEME:HasString("ScreenHelpMenu", defname.."Description") and THEME:GetString("ScreenHelpMenu", defname.."Description") or defname.Description,
                    Image = optionDef.Image,
                },
            }
        end
    end

    local itemsVisible = 20
    local cursorIndex = 1
    local page = 1
    local maxPage = math.ceil(#items / itemsVisible)
    local function getPageFromIndex(i)
        return math.ceil((i) / itemsVisible)
    end
    local function cursorHoversItem(i)
        if getPageFromIndex(cursorIndex) ~= page then return false end
        return ((cursorIndex-1) % itemsVisible == (i-1))
    end
    local function movePage(n)
        local newpage = page + n
        if newpage > maxPage then newpage = 1 end
        if newpage < 1 then newpage = maxPage end
        page = newpage
        MESSAGEMAN:Broadcast("UpdatePage")
    end
    local function moveCursor(n)
        local newpos = cursorIndex + n
        if newpos > #items then newpos = 1 end
        if newpos < 1 then newpos = #items end
        cursorIndex = newpos
        local newpage = getPageFromIndex(cursorIndex)
        if newpage ~= page then
            page = newpage
            MESSAGEMAN:Broadcast("UpdatePage")
        end
        MESSAGEMAN:Broadcast("UpdateCursor")
    end

    local function menuItem(i)
        local yIncrement = (actuals.MainDisplayHeight) / itemsVisible
        local index = i
        local item = items[index]
        return Def.ActorFrame {
            Name = "MenuItem_"..i,
            InitCommand = function(self)
                self:x(actuals.ScrollerWidth + actuals.EdgeBuffer/2)
                -- center y
                self:y(yIncrement * (i-1) + yIncrement / 2)
                self:playcommand("UpdateItem")
            end,
            SelectCurrentCommand = function(self)
                if cursorHoversItem(i) then
                    -- do something
                    MESSAGEMAN:Broadcast("SelectedItem", {def = item.Def, category = item.Parent})
                end
            end,
            UpdateItemCommand = function(self)
                index = (page-1) * itemsVisible + i
                item = items[index]
                if item ~= nil then
                    self:finishtweening()
                    self:diffusealpha(0)
                    self:smooth(animationSeconds)
                    self:diffusealpha(1)
                else
                    self:finishtweening()
                    self:smooth(animationSeconds)
                    self:diffusealpha(0)
                end
            end,
            UpdatePageMessageCommand = function(self)
                self:playcommand("UpdateItem")
            end,

            UIElements.QuadButton(1, 1) .. {
                Name = "ItemBG", -- also the "cursor" position
                InitCommand = function(self)
                    self:halign(0)
                    -- 97% full size to allow a gap for mouse hover logic reasons
                    self:zoomto(actuals.ListWidth - actuals.EdgeBuffer, yIncrement * 0.97)
                    self:diffusealpha(0)
                    self.alphaDeterminingFunction = function(self)
                        local alpha = 1
                        if isOver(self) then
                            alpha = buttonHoverAlpha
                            if cursorHoversItem(i) then
                                alpha = (buttonHoverAlpha + 1) / 2
                            end
                        else
                            alpha = 0
                            if cursorHoversItem(i) then
                                alpha = cursorAlpha
                            end
                        end

                        self:diffusealpha(alpha)
                    end
                end,
                CursorShowCommand = function(self)
                    self:smooth(cursorAnimationSeconds)
                    self:alphaDeterminingFunction()
                end,
                CursorHideCommand = function(self)
                    self:smooth(cursorAnimationSeconds)
                    self:alphaDeterminingFunction()
                end,
                MouseOverCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                MouseOutCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                UpdateCursorMessageCommand = function(self)
                    self:alphaDeterminingFunction()
                end,
                UpdateItemCommand = function(self)
                    self:alphaDeterminingFunction()
                end,
                MouseDownCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    cursorIndex = index
                    self:GetParent():playcommand("SelectCurrent")
                end,
                SelectedItemMessageCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "Text",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoom(listTextSize)
                    self:maxwidth((actuals.ListWidth - actuals.EdgeBuffer * 2) / listTextSize)
                end,
                UpdateItemCommand = function(self)
                    if item ~= nil then
                        if not item.isCategory then
                            self:x(actuals.EdgeBuffer)
                        else
                            self:x(0)
                        end
                        self:settext(item.Name)
                    end
                end,
            }
        }
    end

    local rightAreaWidth = actuals.MainDisplayWidth - (actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
    local t = Def.ActorFrame {
        Name = "MenuContainer",
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type == "InputEventType_Release" then return end

                local gameButton = event.button
                local key = event.DeviceInput.button
                local up = gameButton == "Up" or gameButton == "MenuUp"
                local down = gameButton == "Down" or gameButton == "MenuDown"
                local right = gameButton == "MenuRight" or gameButton == "Right"
                local left = gameButton == "MenuLeft" or gameButton == "Left"
                local enter = gameButton == "Start"
                local back = key == "DeviceButton_escape"

                if up or left then
                    moveCursor(-1)
                    self:playcommand("SelectCurrent")
                elseif down or right then
                    moveCursor(1)
                    self:playcommand("SelectCurrent")
                elseif enter then
                    self:playcommand("SelectCurrent")
                elseif back then
                    SCREENMAN:GetTopScreen():Cancel()
                end
            end)
            self:playcommand("UpdateCursor")
        end,
        Def.Quad {
            Name = "ScrollBar",
            InitCommand = function(self)
                self:zoomto(actuals.ScrollerWidth, actuals.MainDisplayHeight / maxPage)
                self:halign(0):valign(0)
                self:diffusealpha(0.6)
            end,
            UpdatePageMessageCommand = function(self)
                self:finishtweening()
                self:smooth(animationSeconds)
                self:y(actuals.MainDisplayHeight / maxPage * (page-1))
            end,
        },
        Def.Quad {
            Name = "MouseScrollArea",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:diffusealpha(0)
                self:zoomto(actuals.ScrollerWidth + actuals.ListWidth, actuals.MainDisplayHeight)
            end,
            MouseScrollMessageCommand = function(self, params)
                if isOver(self) then
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                end
            end,
        },
        Def.ActorFrame {
            Name = "SelectedItemContainer",
            InitCommand = function(self)
                self:x(actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
                -- make empty defaults load
                self:playcommand("UpdateSelectedItem")
            end,
            SelectedItemMessageCommand = function(self, params)
                self:playcommand("UpdateSelectedItem", params)
            end,

            LoadFont("Menu Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer)
                    self:zoom(titleTextSize)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.Name)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:maxwidth(((rightAreaWidth / 2) - actuals.EdgeBuffer) / titleTextSize)
                        else
                            self:maxwidth((rightAreaWidth - actuals.EdgeBuffer) / titleTextSize)
                        end
                    else
                        self:settext("")
                    end
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "ShortDescription",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer2)
                    self:skewx(-0.15)
                    self:zoom(subtitleTextSize)
                    self:maxheight((actuals.TopBuffer3 - actuals.TopBuffer2) / subtitleTextSize - textZoomFudge * 5)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.ShortDescription)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:wrapwidthpixels(((rightAreaWidth / 2) - actuals.EdgeBuffer) / subtitleTextSize)
                        else
                            self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer * 2) / subtitleTextSize)
                        end
                    else
                        self:settext("")
                    end
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "Paragraph",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer3)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.Description)
                        self:zoom(descTextSize)
                        self:maxheight((actuals.MainDisplayHeight - actuals.TopBuffer3 - actuals.EdgeBuffer) / descTextSize)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:wrapwidthpixels(((rightAreaWidth / 2) - actuals.EdgeBuffer) / descTextSize)
                        else
                            self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer * 2) / descTextSize)
                        end
                    else
                        self:zoom(subtitleTextSize)
                        self:settext(translations["Instruction"])
                        self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer) / subtitleTextSize)
                    end
                end,
            },
            UIElements.SpriteButton(1, 1, nil) .. {
                Name = "Image",
                InitCommand = function(self)
                    self:valign(0)
                    self:xy(rightAreaWidth / 4 * 3, actuals.TopBuffer)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        if def.Image ~= nil and def.Image ~= "" then
                            self:diffusealpha(1)
                            self:Load(def.Image)
                            local h = self:GetHeight()
                            local w = self:GetWidth()
                            local allowedHeight = actuals.MainDisplayHeight - (actuals.TopBuffer * 2)
                            local allowedWidth = rightAreaWidth - (actuals.EdgeBuffer + actuals.IconExitWidth)
                            if h >= allowedHeight and w >= allowedWidth then
                                if h * (allowedWidth / allowedHeight) >= w then
                                    self:zoom(allowedHeight / h)
                                else
                                    self:zoom(allowedWidth / w)
                                end
                            elseif h >= allowedHeight then
                                self:zoom(allowedHeight / h)
                            elseif w >= allowedWidth then
                                self:zoom(allowedWidth / w)
                            else
                                self:zoom(1)
                            end
                        else
                            self:diffusealpha(0)
                        end
                    else
                        self:diffusealpha(0)
                    end
                end,
            },
        }
    }

    for i = 1, itemsVisible do
        t[#t+1] = menuItem(i)
    end
    return t
end


local t = Def.ActorFrame {
    Name = "HelpDisplayFile",

    Def.ActorFrame {
        Name = "InfoBoxFrame",
        InitCommand = function(self)
            self:xy(actuals.InfoLeftGap, actuals.InfoTopGap)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.InfoWidth, actuals.InfoHeight)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Sprite {
            Name = "Logo",
            Texture = THEME:GetPathG("", "Logo-E"),
            InitCommand = function(self)
                self:xy(actuals.ScrollerWidth + (actuals.ListWidth / 2), actuals.InfoHeight / 2)
                self:zoomto(logoW, logoH)
                registerActorToColorConfigElement(self, "title", "LogoE")
            end
        },
        Def.Sprite {
            Name = "LogoTriangle",
            Texture = THEME:GetPathG("", "Logo-Triangle"),
            InitCommand = function(self)
                self:xy(actuals.ScrollerWidth + (actuals.ListWidth / 2), actuals.InfoHeight / 2)
                self:zoomto(logoW, logoH)
                registerActorToColorConfigElement(self, "title", "LogoTriangle")
            end
        },
        LoadColorFont("Menu Bold") .. {
            Name = "Text",
            InitCommand = function(self)
                local textw = actuals.InfoWidth - (actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
                local textx = actuals.InfoWidth - textw / 2
                self:xy(textx, actuals.InfoHeight/2)
                self:zoom(infoTextSize)
                self:maxheight((actuals.InfoHeight - (actuals.InfoVerticalBuffer*2)) / infoTextSize)
                self:wrapwidthpixels(textw / infoTextSize)
                self:settext(translations["Title"])
            end,
            SelectedItemMessageCommand = function(self, params)
                if params and params.category ~= nil then
                    self:settext(params.category)
                else
                    self:settext(translations["Title"])
                end
            end
        },
    },
    Def.ActorFrame {
        Name = "MainDisplayFrame",
        InitCommand = function(self)
            self:xy(actuals.MainDisplayLeftGap, actuals.MainDisplayTopGap)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.MainDisplayWidth, actuals.MainDisplayHeight)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Quad {
            Name = "Separator",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:x(actuals.ScrollerWidth + actuals.ListWidth)
                self:zoomto(actuals.SeparationGapWidth, actuals.MainDisplayHeight)
                self:diffuse(color("1,1,1"))
                self:diffusealpha(0.2)
            end,
        },
        helpMenu() .. {
            InitCommand = function(self)
                self:xy(0, 0)
            end,
        },
        UIElements.SpriteButton(1, 1, THEME:GetPathG("", "exit")) .. {
            Name = "Exit",
            InitCommand = function(self)
                self:valign(0):halign(1)
                self:xy(actuals.MainDisplayWidth - actuals.InfoVerticalBuffer/4, actuals.InfoVerticalBuffer/4)
                self:zoomto(actuals.IconExitWidth, actuals.IconExitHeight)
            end,
            MouseDownCommand = function(self, params)
                SCREENMAN:GetTopScreen():Cancel()
                TOOLTIP:Hide()
            end,
            MouseOverCommand = function(self, params)
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText(translations["Exit"])
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self, params)
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
        },
    },
    
}

return t
