
local ratios = {
    InfoTopGap = 36 / 1080, -- top edge screen to top edge box
    InfoLeftGap = 384 / 1920, -- left edge screen to left edge box
    InfoWidth = 1153 / 1920, -- small box width
    InfoHeight = 150 / 1080, -- small box height
    InfoHorizontalBuffer = 40 / 1920, -- from side of box to side of text
    InfoVerticalBuffer = 28 / 1080, -- from top/bottom edge of box to top/bottom edge of text

    MainDisplayTopGap = 216 / 1080, -- top edge screen to top edge box
    MainDisplayLeftGap = 225 / 1920, -- left edge screen to left edge box
    MainDisplayWidth = 1471 / 1920, -- big box width
    MainDisplayHeight = 648 / 1080, -- big box height

    BundleListTopGap = 24 / 1080, -- top edge of box to top of button
    BundleListEdgeBuffer = 39 / 1920, -- left edge of box to left edge of button, also from right edge big box to right edge text
    BundleListTextBuffer = 10 / 1920, -- from bundle button to text
    BundleItemWidth = 404 / 1920, -- button width
    BundleItemGap = 29 / 1080, -- gap in between items

    ProgressTextBottomGap = 105 / 1080, -- bottom screen to bottom text
    -- progress text is centered screen
    ProgressBarBottomGap = 49 / 1080, -- bottom screen to bottom bar
    ProgressBarWidth = 1255 / 1920, -- width
    ProgressBarHeight = 25 / 1080, -- height
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
    BundleListTopGap = ratios.BundleListTopGap * SCREEN_HEIGHT,
    BundleListEdgeBuffer = ratios.BundleListEdgeBuffer * SCREEN_WIDTH,
    BundleListTextBuffer = ratios.BundleListTextBuffer * SCREEN_WIDTH,
    BundleItemWidth = ratios.BundleItemWidth * SCREEN_WIDTH,
    BundleItemGap = ratios.BundleItemGap * SCREEN_HEIGHT,
    ProgressTextBottomGap = ratios.ProgressTextBottomGap * SCREEN_HEIGHT,
    ProgressBarBottomGap = ratios.ProgressBarBottomGap * SCREEN_HEIGHT,
    ProgressBarWidth = ratios.ProgressBarWidth * SCREEN_WIDTH,
    ProgressBarHeight = ratios.ProgressBarHeight * SCREEN_HEIGHT,
}

local infoTextSize = 0.37
local progressTextSize = 0.35
local bundleNameTextSize = 0.4
local bundleDescTextSize = 0.4
local textZoomFudge = 5
local buttonHoverAlpha = 0.6

