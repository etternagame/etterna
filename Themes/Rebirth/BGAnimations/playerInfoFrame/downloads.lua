local ratios = {
    Width = 782 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything

    IndexColumnLeftGap = 38 /1920, -- left edge to right edge (right align)
    NameColumnLeftGap = 45 / 1920, -- left edge to left edge (left align)
    MSDColumnLeftGap = 495 / 1920, -- left edge to center of text (center align)
    SizeHeaderLeftGap = 592 / 1920, -- left edge to left edge (left align)
    SizeColumnLeftGap = 662 / 1920, -- left edge to right edge (right align)
    MainDLLeftGap = 692 / 1920, -- left edge to left edge
    MirrorDLLeftGap = 734 / 1920, -- left edge to left edge
    DLIconSize = 20 / 1080, -- it is a square
    -- placement of the DL Text is left align on MainDLLeftGap with width MirrorDLLeftGap - MainDLLeftGap + DLIconSize
    HeaderLineUpperGap = 13 / 1080, -- from bottom of top lip to top of header text
    ItemListUpperGap = 55 / 1080, -- from bottom of top lip to top of topmost item in the list
    -- ItemListAllottedSpace is instead just some fraction of the Height related to the height of the bundle area
    MSDWidth = 80 / 1920, -- approximated max normal width of the MSD since it is center aligned and boundaries need to be made

    SearchBGLeftGap = 292 / 1920, -- left edge to left edge
    SearchBGWidth = 456 / 1920,
    SearchBGHeight = 28 / 1080,
    SearchIconLeftGap = 6 / 1920, -- from left edge of search bg to left edge of icon
    SearchIconSize = 23 / 1080, -- it is a square
    SearchTextLeftGap = 40 / 1920, -- left edge of bg to left edge of text
    PageTextRightGap = 33 / 1920, -- right of frame, right of text
}

local actuals = {
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    IndexColumnLeftGap = ratios.IndexColumnLeftGap * SCREEN_WIDTH,
    NameColumnLeftGap = ratios.NameColumnLeftGap * SCREEN_WIDTH,
    MSDColumnLeftGap = ratios.MSDColumnLeftGap * SCREEN_WIDTH,
    SizeHeaderLeftGap = ratios.SizeHeaderLeftGap * SCREEN_WIDTH,
    SizeColumnLeftGap = ratios.SizeColumnLeftGap * SCREEN_WIDTH,
    MainDLLeftGap = ratios.MainDLLeftGap * SCREEN_WIDTH,
    MirrorDLLeftGap = ratios.MirrorDLLeftGap * SCREEN_WIDTH,
    DLIconSize = ratios.DLIconSize * SCREEN_HEIGHT,
    HeaderLineUpperGap = ratios.HeaderLineUpperGap * SCREEN_HEIGHT,
    ItemListUpperGap = ratios.ItemListUpperGap * SCREEN_HEIGHT,
    MSDWidth = ratios.MSDWidth * SCREEN_WIDTH,
    SearchBGLeftGap = ratios.SearchBGLeftGap * SCREEN_WIDTH,
    SearchBGWidth = ratios.SearchBGWidth * SCREEN_WIDTH,
    SearchBGHeight = ratios.SearchBGHeight * SCREEN_HEIGHT,
    SearchIconLeftGap = ratios.SearchIconLeftGap * SCREEN_WIDTH,
    SearchIconSize = ratios.SearchIconSize * SCREEN_HEIGHT,
    SearchTextLeftGap = ratios.SearchTextLeftGap * SCREEN_WIDTH,
    PageTextRightGap = ratios.PageTextRightGap * SCREEN_WIDTH,
}

local visibleframeX = SCREEN_WIDTH - actuals.Width
local visibleframeY = SCREEN_HEIGHT - actuals.Height
local hiddenframeX = SCREEN_WIDTH
local animationSeconds = 0.1
local focused = false

