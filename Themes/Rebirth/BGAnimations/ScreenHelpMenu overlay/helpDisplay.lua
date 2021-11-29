
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
    EdgeBuffer = ratios.EdgeBuffer * SCREEN_WIDTH,
    IconExitWidth = ratios.IconExitWidth * SCREEN_WIDTH,
    IconExitHeight = ratios.IconExitHeight * SCREEN_HEIGHT,
}

local infoTextSize = 0.65
local textZoomFudge = 5
local buttonHoverAlpha = 0.6

-- special handling to make sure our beautiful icon doesnt get tarnished
local logosourceHeight = 133
local logosourceWidth = 102
local logoratio = math.min(1920 / SCREEN_WIDTH, 1080 / SCREEN_HEIGHT)
local logoH, logoW = getHWKeepAspectRatio(logosourceHeight, logosourceWidth, logosourceWidth / logosourceWidth)

local function helpMenu()
    local items = {
        "a",
    }

    local cursorIndex = 1
    local function moveCursor(n)
        local newpos = cursorIndex + n
        if newpos > #items then newpos = 1 end
        if newpos < 1 then newpos = #items end
        cursorIndex = newpos
        MESSAGEMAN:Broadcast("UpdateCursor")
    end

    local function menuItem(i)
        local yIncrement = (actuals.MainDisplayHeight) / #items
        return Def.ActorFrame {
            Name = "MenuItem_"..i,
            InitCommand = function(self)
                -- center y
                self:y(yIncrement * (i-1) + yIncrement / 2)
            end,
            SelectCurrentCommand = function(self)
                if cursorIndex == i then
                    -- do something
                end
            end,

        }
    end

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
                elseif down or right then
                    moveCursor(1)
                elseif enter then
                    self:playcommand("SelectCurrent")
                elseif back then
                    SCREENMAN:GetTopScreen():Cancel()
                end
            end)
            self:playcommand("UpdateCursor")
        end,
    }

    for i = 1, #items do
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
            Texture = THEME:GetPathG("", "Logo"),
            InitCommand = function(self)
                self:xy(actuals.ScrollerWidth + (actuals.ListWidth / 2), actuals.InfoHeight / 2)
                self:zoomto(logoW, logoH)
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
                self:settext("Ayo we got the mofukin boneless screen")
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
                TOOLTIP:SetText("Exit")
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
