local ratios = {
    Width = 782 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything

    IndexColumnLeftGap = 30 /1920, -- left edge to right edge (right align)
    NameColumnLeftGap = 45 / 1920, -- left edge to left edge (left align)
    MSDColumnLeftGap = 495 / 1920, -- left edge to center of text (center align)
    SizeHeaderLeftGap = 592 / 1920, -- left edge to left edge (left align)
    SizeColumnLeftGap = 662 / 1920, -- left edge to right edge (right align)
    MainDLLeftGap = 692 / 1920, -- left edge to left edge
    MirrorDLLeftGap = 734 / 1920, -- left edge to left edge
    DLIconSize = 29 / 1080, -- it is a square
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
}

local visibleframeX = SCREEN_WIDTH - actuals.Width
local visibleframeY = SCREEN_HEIGHT - actuals.Height
local animationSeconds = 0.1
local focused = false

local t = Def.ActorFrame {
    Name = "DownloadsFile",
    InitCommand = function(self)
        -- lets just say uh ... despite the fact that this file might want to be portable ...
        -- lets ... just .... assume it always goes in the same place ... and the playerInfoFrame is the same size always
        self:x(SCREEN_WIDTH)
        self:y(visibleframeY)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:x(SCREEN_WIDTH)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "Downloads" then
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
            self:smooth(animationSeconds)
            self:x(visibleframeX)
        end
    end,
    FinishFocusingCommand = function(self)
        -- the purpose of this is to delay the act of focusing the screen
        -- the reason is that we dont want to trigger Ctrl+3 inputting a 3 on the search field immediately
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Downloads")
    end
}

local titleTextSize = 1
local indexTextSize = 1
local nameTextSize = 1
local msdTextSize = 1
local sizeTextSize = 1
local cancelTextSize = 1

local indexHeaderSize = 1
local nameHeaderSize = 1
local msdHeaderSize = 1
local sizeHeaderSize = 1
local searchTextSize = 1

local textZoomFudge = 5
local buttonHoverAlpha = 0.6
local buttonEnabledAlphaMultiplier = 0.8 -- this is multiplied to the current alpha (including the hover alpha) if "clicked"