local translations = {
    Title = THEME:GetString("PackDownloader", "Title"),
    Back = THEME:GetString("PackDownloader", "Back"),
    BundleSelect = THEME:GetString("PackDownloader", "BundleSelect"),
    CancelAll = THEME:GetString("PackDownloader", "CancelAll"),
    Expanded = THEME:GetString("PackDownloader", "Expanded"),
    Novice = THEME:GetString("PackDownloader", "Novice"),
    Beginner = THEME:GetString("PackDownloader", "Beginner"),
    Intermediate = THEME:GetString("PackDownloader", "Intermediate"),
    Advanced = THEME:GetString("PackDownloader", "Advanced"),
    Expert = THEME:GetString("PackDownloader", "Expert"),
    Megabytes = THEME:GetString("PackDownloader", "Megabytes"),
    Cancel = THEME:GetString("PackDownloader", "Cancel"),
    Queued = THEME:GetString("PackDownloader", "Queued"),
    HeaderName = THEME:GetString("PackDownloader", "HeaderName"),
    HeaderAverage = THEME:GetString("PackDownloader", "HeaderAverage"),
    HeaderSize = THEME:GetString("PackDownloader", "HeaderSize"),
    DownloadBundle = THEME:GetString("PackDownloader", "DownloadBundle"),
    DownloadBundleMirrored = THEME:GetString("PackDownloader", "DownloadBundleMirrored"),
    DownloadPack = THEME:GetString("PackDownloader", "DownloadPack"),
    DownloadPackMirrored = THEME:GetString("PackDownloader", "DownloadPackMirrored"),
    AlreadyInstalled = THEME:GetString("PackDownloader", "AlreadyInstalled"),
    CurrentlyDownloading = THEME:GetString("PackDownloader", "CurrentlyDownloading"),
}

local t = Def.ActorFrame {
    Name = "DownloadsFile",
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
        if params.tab and params.tab == "Downloads" then
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
        -- the reason is that we dont want to trigger Ctrl+3 inputting a 3 on the search field immediately
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Downloads")
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
local indexTextSize = 0.8
local nameTextSize = 0.8
local msdTextSize = 0.8
local sizeTextSize = 0.8
local cancelTextSize = 0.8

local indexHeaderSize = 1
local nameHeaderSize = 1
local msdHeaderSize = 1
local sizeHeaderSize = 1
local searchTextSize = 1
local choiceTextSize = 1

local pageTextSize = 0.5
local textZoomFudge = 5
local pageAnimationSeconds = 0.01
local buttonHoverAlpha = 0.6
local buttonEnabledAlphaMultiplier = 0.8 -- this is multiplied to the current alpha (including the hover alpha) if "clicked"

t[#t+1] = Def.Quad {
    Name = "DownloadsBGQuad",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "PrimaryBackground")
    end
}

t[#t+1] = Def.Quad {
    Name = "DownloadsLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.TopLipHeight)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "SecondaryBackground")
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "DownloadsTitle",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
        self:zoom(titleTextSize)
        self:maxwidth(actuals.Width / titleTextSize - textZoomFudge)
        self:settext(translations["Title"])
        registerActorToColorConfigElement(self, "main", "PrimaryText")
    end
}

local function toolTipOn(msg)
    TOOLTIP:SetText(msg)
    TOOLTIP:Show()
end

