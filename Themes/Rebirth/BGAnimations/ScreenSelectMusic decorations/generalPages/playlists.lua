local focused = false
local inPlaylistDetails = false
local t = Def.ActorFrame {
    Name = "PlaylistsPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.playliststabindex then
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
                inPlaylistDetails = false
                self:playcommand("UpdatePlaylistsTab")
            else
                self:z(-100)
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
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

    ItemIndexLeftGap = 11 / 1920, -- left edge of frame to left edge of number
    ItemIndexWidth = 38 / 1920, -- left edge of number to uhh nothing
    IconWidth = 18 / 1920, -- for the trash thing
    IconHeight = 21 / 1080,

    -- pertains to the playlist detail pages, not the playlist display
    DetailPageLeftGap = 743 / 1920, -- left edge to left edge of text
    DetailPageUpperGap = 52 / 1080,
    DetailItemAllottedSpace = 440 / 1080,
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
    ItemIndexLeftGap = ratios.ItemIndexLeftGap * SCREEN_WIDTH,
    ItemIndexWidth = ratios.ItemIndexWidth * SCREEN_WIDTH,
    IconWidth = ratios.IconWidth * SCREEN_WIDTH,
    IconHeight = ratios.IconHeight * SCREEN_HEIGHT,
    DetailPageLeftGap = ratios.DetailPageLeftGap * SCREEN_WIDTH,
    DetailPageUpperGap = ratios.DetailPageUpperGap * SCREEN_HEIGHT,
    DetailItemAllottedSpace = ratios.DetailItemAllottedSpace * SCREEN_HEIGHT,
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
    PlayAsCourse = THEME:GetString("ScreenSelectMusic Playlists", "PlayAsCourse"),
    DeletePlaylist = THEME:GetString("ScreenSelectMusic Playlists", "DeletePlaylist"),
    DeleteChart = THEME:GetString("ScreenSelectMusic Playlists", "DeleteChart"),
    NumberOfCharts = THEME:GetString("ScreenSelectMusic Playlists", "NumberOfCharts"),
    AverageMSD = THEME:GetString("ScreenSelectMusic Playlists", "AverageMSD"),
    NewPlaylist = THEME:GetString("ScreenSelectMusic Playlists", "NewPlaylist"),
    AddCurrentChart = THEME:GetString("ScreenSelectMusic Playlists", "AddCurrentChart"),
    Back = THEME:GetString("ScreenSelectMusic Playlists", "Back"),
}

-- playlist list sizing
local itemLine1TextSize = 0.85
local itemLine2TextSize = 0.75
local pageTextSize = 0.7

-- playlist detail sizing
local itemIndexSize = 0.9
local nameTextSize = 0.9
local rateTextSize = 0.9
local msdTextSize = 0.9
local diffTextSize = 0.9
local detailPageTextSize = 0.7

local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local textzoomFudge = 5

local itemListAnimationSeconds = 0.05

-- for accessibility concerns, make buttons a bit bigger than the text they cover
local textButtonHeightFudgeScalarMultiplier = 1.6