t[#t+1] = Def.Quad {
    Name = "DownloadsBGQuad",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = Def.Quad {
    Name = "DownloadsLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.TopLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "DownloadsTitle",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
        self:zoom(titleTextSize)
        self:maxwidth(actuals.Width / titleTextSize - textZoomFudge)
        self:settext("Pack Downloader")
    end
}

-- produces all the fun stuff in the pack downloader
local function downloadsList()
    local itemCount = 16
    local listAllottedSpace = (actuals.Height - actuals.TopLipHeight - actuals.ItemListUpperGap) / 3 * 2
    local bundlesAllottedSpace = (actuals.Height - actuals.TopLipHeight - actuals.ItemListUpperGap) / 3

    local function listItem(i)
        return Def.ActorFrame {
            InitCommand = function(self)
                self:y(actuals.TopLipHeight + actuals.ItemListUpperGap + listAllottedSpace / itemCount * (i-1))
            end,

            LoadFont("Common Normal") .. {
                Name = "Index",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.IndexColumnLeftGap)
                    self:zoom(indexTextSize)
                    self:maxwidth(actuals.IndexColumnLeftGap / indexTextSize - textZoomFudge)
                    self:settext("100")
                end,
                SetPackCommand = function(self)
                end,
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.NameColumnLeftGap)
                    self:zoom(nameTextSize)
                    self:maxwidth((actuals.MSDColumnLeftGap - actuals.NameColumnLeftGap - actuals.MSDWidth / 2) / nameTextSize - textZoomFudge)
                    self:settext("FREE SHIPPING AND HANDLING FOR FREE (free)")
                end,
                SetPackCommand = function(self)
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "AverageMSD",
                InitCommand = function(self)
                    self:valign(0)
                    self:x(actuals.MSDColumnLeftGap)
                    self:zoom(msdTextSize)
                    self:maxwidth(actuals.MSDWidth / msdTextSize - textZoomFudge)
                    self:settext("19.99")
                end,
                SetPackCommand = function(self)
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "Size",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.SizeColumnLeftGap)
                    self:zoom(sizeTextSize)
                    self:maxwidth((actuals.SizeColumnLeftGap - actuals.MSDColumnLeftGap - actuals.MSDWidth / 2) / sizeTextSize - textZoomFudge)
                    self:settext("666MB")
                end,
                SetPackCommand = function(self)
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
                    bg:zoomto(width, txt:GetZoomedHeight())
                    txt:maxwidth(width / cancelTextSize - textZoomFudge)

                    txt:settext("Cancel")
                    self:diffusealpha(0)
                    -- if clicked, cancels download or removes from queue
                    -- otherwise:
                    -- is invisible if not queued or in progress
                    -- says "Queued" if in queue
                    -- says "Cancel" if downloading
                    -- invisible if fully installed
                end,

            }
        }
    end

    -- produces the bundle display
    -- click a bundle to filter the pack list to just the packs in the bundle
    -- click the same bundle to unfilter
    -- one bundle at a time only or none at all
    local function bundleDisplay()
        local bundlespacetopY = actuals.TopLipHeight + actuals.ItemListUpperGap + listAllottedSpace

        -- these are the defined bundle types
        -- note that each bundle has an expanded version
        -- this means 5 elements makes 10 buttons (5 rows, 2 columns)
        local bundleTypes = {
            "Novice",
            "Beginner",
            "Intermediate",
            "Advanced",
            "Expert",
        }

        -- a reduction of code duplication means an increase in blinding complexity
        -- im all for that
        -- this function makes a pair of buttons within an ActorFrame once per bundle
        -- im thinking about this 5 minutes later and this is going to multiply the number of actors in this by like 5
        --  too late already started
        local function makeBundlePair(i)
            local bundleType = bundleTypes[i]

            -- nesty nesty function scary
            -- (its an ActorFrame that holds text and a bg)
            local function bundleButton(j)
                local isExpandedBundle = j == 2
                local isClicked = false

                -- just because i can, add another function to get the children for the frame
                local function getkids(self) return self:GetChild("Text"), self:GetChild("BG") end
                return UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "BundleButtonPairSub_"..j,
                    InitCommand = function(self)
                        local txt, bg = getkids(self)
                        self:x(actuals.Width / 2 * (j-1) + actuals.Width / 4)

                        bg:zoomto(actuals.Width / 5, bundlesAllottedSpace / #bundleTypes / 2 )
                        local ex = isExpandedBundle and " EX" or ""
                        txt:settext(bundleType .. ex)
                        bg:diffusealpha(0.2)
                    end,
                    UpdateHoverCommand = function(self)
                        local txt, bg = getkids(self)
                        if isOver(bg) then
                            self:diffusealpha(buttonHoverAlpha * (isClicked and buttonEnabledAlphaMultiplier or 1))
                        else
                            self:diffusealpha(1)
                        end
                    end,
                    SetBundleCommand = function(self)
                        local txt, bg = getkids(self)

                        -- update alpha in case hovering or not
                        -- assumption: all bundle buttons will be clickable thus able to be hovered
                        self:playcommand("UpdateHover")
                    end,
                    RolloverUpdateCommand = function(self, params)
                        self:playcommand("UpdateHover")
                    end,
                    MouseDownCommand = function(self, params)
                        if params.event == "DeviceButton_left mouse button" then
                            if isClicked then
                                -- unfilter packs
                            else
                                -- filter packs
                            end
                        end
                    end,
                }
            end

            return Def.ActorFrame {
                Name = "BundleButtonPair_"..i,
                InitCommand = function(self)
                    self:y(bundlesAllottedSpace / #bundleTypes * (i-1) + (bundlesAllottedSpace / #bundleTypes / 2))
                end,

                bundleButton(1),
                bundleButton(2),
            }
        end


        local t = Def.ActorFrame {
            Name = "BundleSectionFrame",
            InitCommand = function(self)
                self:y(bundlespacetopY)
            end,
        }

        for i = 1, #bundleTypes do
            t[#t+1] = makeBundlePair(i)
        end

        return t
    end

    local t = Def.ActorFrame {
        Name = "DownloaderInternalFrame",
        InitCommand = function(self)
        end,

        Def.ActorFrame {
            Name = "PackSearchFrame",
            InitCommand = function(self)
                self:xy(actuals.SearchBGLeftGap, actuals.TopLipHeight / 2)
            end,

            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "searchBar")) .. {
                Name = "PackSearchBG",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoomto(actuals.SearchBGWidth, actuals.SearchBGHeight)
                    self:diffusealpha(0.4)
                end
            },
            Def.Sprite {
                Name = "PackSearchIcon",
                Texture = THEME:GetPathG("", "search"),
                InitCommand = function(self)
                    self:halign(0)
                    self:x(actuals.SearchIconLeftGap)
                    self:zoomto(actuals.SearchIconSize, actuals.SearchIconSize)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "PackSearchText",
                InitCommand = function(self)
                    self:halign(0)
                    self:x(actuals.SearchTextLeftGap)
                    self:zoom(searchTextSize)
                    self:settext("this is input text")
                end,
            }
        },
        LoadFont("Common Normal") .. {
            Name = "IndexHeader",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:xy(actuals.IndexColumnLeftGap, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                self:zoom(indexHeaderSize)
                self:maxwidth(actuals.IndexColumnLeftGap / indexHeaderSize - textZoomFudge)
                self:settext("#")
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
                txt:zoom(nameHeaderSize)
                txt:maxwidth(width / nameHeaderSize - textZoomFudge)
                txt:settext("Name")
            end,
        },
        UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "AverageHeader",
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                local width = actuals.MSDWidth
                self:xy(actuals.MSDColumnLeftGap, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                
                txt:valign(0)
                txt:zoom(msdHeaderSize)
                txt:maxwidth(width / msdHeaderSize - textZoomFudge)
                txt:settext("Avg")
            end,
        },
        UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "Size Header",
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                local width = actuals.SizeColumnLeftGap - actuals.MSDColumnLeftGap - actuals.MSDWidth / 2
                self:xy(actuals.SizeHeaderLeftGap, actuals.HeaderLineUpperGap + actuals.TopLipHeight)
                
                txt:valign(0)
                txt:zoom(sizeHeaderSize)
                txt:maxwidth(width / sizeHeaderSize - textZoomFudge)
                txt:settext("Size")
            end,
        }
    }
    
    for i = 1, itemCount do
        t[#t+1] = listItem(i)
    end

    t[#t+1] = bundleDisplay()

    return t
end

t[#t+1] = downloadsList()

return t