-- produces all the fun stuff in the pack downloader
local function downloadsList()
    local itemCount = 25
    local listAllottedSpace = actuals.Height - actuals.TopLipHeight - actuals.ItemListUpperGap - actuals.TopLipHeight
    local inBundles = false
    local downloaderframe = nil
    local searchstring = ""

    -- fallback behavior: this is a PackList
    -- it has internal sorting properties we will use to our advantage
    local pl = PackList:new()
    local packlisting = pl:GetPackTable()
    local downloadingPacks = DLMAN:GetDownloadingPacks()
    local queuedPacks = DLMAN:GetQueuedPacks()
    local downloadingPacksByName = {}
    local queuedPacksByName = {}

    -- these are the defined bundle types
    -- note that each bundle has an expanded version
    local bundleTypes = {
        "Novice",
        "Beginner",
        "Intermediate",
        "Advanced",
        "Expert",
    }

    local page = 1
    local maxPage = 1

    local function movePage(n)
        if not inBundles then
            if maxPage <= 1 then
                return
            end

            -- math to make pages loop both directions
            local nn = (page + n) % (maxPage + 1)
            if nn == 0 then
                nn = n > 0 and 1 or maxPage
            end
            page = nn
            if downloaderframe ~= nil then
                downloaderframe:playcommand("UpdateItemList")
            end
        end
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
            {   -- Enter or Exit from Bundle Select
                Name = "bundleselect",
                Type = "Tap",
                Display = {translations["BundleSelect"], translations["Back"]},
                IndexGetter = function()
                    if inBundles then
                        return 2
                    else
                        return 1
                    end
                end,
                Condition = function() return true end,
                TapFunction = function()
                    inBundles = not inBundles
                    page = 1
                end,
            },
            {   -- Cancel all current downloads
                Name = "cancelall",
                Type = "Tap",
                Display = {translations["CancelAll"]},
                IndexGetter = function() return 1 end,
                Condition = function() return true end,
                TapFunction = function()
                    local count = 0
                    for _, p in ipairs(DLMAN:GetQueuedPacks()) do
                        local s = p:RemoveFromQueue()
                        if s then count = count + 1 end
                    end
                    for _, p in ipairs(DLMAN:GetDownloadingPacks()) do
                        p:GetDownload():Stop()
                        count = count + 1
                    end
                    if count > 0 then
                        ms.ok("Stopped All Downloads: "..count.." Downloads")
                    end
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
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
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

    -- for the pack list
    local function listItem(i)
        local index = i
        local pack = nil
        local bundle = nil

        return Def.ActorFrame {
            InitCommand = function(self)
                self:y(actuals.TopLipHeight + actuals.ItemListUpperGap + listAllottedSpace / itemCount * (i-1))
            end,
            SetPackCommand = function(self)
                self:finishtweening()
                self:diffusealpha(0)
                self:smooth(pageAnimationSeconds * i)
                pack = nil
                bundle = nil
                -- mixing bundle behavior with packs
                -- ASSUMPTION: bundles arent going to take up more than 1 page
                if inBundles then
                    if i <= #bundleTypes * 2 then
                        index = math.ceil(i/2) -- 1 = 1, 2 = 1, 3 = 2, 4 = 2 ...
                        -- this index points to bundleTypes index
                        -- if i % 2 == 0 then it is an expanded bundle
                        self:diffusealpha(1)
                        bundle = DLMAN:GetCoreBundle(bundleTypes[index]:lower()..(i%2==0 and "-expanded" or ""))
                    end
                else
                    index = (page-1) * itemCount + i
                    pack = packlisting[index]
                    if pack ~= nil then
                        self:diffusealpha(1)
                    end
                end 
            end,

            LoadFont("Common Normal") .. {
                Name = "Index",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(actuals.IndexColumnLeftGap / 2)
                    self:zoom(indexTextSize)
                    -- without this random 2, the index touches the left edge of the frame and it feels really weird
                    self:maxwidth(actuals.IndexColumnLeftGap / indexTextSize - 2)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        self:settext(index)
                    elseif bundle ~= nil then
                        self:settext(i)
                    end
                end,
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.NameColumnLeftGap)
                    self:zoom(nameTextSize)
                    self:maxwidth((actuals.MSDColumnLeftGap - actuals.NameColumnLeftGap - actuals.MSDWidth / 2) / nameTextSize - textZoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                    self.alphaDeterminingFunction = function(self)
                        if isOver(self) and pack ~= nil then self:diffusealpha(buttonHoverAlpha) else self:diffusealpha(1) end
                    end
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        self:settext(pack:GetName())
                    elseif bundle ~= nil then
                        local expanded = i % 2 == 0 and " "..translations["Expanded"] or ""
                        self:settext(translations[bundleTypes[index]] .. expanded)
                    end
                    self:alphaDeterminingFunction()
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if pack ~= nil then
                        local urlstring = "https://etternaonline.com/pack/" .. pack:GetID()
					    GAMESTATE:ApplyGameCommand("urlnoexit," .. urlstring)
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "AverageMSD",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(actuals.MSDColumnLeftGap)
                    self:zoom(msdTextSize)
                    self:maxwidth(actuals.MSDWidth / msdTextSize - textZoomFudge)
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        local msd = pack:GetAvgDifficulty()
                        self:settextf("%0.2f", msd)
                        self:diffuse(colorByMSD(msd))
                    elseif bundle ~= nil then
                        local msd = bundle.AveragePackDifficulty
                        self:settextf("%0.2f", msd)
                        self:diffuse(colorByMSD(msd))
                    end
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "Size",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.SizeColumnLeftGap)
                    self:zoom(sizeTextSize)
                    self:maxwidth((actuals.SizeColumnLeftGap - actuals.MSDColumnLeftGap - actuals.MSDWidth / 2) / sizeTextSize - textZoomFudge)
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        local sz = pack:GetSize() / 1024 / 1024
                        self:settextf("%i%s", sz, translations["Megabytes"])
                        self:diffuse(colorByFileSize(sz))
                    elseif bundle ~= nil then
                        local sz = bundle.TotalSize
                        self:settextf("%i%s", sz, translations["Megabytes"])
                        self:diffuse(colorByFileSize(sz))
                    end
                end,
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "packdlicon")) .. {
                Name = "MainDL",
                InitCommand = function(self)
                    -- white: not installed, can queue
                    -- installed: green
                    self:halign(0):valign(0)
                    self:x(actuals.MainDLLeftGap)
                    self:zoomto(actuals.DLIconSize, actuals.DLIconSize)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetPack")
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        self:playcommand("UpdateVisibilityByDownloadStatus")
                    elseif bundle ~= nil then
                        self:diffuse(COLORS:getDownloaderColor("NotInstalledIcon"))
                        self:diffusealpha(isOver(self) and buttonHoverAlpha or 1)
                        if isOver(self) then toolTipOn(translations["DownloadBundle"]) end
                    else
                        self:diffusealpha(0)
                    end
                end,
                UpdateVisibilityByDownloadStatusCommand = function(self)
                    if pack ~= nil then
                        local name = pack:GetName()
                        if SONGMAN:DoesSongGroupExist(name) then
                            -- the pack is already installed
                            self:diffuse(COLORS:getDownloaderColor("InstalledIcon"))
                            self:diffusealpha(1)
                            if isOver(self) then toolTipOn(translations["AlreadyInstalled"]) end
                        elseif downloadingPacksByName[name] ~= nil or queuedPacksByName[name] ~= nil then
                            -- the pack is downloading or queued
                            self:diffusealpha(0)
                        else
                            self:diffuse(COLORS:getDownloaderColor("NotInstalledIcon"))
                            self:diffusealpha(isOver(self) and buttonHoverAlpha or 1)
                            if isOver(self) then toolTipOn(translations["DownloadPack"]) end
                        end
                    end
                end,
                MouseDownCommand = function(self)
                    if self:IsInvisible() then return end
                    if pack ~= nil then
                        local name = pack:GetName()
                        if downloadingPacksByName[name] ~= nil or queuedPacksByName[name] ~= nil or SONGMAN:DoesSongGroupExist(name) then
                            return
                        end
                        pack:DownloadAndInstall(false)
                    elseif bundle ~= nil then
                        local name = bundleTypes[index]:lower()..(i%2==0 and "-expanded" or "")
                        DLMAN:DownloadCoreBundle(name)
                        inBundles = false
                        page = 1
                    end
                    self:GetParent():GetParent():playcommand("UpdateItemList")
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    if pack ~= nil then
                        local name = pack:GetName()
                        if downloadingPacksByName[name] ~= nil then
                            toolTipOn(translations["CurrentlyDownloading"])
                        elseif queuedPacksByName[name] ~= nil then
                            toolTipOn(translations["Queued"])
                        elseif SONGMAN:DoesSongGroupExist(name) then
                            toolTipOn(translations["AlreadyInstalled"])
                        else
                            toolTipOn(translations["DownloadPack"])
                            self:diffusealpha(buttonHoverAlpha)
                        end
                    elseif bundle ~= nil then
                        toolTipOn(translations["DownloadBundle"])
                        self:diffusealpha(buttonHoverAlpha)
                    end
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(1)
                    TOOLTIP:Hide()
                end,
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "packdlicon")) .. {
                Name = "MirrorDL",
                InitCommand = function(self)
                    -- white: not installed, can queue
                    -- installed: green
                    self:halign(0):valign(0)
                    self:x(actuals.MirrorDLLeftGap)
                    self:zoomto(actuals.DLIconSize, actuals.DLIconSize)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetPack")
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        self:playcommand("UpdateVisibilityByDownloadStatus")
                    elseif bundle ~= nil then
                        self:diffuse(COLORS:getDownloaderColor("NotInstalledIcon"))
                        self:diffusealpha(isOver(self) and buttonHoverAlpha or 1)
                        if isOver(self) then toolTipOn(translations["DownloadBundleMirrored"]) end
                    else
                        self:diffusealpha(0)
                    end
                end,
                UpdateVisibilityByDownloadStatusCommand = function(self)
                    if pack ~= nil then
                        -- hide mirror DL if literally the same link
                        if pack:GetURL() == pack:GetMirror() then
                            self:diffusealpha(0)
                            return
                        end

                        local name = pack:GetName()
                        if SONGMAN:DoesSongGroupExist(name) then
                            -- the pack is already installed
                            self:diffuse(COLORS:getDownloaderColor("InstalledIcon"))
                            self:diffusealpha(1)
                            if isOver(self) then toolTipOn(translations["AlreadyInstalled"]) end
                        elseif downloadingPacksByName[name] ~= nil or queuedPacksByName[name] ~= nil then
                            -- the pack is downloading or queued
                            self:diffusealpha(0)
                        else
                            self:diffuse(COLORS:getDownloaderColor("NotInstalledIcon"))
                            self:diffusealpha(isOver(self) and buttonHoverAlpha or 1)
                            if isOver(self) then toolTipOn(translations["DownloadPackMirrored"]) end
                        end
                    end
                end,
                MouseDownCommand = function(self)
                    if self:IsInvisible() then return end
                    if pack ~= nil then
                        local name = pack:GetName()
                        if downloadingPacksByName[name] ~= nil or queuedPacksByName[name] ~= nil or SONGMAN:DoesSongGroupExist(name) then
                            return
                        elseif bundle ~= nil then
                            local expanded = i % 2 == 0 and " Expanded" or ""
                            local name = bundleTypes[index]:lower()..(i%2==0 and "-expanded" or "")
                            DLMAN:DownloadCoreBundle(name, true)
                            inBundles = false
                            page = 1
                        end
                        pack:DownloadAndInstall(true)
                        self:GetParent():GetParent():playcommand("UpdateItemList")
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    if pack ~= nil then
                        local name = pack:GetName()
                        if downloadingPacksByName[name] ~= nil then
                            toolTipOn(translations["CurrentlyDownloading"])
                        elseif queuedPacksByName[name] ~= nil then
                            toolTipOn(translations["Queued"])
                        elseif SONGMAN:DoesSongGroupExist(name) then
                            toolTipOn(translations["AlreadyInstalled"])
                        else
                            toolTipOn(translations["DownloadPackMirrored"])
                            self:diffusealpha(buttonHoverAlpha)
                        end
                    elseif bundle ~= nil then
                        toolTipOn(translations["DownloadBundleMirrored"])
                        self:diffusealpha(buttonHoverAlpha)
                    end
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(1)
                    TOOLTIP:Hide()
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "CancelButton",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    local width = actuals.MirrorDLLeftGap - actuals.MainDLLeftGap + actuals.DLIconSize
                    txt:halign(0):valign(0)
                    bg:halign(0):valign(0)

                    self:x(actuals.MainDLLeftGap)
                    txt:settext(" ")
                    txt:zoom(cancelTextSize)
                    bg:zoomto(width, txt:GetZoomedHeight())
                    txt:maxwidth(width / cancelTextSize - textZoomFudge)

                    txt:settext(" ")
                    self:diffusealpha(0)
                    -- if clicked, cancels download or removes from queue
                    -- otherwise:
                    -- is invisible if not queued or in progress
                    -- says "Queued" if in queue
                    -- says "Cancel" if downloading
                    -- invisible if fully installed
                    registerActorToColorConfigElement(txt, "main", "SecondaryText")
                end,
                SetPackCommand = function(self)
                    if pack ~= nil then
                        self:playcommand("UpdateVisibilityByDownloadStatus")
                    else
                        self:diffusealpha(0)
                        -- button layer management
                        self:z(-5)
                    end
                end,
                UpdateVisibilityByDownloadStatusCommand = function(self)
                    if pack ~= nil then
                        local name = pack:GetName()
                        -- z movement is for button layer management
                        -- lower z has less priority, is "on bottom"
                        if SONGMAN:DoesSongGroupExist(name) then
                            -- the pack is already installed
                            self:diffusealpha(0)
                            self:z(-5)
                        elseif downloadingPacksByName[name] ~= nil then
                            -- the pack is downloading
                            self:diffusealpha(isOver(self:GetChild("BG")) and buttonHoverAlpha or 1)
                            if isOver(self) then TOOLTIP:Hide() end
                            self:GetChild("Text"):settext(translations["Cancel"])
                            self:z(5)
                        elseif queuedPacksByName[name] ~= nil then
                            -- the pack is queued
                            self:diffusealpha(isOver(self:GetChild("BG")) and buttonHoverAlpha or 1)
                            if isOver(self) then TOOLTIP:Hide() end
                            self:GetChild("Text"):settext(translations["Queued"])
                            self:z(5)
                        else
                            self:diffusealpha(0)
                            self:z(-5)
                        end
                    end
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update ~= "OnMouseDown" then return end
                    if pack ~= nil then
                        local name = pack:GetName()
                        if queuedPacksByName[name] then
                            pack:RemoveFromQueue()
                        elseif downloadingPacksByName[name] then
                            downloadingPacksByName[name]:Stop()
                        else
                            return
                        end
                        self:GetParent():GetParent():playcommand("UpdateItemList")
                    elseif bundle ~= nil then
                        return
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" then
                        self:diffusealpha(buttonHoverAlpha)
                        TOOLTIP:Hide()
                    else
                        self:diffusealpha(1)
                    end
                end,
            }
        }
    end

    local t = Def.ActorFrame {
        Name = "DownloaderInternalFrame",
        InitCommand = function(self)
        end,
        BeginCommand = function(self)
            -- make sure we arent filtering anything out on first load
            pl:FilterAndSearch("", 0, 0, 0, 0)
            downloaderframe = self
            self:playcommand("UpdateItemList")

            -- this function will update the downloading pack and queued pack lists
            -- pack downloads are sequential but can potentially become concurrent, so the downloading is a table of length 1
            self:SetUpdateFunction(function(self)
                if not focused then return end -- dont update if cant see
                downloadingPacks = DLMAN:GetDownloadingPacks()
                queuedPacks = DLMAN:GetQueuedPacks()
                downloadingPacksByName = {}
                queuedPacksByName = {}
                for _, pack in ipairs(queuedPacks) do
                    queuedPacksByName[pack:GetName()] = true
                end
                for _, pack in ipairs(downloadingPacks) do
                    downloadingPacksByName[pack:GetName()] = pack:GetDownload()
                end
                self:playcommand("UpdateVisibilityByDownloadStatus")
            end)
            self:SetUpdateFunctionInterval(0.25) -- slow down updates to quarter second

            local snm = SCREENMAN:GetTopScreen():GetName()
            local anm = self:GetName()
            -- init the input context but start it out false
            CONTEXTMAN:RegisterToContextSet(snm, "Downloads", anm)
            CONTEXTMAN:ToggleContextSet(snm, "Downloads", false)

            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                -- if context is set to Downloads, passthrough unless not holding ctrl and a number
                -- pressing a number with ctrl should lead to the general tab
                -- otherwise, typing numbers is allowed
                if CONTEXTMAN:CheckContextSet(snm, "Downloads") then
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
                            local ctrl = INPUTFILTER:IsControlPressed()
                            local copypasta = btn == "DeviceButton_v" and ctrl
                            
                            -- if ctrl is pressed with a number, let the general tab input handler deal with this
                            if char ~= nil and tonumber(char) and INPUTFILTER:IsControlPressed() then
                                return
                            end

                            -- paste
                            if copypasta then
                                char = Arch.getClipboard()
                            end

                            local searchb4 = searchstring

                            if bs then
                                searchstring = searchstring:sub(1, -2)
                            elseif del then
                                searchstring = ""
                            elseif char ~= nil then
                                searchstring = searchstring .. char
                            elseif up then
                                -- up move the page up
                                movePage(-1)
                            elseif down then
                                -- down move the page down
                                movePage(1)
                            else
                                if char == nil then return end
                            end

                            -- reset page if search changed
                            if searchb4 ~= searchstring then
                                page = 1
                            end
                            self:playcommand("UpdateSearch")
                            self:playcommand("UpdateItemList")
                        end
                    end
                end
            
            end)
        end,
        UpdateSearchCommand = function(self)
            pl:FilterAndSearch(searchstring, 0, 0, 0, 0)
        end,
        UpdateItemListCommand = function(self)
            TOOLTIP:Hide()
            if not inBundles then
                packlisting = pl:GetPackTable()
                maxPage = math.ceil(#packlisting / itemCount)
            end
            self:playcommand("SetPack")
        end,

        Def.ActorFrame {
            Name = "PackSearchFrame",
            InitCommand = function(self)
                self:xy(actuals.SearchBGLeftGap, actuals.TopLipHeight / 2)
            end,
            UpdateItemListCommand = function(self)
                if inBundles then
                    self:diffusealpha(0)
                else
                    self:diffusealpha(1)
                end
            end,

            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "searchBar")) .. {
                Name = "PackSearchBG",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoomto(actuals.SearchBGWidth, actuals.SearchBGHeight)
                    self:diffusealpha(0.35)
                end
            },
            Def.Sprite {
                Name = "PackSearchIcon",
                Texture = THEME:GetPathG("", "searchIcon"),
                InitCommand = function(self)
                    self:halign(0)
                    self:x(actuals.SearchIconLeftGap)
                    self:zoomto(actuals.SearchIconSize, actuals.SearchIconSize)
                    registerActorToColorConfigElement(self, "main", "IconColor")
                end
            },
            LoadFont("Common Normal") .. {
                Name = "PackSearchText",
                InitCommand = function(self)
                    self:halign(0)
                    self:x(actuals.SearchTextLeftGap)
                    self:zoom(searchTextSize)
                    self:maxwidth((actuals.SearchBGWidth - actuals.SearchTextLeftGap*1.5) / searchTextSize - textZoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                UpdateSearchCommand = function(self)
                    self:settext(searchstring)
                end,
            }
        },
        LoadFont("Common Normal") .. {
            Name = "IndexHeader",
            InitCommand = function(self)
                self:valign(0)
                self:xy(actuals.IndexColumnLeftGap / 2, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                self:zoom(indexHeaderSize)
                self:maxwidth(actuals.IndexColumnLeftGap / indexHeaderSize - textZoomFudge)
                self:settext("#")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
        },
        UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "NameHeader",
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                local width = actuals.MSDColumnLeftGap - actuals.NameColumnLeftGap - actuals.MSDWidth / 2
                self:xy(actuals.NameColumnLeftGap, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                
                txt:halign(0):valign(0)
                bg:halign(0):valign(0)
                txt:zoom(nameHeaderSize)
                txt:maxwidth(width / nameHeaderSize - textZoomFudge)
                txt:settext(translations["HeaderName"])
                bg:zoomto(math.max(width/2, txt:GetZoomedWidth()), txt:GetZoomedHeight())
                registerActorToColorConfigElement(txt, "main", "PrimaryText")
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update ~= "OnMouseDown" then return end
                pl:SortByName()
                self:GetParent():playcommand("UpdateItemList")
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
        UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "AverageHeader",
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                local width = actuals.MSDWidth
                self:xy(actuals.MSDColumnLeftGap, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                
                txt:valign(0)
                bg:valign(0)
                txt:zoom(msdHeaderSize)
                txt:maxwidth(width / msdHeaderSize - textZoomFudge)
                txt:settext(translations["HeaderAverage"])
                bg:zoomto(width, txt:GetZoomedHeight())
                registerActorToColorConfigElement(txt, "main", "PrimaryText")
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update ~= "OnMouseDown" then return end
                pl:SortByDiff()
                self:GetParent():playcommand("UpdateItemList")
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
        UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "Size Header",
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                local width = actuals.SizeColumnLeftGap - actuals.MSDColumnLeftGap - actuals.MSDWidth / 2
                self:xy(actuals.SizeHeaderLeftGap, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                
                txt:valign(0)
                bg:valign(0)
                txt:zoom(sizeHeaderSize)
                txt:maxwidth(width / sizeHeaderSize - textZoomFudge)
                txt:settext(translations["HeaderSize"])
                bg:zoomto(width, txt:GetZoomedHeight())
                registerActorToColorConfigElement(txt, "main", "PrimaryText")
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update ~= "OnMouseDown" then return end
                pl:SortBySize()
                self:GetParent():playcommand("UpdateItemList")
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
                self:xy(actuals.Width - actuals.PageTextRightGap, actuals.TopLipHeight + actuals.HeaderLineUpperGap)
                self:zoom(pageTextSize)
                self:maxwidth((actuals.Width - actuals.SizeColumnLeftGap) / pageTextSize - textZoomFudge)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateItemListCommand = function(self)
                if inBundles then
                    self:settext("")
                else
                    local lb = clamp((page-1) * (itemCount) + 1, 0, #packlisting)
                    local ub = clamp(page * itemCount, 0, #packlisting)
                    self:settextf("%d-%d/%d", lb, ub, #packlisting)
                end
            end
        },
    }
    
    for i = 1, itemCount do
        t[#t+1] = listItem(i)
    end

    t[#t+1] = tabChoices()

    return t
end

t[#t+1] = downloadsList()

return t