-- the entire playlist display ActorFrame
local function playlistList()
    -- modifiable parameters
    local itemCount = 7
    local detailItemCount = 15

    -- internal var storage
    local page = 1
    local maxPage = 1
    local playlistListFrame = nil
    local playlistTable = {}

    local displayListFrame = nil
    local detailPage = 1
    local detailMaxPage = 1

    local function updatePlaylists()
        playlistTable = SONGMAN:GetPlaylists()
        maxPage = math.ceil(#playlistTable / itemCount)

        table.sort(
            playlistTable,
            function(a, b)
                -- this sorts the playlists using the typical alphabetical order we are all familiar with
                local aname = WHEELDATA.makeSortString(a:GetName())
                local bname = WHEELDATA.makeSortString(b:GetName())
                return aname < bname
            end
        )
    end

    local function movePage(n)
        if inPlaylistDetails then
            if detailMaxPage <= 1 then
                return
            end

            -- math to make pages loop both directions
            local nn = (detailPage + n) % (detailMaxPage + 1)
            if nn == 0 then
                nn = n > 0 and 1 or detailMaxPage
            end
            detailPage = nn

            if displayListFrame then
                displayListFrame:playcommand("UpdateItemList")
            end
        else
            if maxPage <= 1 then
                return
            end

            -- math to make pages loop both directions
            local nn = (page + n) % (maxPage + 1)
            if nn == 0 then
                nn = n > 0 and 1 or maxPage
            end
            page = nn

            if playlistListFrame then
                playlistListFrame:playcommand("UpdateItemList")
            end
        end
    end

    local function playlistItem(i)
        local index = i
        local playlist = nil

        -- theres a lot going on here i just wanted to write down vars representing math so its a little clearer for everyone
        -- i should have done this kind of thing in more places but ...
        local itemWidth = actuals.Width * 0.84 -- this 0.84 is balanced with the multiplier for the page number width
        local indX = actuals.ItemIndexLeftGap
        local indW = actuals.ItemIndexWidth
        local remainingWidth = itemWidth - indW - indX
        local nameX = indX + indW -- halign 0
        local deleteX = itemWidth - indX -- halign 1
        local playX = deleteX - actuals.IconWidth - actuals.IconWidth/2 -- halign 1
        local nameW = remainingWidth - (actuals.IconWidth * 2.5) - indX -- area between index and leftmost icon

        return Def.ActorFrame {
            Name = "PlaylistItemFrame_"..i,
            InitCommand = function(self)
                self:y((actuals.ItemAllottedSpace / (itemCount - 1)) * (i-1) + actuals.ItemListUpperGap + actuals.UpperLipHeight)
            end,
            UpdateItemListCommand = function(self)
                index = (page - 1) * itemCount + i
                playlist = playlistTable[index]
                self:finishtweening()
                self:diffusealpha(0)
                if playlist ~= nil then
                    self:playcommand("UpdateText")
                    self:smooth(itemListAnimationSeconds * i)
                    self:diffusealpha(1)
                end
            end,
        
            LoadFont("Common Normal") .. {
                Name = "Index",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(indX)
                    self:zoom(itemLine1TextSize)
                    self:maxwidth(indW / itemLine1TextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settextf("%d.", index)
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(nameX)
                    self:zoom(itemLine1TextSize)
                    self:maxwidth(nameW / itemLine1TextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settext(playlist:GetName())
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if playlist == nil then return end
                    self:diffusealpha(1)
                    SONGMAN:SetActivePlaylist(playlist:GetName())
                    MESSAGEMAN:Broadcast("OpenPlaylistDetails", {playlist = playlist})
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(1)
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showReplay")) .. {
                Name = "PlayCourse",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(playX)
                    self:zoomto(actuals.IconWidth, actuals.IconHeight)
                    registerActorToColorConfigElement(self, "main", "IconColor")
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then
                        self:diffusealpha(0)
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText(translations["PlayAsCourse"])
                            TOOLTIP:Show()
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if playlist == nil then return end
                    SONGMAN:SetActivePlaylist(playlist:GetName())
                    SCREENMAN:GetTopScreen():StartPlaylistAsCourse(playlist:GetName())
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:SetText(translations["PlayAsCourse"])
                    TOOLTIP:Show()
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:Hide()
                    self:diffusealpha(1)
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "deleteGoal")) .. {
                Name = "DeletePlaylist",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(deleteX)
                    self:zoomto(actuals.IconWidth, actuals.IconHeight)
                    registerActorToColorConfigElement(self, "main", "IconColor")
                end,
                UpdateTextCommand = function(self)
                    -- dont allow deleting the Favorites playlist
                    -- this breaks so many things
                    if playlist == nil or playlist:GetName() == "Favorites" then
                        self:diffusealpha(0)
                        if isOver(self) then
                            TOOLTIP:Hide()
                        end
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText(translations["DeletePlaylist"])
                            TOOLTIP:Show()
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if playlist == nil then return end
                    if playlist:GetName() == "Favorites" then
                        -- block the ability to delete the favorites playlist here too
                    else
                        -- this will trigger a save
                        SONGMAN:DeletePlaylist(playlist:GetName())
                        updatePlaylists()
                        -- self - item - itemlist - playlist tab
                        self:GetParent():GetParent():GetParent():playcommand("UpdatePlaylistsTab")
                        TOOLTIP:Hide()
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:SetText(translations["DeletePlaylist"])
                    TOOLTIP:Show()
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    TOOLTIP:Hide()
                    self:diffusealpha(1)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Count",
                InitCommand = function(self)
                    self:valign(0):halign(0)
                    self:x(nameX)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(itemLine2TextSize)
                    self:maxwidth((itemWidth - nameX) / 2 / itemLine2TextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settextf("%s: %d", translations["NumberOfCharts"], playlist:GetNumCharts())
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Average",
                InitCommand = function(self)
                    self:valign(0):halign(1)
                    self:x(itemWidth - indX)
                    self:y(actuals.ItemLowerLineUpperGap)
                    self:zoom(itemLine2TextSize)
                    self:maxwidth((itemWidth - nameX) / 2 / itemLine2TextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                UpdateTextCommand = function(self)
                    if playlist == nil then return end
                    self:settextf("(%s %5.2f)", translations["AverageMSD"], playlist:GetAverageRating())
                end
            }
        }
    end

    -- functionally created frame for only displaying contents of a playlist
    local function detailPageFrame()
        -- playlists keep track of basically just chartkeys and the songs for them might not be loaded
        -- we do our best to care about that ... kind of
        local playlist = nil
        local keylist = {} -- this is a list of keys
        local chartlist = {} -- this is a list of "Chart" which isnt a Steps

        local function detailItem(i)
            local chart = nil
            local chartkey = nil
            local stepsloaded = false
            local index = i

            local itemWidth = actuals.Width
            local indX = actuals.ItemIndexLeftGap
            local indW = actuals.ItemIndexWidth
            local remainingWidth = itemWidth - indW - indX
            local nameX = indX + indW -- halign 0
            local nameW = remainingWidth / 8 * 5
            local rateX = nameX + nameW
            local rateW = remainingWidth / 12 * 1
            local msdX = rateX + rateW
            local msdW = remainingWidth / 8 * 1
            local diffX = msdX + msdW
            local diffW = remainingWidth / 16 * 1
            local deleteX = itemWidth - indX - diffW -- halign 0
    
            return Def.ActorFrame {
                Name = "ChartItem_"..i,
                InitCommand = function(self)
                    self:y((actuals.DetailItemAllottedSpace / (detailItemCount)) * (i-1) + actuals.ItemListUpperGap)
                end,
                SetChartCommand = function(self)
                    index = (detailPage - 1) * detailItemCount + i
                    -- make the assumption that these lists are the same length and if one is nil the other is too
                    chart = chartlist[index]
                    chartkey = keylist[index]
                    self:finishtweening()
                    self:diffusealpha(0)
                    if chart ~= nil then
                        self:smooth(itemListAnimationSeconds * i)
                        self:diffusealpha(1)
                    end
                end,
    
                LoadFont("Common Normal") .. {
                    Name = "Index",
                    InitCommand = function(self)
                        self:valign(0):halign(0)
                        self:x(indX)
                        self:zoom(itemIndexSize)
                        self:maxwidth((indW) / itemIndexSize - textzoomFudge)
                        registerActorToColorConfigElement(self, "main", "PrimaryText")
                    end,
                    SetChartCommand = function(self)
                        if chart ~= nil then
                            self:settextf("%d.", index)
                        end
                    end
                },
                UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "Name",
                    InitCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        self:x(nameX)
                        
                        txt:halign(0):valign(0)
                        bg:halign(0):valign(0)
                        -- this upwards bump fixes font related positioning
                        -- the font has a baseline which pushes it downward by some bit
                        -- this corrects the bg so that the hover is not wrong as a result
                        bg:y(-1)
    
                        txt:zoom(nameTextSize)
                        txt:maxwidth(nameW / nameTextSize - textzoomFudge)
                        registerActorToColorConfigElement(txt, "main", "PrimaryText")
                        bg:zoomy(actuals.ItemAllottedSpace / detailItemCount)
                    end,
                    SetChartCommand = function(self)
                        if chart ~= nil then
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")

                            local name = chart:GetSongTitle()
                            txt:settext(name)
    
                            bg:zoomx(txt:GetZoomedWidth())
    
                            -- if mouse is currently hovering
                            if isOver(bg) and chart:IsLoaded() then
                                self:diffusealpha(buttonHoverAlpha)
                            else
                                self:diffusealpha(1)
                            end
                        end
                    end,
                    ClickCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "OnMouseDown" then
                            -- find song on click (even if filtered)
                            local w = SCREENMAN:GetTopScreen():GetChild("WheelFile")
                            if w ~= nil then
                                if chart:IsLoaded() then
                                    setMusicRate(chart:GetRate())
                                    w:playcommand("FindSong", {chartkey = chartkey})
                                else
                                    -- not loaded - do nothing
                                end
                            end
                        end
                    end,
                    RolloverUpdateCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "in" and chart ~= nil and chart:IsLoaded() then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
                    end
                },
                UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "Rate",
                    InitCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        self:x(msdX - textzoomFudge)

                        txt:halign(1):valign(0)
                        bg:halign(1):valign(0)
                        -- this upwards bump fixes font related positioning
                        -- the font has a baseline which pushes it downward by some bit
                        -- this corrects the bg so that the hover is not wrong as a result
                        bg:y(-1)

                        txt:zoom(rateTextSize)
                        txt:maxwidth(rateW / rateTextSize - textzoomFudge)
                        registerActorToColorConfigElement(txt, "main", "PrimaryText")
                        bg:zoomy(actuals.ItemAllottedSpace / detailItemCount)
                    end,
                    SetChartCommand = function(self)
                        if chart ~= nil then
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")

                            -- cant change rate in the favorites playlist
                            if playlist:GetName() == "Favorites" then
                                self:diffusealpha(0)
                                return
                            else
                                self:diffusealpha(1)
                            end

                            local rate = chart:GetRate()
                            local ratestring = string.format("%.2f", rate):gsub("%.?0+$", "") .. "x"
                            txt:settext(ratestring)
                            bg:zoomx(txt:GetZoomedWidth())

                            -- if mouse is currently hovering
                            if isOver(bg) then
                                self:diffusealpha(buttonHoverAlpha)
                            else
                                self:diffusealpha(1)
                            end
                        end
                    end,
                    ClickCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "OnMouseDown" then
                            if params.event == "DeviceButton_left mouse button" then
                                chart:ChangeRate(0.05)
                            else
                                chart:ChangeRate(-0.05)
                            end
                            self:GetParent():GetParent():playcommand("UpdateDetailDisplay") 
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
                    Name = "MSD",
                    InitCommand = function(self)
                        self:halign(0):valign(0)
                        self:x(msdX)
                        self:zoom(msdTextSize)
                        self:maxwidth(msdW / msdTextSize - textzoomFudge)
                    end,
                    SetChartCommand = function(self)
                        if chart ~= nil then
                            local msd = 0
                            if chart:IsLoaded() then
                                local steps = SONGMAN:GetStepsByChartKey(chartkey)
                                msd = steps:GetMSD(chart:GetRate(), 1)
                            end
                            self:settextf("%05.2f", msd)
                            self:diffuse(colorByMSD(msd))
                        end
                    end
                },
                LoadFont("Common Normal") .. {
                    Name = "Difficulty",
                    InitCommand = function(self)
                        self:halign(0):valign(0)
                        self:x(diffX)
                        self:zoom(diffTextSize)
                        self:maxwidth(diffW / diffTextSize - textzoomFudge)
                    end,
                    ColorConfigUpdatedMessageCommand = function(self)
                        self:playcommand("SetChart")
                    end,
                    SetChartCommand = function(self)
                        if chart ~= nil then
                            local diff = nil
                            if chart:IsLoaded() then
                                local steps = SONGMAN:GetStepsByChartKey(chartkey)
                                diff = steps:GetDifficulty()
                            else
                                diff = chart:GetDifficulty()
                            end
                            self:settext(getShortDifficulty(diff))
                            self:diffuse(colorByDifficulty(diff))
                        end
                    end
                },
                UIElements.SpriteButton(1, 1, THEME:GetPathG("", "deleteGoal")) .. {
                    Name = "DeleteChart",
                    InitCommand = function(self)
                        self:halign(0):valign(0)
                        self:x(deleteX)
                        self:zoomto(actuals.IconWidth, actuals.IconHeight)
                        registerActorToColorConfigElement(self, "main", "IconColor")
                    end,
                    SetChartCommand = function(self)
                        -- dont allow deleting from the Favorites playlist
                        -- this breaks so many things
                        if chart == nil or playlist:GetName() == "Favorites" then
                            self:diffusealpha(0)
                            if isOver(self) then
                                TOOLTIP:Hide()
                            end
                        else
                            if isOver(self) then
                                self:diffusealpha(buttonHoverAlpha)
                                TOOLTIP:SetText(translations["DeleteChart"])
                                TOOLTIP:Show()
                            else
                                self:diffusealpha(1)
                            end
                        end
                    end,
                    MouseDownCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if playlist == nil or playlist:GetName() == "Favorites" then return end
                        -- this will trigger a save
                        playlist:DeleteChart(index)
                        updatePlaylists()
                        self:GetParent():GetParent():playcommand("UpdateDetailDisplay")
                        TOOLTIP:Hide()
                    end,
                    MouseOverCommand = function(self)
                        if self:IsInvisible() then return end
                        TOOLTIP:SetText(translations["DeleteChart"])
                        TOOLTIP:Show()
                        self:diffusealpha(buttonHoverAlpha)
                    end,
                    MouseOutCommand = function(self)
                        if self:IsInvisible() then return end
                        TOOLTIP:Hide()
                        self:diffusealpha(1)
                    end
                }
            }
        end

        local t = Def.ActorFrame {
            Name = "DetailPageFrame",
            InitCommand = function(self)
                self:y(actuals.DetailPageUpperGap)
                self:diffusealpha(0)
            end,
            BeginCommand = function(self)
                displayListFrame = self
            end,
            UpdateDetailDisplayCommand = function(self, params)
                -- not updating page here because we can use this command from any page
                -- but do update max page
                playlist = SONGMAN:GetActivePlaylist()
                if playlist ~= nil then
                    keylist = playlist:GetChartkeys()
                    chartlist = playlist:GetAllSteps()
                    self:GetChild("PageText"):diffusealpha(1)
                else
                    keylist = {}
                    chartlist = {}
                    self:GetChild("PageText"):diffusealpha(0)
                end
                detailMaxPage = math.ceil(#chartlist / detailItemCount)
                -- just in case max page did change ... clamp the page but dont move it otherwise
                detailPage = clamp(detailPage, 1, detailMaxPage)
                self:playcommand("UpdateItemList")
            end,
            UpdateItemListCommand = function(self)
                if inPlaylistDetails then
                    self:diffusealpha(1)
                    self:playcommand("SetChart")
                else
                    self:diffusealpha(0)
                end
            end,
            LoadFont("Common Normal") .. {
                Name = "PageText",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.DetailPageLeftGap)
                    self:zoom(detailPageTextSize)
                    -- oddly precise max width but this should fit with the original size
                    self:maxwidth(actuals.Width / 3 / detailPageTextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                UpdateItemListCommand = function(self)
                    local lb = clamp((detailPage-1) * (detailItemCount) + 1, 0, #chartlist)
                    local ub = clamp(detailPage * detailItemCount, 0, #chartlist)
                    self:settextf("%d-%d/%d", lb, ub, #chartlist)
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "PlaylistName",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0):valign(0)
                    bg:halign(0):valign(0)
                    self:x(actuals.Width - actuals.DetailPageLeftGap)
                    txt:zoom(detailPageTextSize)
                    -- oddly precise max width but this should fit with the original size
                    txt:maxwidth(actuals.Width / 3 * 2 / detailPageTextSize - textzoomFudge)
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                end,
                UpdateItemListCommand = function(self)
                    if playlist ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        txt:settext(playlist:GetName())
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    end
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    -- cant rename the favorites playlist
                    if params.update == "OnMouseDown" and playlist:GetName() ~= "Favorites" then
                        if params.event == "DeviceButton_left mouse button" then
                            renamePlaylistDialogue(playlist:GetName())
                            self:diffusealpha(1)
                        end
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" and playlist:GetName() ~= "Favorites" then
                        self:diffusealpha(buttonHoverAlpha)
                    else
                        self:diffusealpha(1)
                    end
                end,
                PlaylistRenamedMessageCommand = function(self, params)
                    if params and params.success then
                        ms.ok("Successfully renamed playlist")
                        self:playcommand("UpdateItemList")
                    else
                        ms.ok("Failed to rename playlist")
                    end
                end,
            },
        }

        for i = 1, detailItemCount do
            t[#t+1] = detailItem(i)
        end

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
            {   -- Make a new playlist or add the current chart to the opened playlist
                Name = "newentry",
                Type = "Tap",
                Display = {translations["NewPlaylist"], translations["AddCurrentChart"]},
                IndexGetter = function()
                    if inPlaylistDetails then
                        return 2
                    else
                        return 1
                    end
                end,
                Condition = function()
                    if inPlaylistDetails then
                        -- dont allow adding to favorites here because of weird interactions internally
                        if SONGMAN:GetActivePlaylist():GetName() == "Favorites" then
                            return false
                        end
                    end
                    return true
                end,
                TapFunction = function()
                    if inPlaylistDetails then
                        -- adding chart to playlist
                        local pl = SONGMAN:GetActivePlaylist()
                        if pl:GetName() == "Favorites" then
                            -- no adding to favorites this way :)
                        else
                            local steps = GAMESTATE:GetCurrentSteps()
                            if steps ~= nil then
                                -- this triggers a save
                                pl:AddChart(steps:GetChartKey())
                                updatePlaylists()
                                if displayListFrame ~= nil then
                                    displayListFrame:playcommand("UpdateDetailDisplay")
                                end
                            end
                        end
                    else
                        -- adding new playlist
                        newPlaylistDialogue()
                        -- this will trigger the DisplayAll Message after success (also a Profile Save)
                        -- this triggers nothing on failure
                    end
                end,
            },
            {   -- Exit the page that lets you see inside a playlist
                Name = "back",
                Type = "Tap",
                Display = {translations["Back"]},
                IndexGetter = function() return 1 end,
                Condition = function() return inPlaylistDetails end,
                TapFunction = function()
                    MESSAGEMAN:Broadcast("ClosePlaylistDetails")
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
                ColorConfigUpdatedMessageCommand = function(self)
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
                self:y(actuals.UpperLipHeight / 2)
            end,
        }

        for i = 1, #choiceDefinitions do
            t[#t+1] = createChoice(i)
        end

        return t
    end

    local t = Def.ActorFrame {
        Name = "PlaylistListFrame",
        BeginCommand = function(self)
            playlistListFrame = self
            updatePlaylists()
            self:playcommand("UpdateItemList")
            self:playcommand("UpdateText")
        end,
        UpdatePlaylistsTabCommand = function(self)
            page = 1
            inPlaylistDetails = false
            self:playcommand("ClosePlaylistDetails")
            self:playcommand("UpdateItemList")
            self:playcommand("UpdateText")
        end,
        UpdateItemListCommand = function(self)
            -- in case tooltip gets stuck
            TOOLTIP:Hide()
        end,
        OpenPlaylistDetailsMessageCommand = function(self)
            inPlaylistDetails = true
            detailPage = 1
            self:GetChild("Choices"):playcommand("UpdateText")

            if displayListFrame ~= nil then
                displayListFrame:diffusealpha(1)
                displayListFrame:z(10)
                displayListFrame:playcommand("UpdateDetailDisplay")
            end

            local itemframe = self:GetChild("ItemListFrame")
            itemframe:diffusealpha(0)
            self:GetChild("PageText"):diffusealpha(0)
        end,
        ClosePlaylistDetailsMessageCommand = function(self)
            inPlaylistDetails = false
            self:GetChild("Choices"):playcommand("UpdateText")

            if displayListFrame ~= nil then
                displayListFrame:diffusealpha(0)
                displayListFrame:z(-10)
                displayListFrame:playcommand("UpdateDetailDisplay")
            end

            local itemframe = self:GetChild("ItemListFrame")
            itemframe:diffusealpha(1)
            self:GetChild("PageText"):diffusealpha(1)
        end,
        DisplayAllMessageCommand = function(self)
            -- this should only trigger if a new playlist was successfully made
            -- if not ... uhhh..... ???
            updatePlaylists()
            self:playcommand("UpdatePlaylistsTab")
        end,
        DisplaySinglePlaylistMessageCommand = function(self)
            -- really hate this garbage naming convention for the message
            -- this is used for displaying a single playlist in til death but i dont like it
            updatePlaylists()
            self:playcommand("UpdateText")
            self:playcommand("UpdateDetailDisplay")
        end,

        tabChoices(),
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
                    self:GetParent():playcommand("UpdateItemList")
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
            UpdateItemListCommand = function(self)
                local lb = clamp((page-1) * (itemCount) + 1, 0, #playlistTable)
                local ub = clamp(page * itemCount, 0, #playlistTable)
                self:settextf("%d-%d/%d", lb, ub, #playlistTable)
            end
        },
        detailPageFrame()
    }

    -- doing this in a weird way partly out of laziness but also necessity
    -- want to wrap all the playlist displays into a single frame separate from the overall frame
    -- so we can control between showing those and the detail page quickly
    local tt = Def.ActorFrame {Name = "ItemListFrame"}
    for i = 1, itemCount do
        tt[#tt+1] = playlistItem(i)
    end
    t[#t+1] = tt

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

t[#t+1] = playlistList()

return t