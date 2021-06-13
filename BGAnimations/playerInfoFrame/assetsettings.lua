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
                            
                            -- if ctrl is pressed with a number, let the general tab input handler deal with this
                            if char ~= nil and tonumber(char) and INPUTFILTER:IsControlPressed() then
                                return
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

    return t
end

t[#t+1] = assetList()

return t