local function bundleList()
    local bundles = {
        {
            Name = "Novice",
            Color = "#66ccff",
            Description = "A bundle aimed at people who are entirely new to rhythm games.\nMostly single notes throughout the song and very little pattern complexity.",
        },
        {
            Name = "Beginner",
            Color = "#099948",
            Description = "A bundle for those who have formed some muscle memory.\nJumps (2 note chords) are introduced; some technical patterns start to appear.",
        },
        {
            Name = "Intermediate",
            Color = "#ddaa00",
            Description = "A bundle for players who can confidently play complex patterns.\nJumpstream/handstream, very technical patterns, and jacks are common.",
        },
        {
            Name = "Advanced",
            Color = "#ff6666",
            Description = "A bundle for advanced players.\nDumps are introduced. Very fast patterns in stamina intensive and complex files.",
        },
        {
            Name = "Expert",
            Color = "#c97bff",
            Description = "A bundle for veterans.\nSome of the hardest songs the game has to offer. Nothing is off-limits.",
        },
    }

    local function bundleItem(i)
        local bundle = bundles[i]
        local bundleUserdata = DLMAN:GetCoreBundle(bundle.Name:lower())
        local yIncrement = (actuals.MainDisplayHeight - (actuals.BundleListTopGap*2)) / #bundles
        return Def.ActorFrame {
            Name = "Bundle_"..i,
            InitCommand = function(self)
                -- center y
                self:y(yIncrement * (i-1) + yIncrement / 2)
            end,

            UIElements.QuadButton(1) .. {
                Name = "BG",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoomto(actuals.BundleItemWidth, yIncrement - (actuals.BundleItemGap))
                    self:diffuse(color(bundle.Color))
                end,
                MouseDownCommand = function(self, params)
					DLMAN:DownloadCoreBundle(bundle.Name:lower())
                end,
                MouseOverCommand = function(self, params)
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self, params)
                    self:diffusealpha(1)
                end,
            },
            LoadFont("Common Large") .. {
                Name = "BundleNameSize",
                InitCommand = function(self)
                    self:x(actuals.BundleItemWidth / 2)
                    self:zoom(bundleNameTextSize)
                    self:maxwidth(actuals.BundleItemWidth / bundleNameTextSize - textZoomFudge)
                    self:settextf("%s Bundle (%dMB)", bundle.Name, bundleUserdata.TotalSize)
                end,
            },
            LoadFont("Common Large") .. {
                Name = "BundleDescription",
                InitCommand = function(self)
                    self:halign(0)
                    self:x(actuals.BundleItemWidth + actuals.BundleListTextBuffer)
                    self:zoom(bundleDescTextSize)
                    self:maxheight(yIncrement * 0.75 / bundleDescTextSize)
                    self:wrapwidthpixels((actuals.MainDisplayWidth - actuals.BundleListTextBuffer - actuals.BundleItemWidth - (actuals.BundleListEdgeBuffer*2)) / bundleDescTextSize)
                    self:settext(bundle.Description)
                end,
            }

        }
    end

    local t = Def.ActorFrame {Name = "BundleListContainer"}

    for i = 1, #bundles do
        t[#t+1] = bundleItem(i)
    end
    return t
end


local t = Def.ActorFrame {
    Name = "BundleDisplayFile",

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
        LoadColorFont("Common Large") .. {
            Name = "Text",
            InitCommand = function(self)
                self:halign(0)
                self:xy(actuals.InfoHorizontalBuffer, actuals.InfoHeight/2)
                self:zoom(infoTextSize)
                self:maxheight((actuals.InfoHeight - (actuals.InfoVerticalBuffer*2)) / infoTextSize)
                self:wrapwidthpixels((actuals.InfoWidth - (actuals.InfoHorizontalBuffer*2)) / infoTextSize)
                self:settext("Welcome to Etterna!\nLet's start by installing some songs. Click the button that corresponds to your skill level and the installation will proceed automatically.")
            end,
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
        bundleList() .. {
            InitCommand = function(self)
                self:xy(actuals.BundleListEdgeBuffer, actuals.BundleListTopGap)
            end,
        },
    },
    Def.ActorFrame { 
        Name = "ProgressFrame",
        InitCommand = function(self)
            self:xy(SCREEN_CENTER_X, SCREEN_HEIGHT)
            self:visible(false)
        end,
        DLProgressAndQueueUpdateMessageCommand = function(self)
            local dls = DLMAN:GetDownloads()
            if #dls > 0 then
                local progress = dls[1]:GetKBDownloaded()
                local size = dls[1]:GetTotalKB()
                local kbsec = dls[1]:GetKBPerSecond()
                self:playcommand("UpdateProgress", {
                    progress = progress, -- progress of current dl, not total
                    size = size, -- size of current dl, not total
                    kbsec = kbsec,
                    filesRemaining = #dls-1, -- how many more downloads after the current one
                })
            else
                self:playcommand("ClearProgress")
            end
        end,
        AllDownloadsCompletedMessageCommand = function(self)
            self:playcommand("ClearProgress")
        end,
        UpdateProgressCommand = function(self)
            self:visible(true)
        end,
        ClearProgressCommand = function(self)
            self:visible(false)
        end,

        LoadFont("Common Large") .. {
            Name = "Text",
            InitCommand = function(self)
                self:valign(1)
                self:y(-actuals.ProgressTextBottomGap)
                self:zoom(progressTextSize)
                self:maxwidth(actuals.ProgressBarWidth / progressTextSize)
            end,
            UpdateProgressCommand = function(self, params)
                if params.filesRemaining > 0 then
                    self:settextf("Downloading ... (%5.2f%% %dKB/s %d packs remaining)", params.progress / params.size * 100, params.kbsec, params.filesRemaining)
                else
                    self:settextf("Downloading ... (%5.2f%% %dKB/s)", params.progress / params.size * 100, params.kbsec)
                end
            end,
        },
        Def.ActorFrame {
            Name = "BarContainer",
            InitCommand = function(self)
                self:y(-actuals.ProgressBarBottomGap)
            end,

            Def.Quad {
                Name = "BG",
                InitCommand = function(self)
                    self:valign(1)
                    self:zoomto(actuals.ProgressBarWidth, actuals.ProgressBarHeight)
                    self:diffuse(color("0.7,0.7,0.7"))
                    self:diffusealpha(0.6)
                end,
            },
            Def.Quad {
                Name = "Progress",
                InitCommand = function(self)
                    self:valign(1):halign(0)
                    self:x(-actuals.ProgressBarWidth/2)
                    self:zoomto(0, actuals.ProgressBarHeight)
                    self:diffuse(color("1,1,1"))
                end,
                UpdateProgressCommand = function(self, params)
                    self:zoomx(actuals.ProgressBarWidth * (params.progress / params.size))
                end,
            }
        }
    },
}

return t