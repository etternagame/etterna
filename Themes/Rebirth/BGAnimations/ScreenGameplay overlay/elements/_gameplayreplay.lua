local buttons = {}
local bobo
local function spaceButtons(value)
    for i, b in ipairs(buttons) do
        b:addy((i-1) * value)
    end
    bobo:playcommand("ChangeHeight", {val = buttons[#buttons]:GetChild("Label"):GetY() +  buttons[#buttons]:GetChild("Label"):GetHeight()/2})
end

local modifierPressed = false
local forward = true
local position = 0

local scroller  -- just an alias for the actor that runs the commands

local function input(event)
    
end

local function getNewSongPos()
    local currentpos = SCREENMAN:GetTopScreen():GetSongPosition()
    local newpos = currentpos + (modifierPressed and 0.1 or 5) * (forward and 1 or -1)
    --SCREENMAN:SystemMessage(string.format("%f to %f", currentpos, newpos))
    return newpos
end

local function getNewRate()
    local currentrate = GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate()
    local newrate = currentrate + (modifierPressed and 0.05 or 0.1) * (forward and 1 or -1)
    --SCREENMAN:SystemMessage(string.format("%f to %f", currentrate, newrate))
    return newrate
end

scroller = Def.ActorFrame {
    Name = "ReplayButtons",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
    end,
    OnCommand = function(self)
        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            --SCREENMAN:SystemMessage(event.DeviceInput.button)
            if event.DeviceInput.button == "DeviceButton_right ctrl" or event.DeviceInput.button == "DeviceButton_left ctrl" then
                modifierPressed = not (event.type == "InputEventType_Release")
            end
            if event.DeviceInput.button == "DeviceButton_right shift" or event.DeviceInput.button == "DeviceButton_left shift" then
                ratePressed = not (event.type == "InputEventType_Release")
            end
            if event.DeviceInput.button == "DeviceButton_right alt" or event.DeviceInput.button == "DeviceButton_left alt" then
                bookmarkPressed = not (event.type == "InputEventType_Release")
            end
            if event.type ~= "InputEventType_Release" then
                if event.GameButton == "EffectUp" then
                    if bookmarkPressed then
                        self:queuecommand("ReplayBookmarkSet")
                        return false
                    end
                    forward = true
                    if ratePressed then
                        self:queuecommand("ReplayRate")
                    else
                        self:queuecommand("ReplayScroll")
                    end
                elseif event.GameButton == "EffectDown" then
                    if bookmarkPressed then
                        self:queuecommand("ReplayBookmarkGoto")
                        return false
                    end
                    forward = false
                    if ratePressed then
                        self:queuecommand("ReplayRate")
                    else
                        self:queuecommand("ReplayScroll")
                    end
                elseif event.GameButton == "Coin" then
                    self:queuecommand("ReplayPauseToggle")
                end
            end
        end)
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.ReplayButtonsX, MovableValues.ReplayButtonsY)
    end,
    ReplayScrollCommand = function(self)
        local newpos = getNewSongPos()
        SCREENMAN:GetTopScreen():SetSongPosition(newpos)
    end,
    ReplayRateCommand = function(self)
        local newrate = getNewRate()
        local givenrate = SCREENMAN:GetTopScreen():SetRate(newrate)
        if givenrate ~= nil then
            local realnewrate = notShit.round(givenrate, 3)
        --SCREENMAN:SystemMessage(string.format("Set rate to %f", realnewrate))
        end
    end,
    ReplayPauseToggleCommand = function(self)
        SCREENMAN:GetTopScreen():TogglePause()
    end,
    ReplayBookmarkSetCommand = function(self)
        position = SCREENMAN:GetTopScreen():GetSongPosition()
        SCREENMAN:GetTopScreen():SetBookmark(position)
    end,
    ReplayBookmarkGotoCommand = function(self)
        SCREENMAN:GetTopScreen():JumpToBookmark()
    end,
}
local span = 50
local x = -1 * span
local textSize = 0.8
local hoverAlpha = 0.6
local width = 60
local height = 30
local uiBGAlpha = 0.6

local translations = {
    Pause = THEME:GetString("ScreenGameplay", "ButtonPause"),
    FastForward = THEME:GetString("ScreenGameplay", "ButtonFastForward"),
    Rewind = THEME:GetString("ScreenGameplay", "ButtonRewind"),
    Play = THEME:GetString("ScreenGameplay", "ButtonPlay"),
    Results = THEME:GetString("ScreenGameplay", "Results"),
    Exit = THEME:GetString("ScreenGameplay", "Exit"),
    MustBePaused = THEME:GetString("ScreenGameplay", "MustBePaused"),
}

local function button(i, txt, click, mustBePaused)
    return Def.ActorFrame {
        InitCommand = function(self)
            self:y(x + span*i) -- wow this is bad
        end,

        Def.Quad {
            Name = "Border",
            InitCommand = function(self)
                self:halign(1)
                self:zoomto(width, height)
                self:diffuse(COLORS:getColor("replay", "ButtonBorder"))
                self:diffusealpha(1)
            end,
        },
        UIElements.QuadButton(1, 1) .. {
            Name = "BG",
            InitCommand = function(self)
                self:halign(1)
                self:x(-1)
                self:zoomto(width - 2, height - 2)
                self:diffuse(COLORS:getColor("replay", "ButtonBG"))
                self:diffusealpha(1)
            end,
            MouseOverCommand = function(self)
                self:diffusealpha(hoverAlpha)
                if mustBePaused then
                    if not GAMESTATE:IsPaused() then
                        TOOLTIP:SetText(translations["MustBePaused"])
                        TOOLTIP:Show()
                    end
                end
            end,
            MouseOutCommand = function(self)
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
            MouseDownCommand = function(self, params)
                if params and params.event == "DeviceButton_left mouse button" then
                    click(self)
                end
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "Text",
            InitCommand = function(self)
                self:x(-width / 2)
                self:zoom(textSize)
                self:maxwidth(width / textSize)
                self:settext(txt)
                self:diffuse(COLORS:getColor("gameplay", "PrimaryText"))
                self:diffusealpha(1)
            end,
        }
    }
end

scroller[#scroller + 1] = Def.ActorFrame {
    Name = "ReplayButtons",
    InitCommand = function(self)
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            spacingInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    UIElements.QuadButton(1) .. {
        Name = "DraggableBox",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoomto(width * 1.5, height * 3.5 + span * 4)
            self:xy(width/4, -span)
            registerActorToColorConfigElement(self, "main", "SecondaryBackground")
            self:diffusealpha(uiBGAlpha)
        end,
        MouseDragCommand = function(self, params)
            local newx = params.MouseX - (self.initialClickX or 0)
            local newy = params.MouseY - (self.initialClickY or 0)
            self:GetParent():addx(newx):addy(newy)
        end,
        MouseDownCommand = function(self, params)
            self.initialClickX = params.MouseX
            self.initialClickY = params.MouseY
        end,
    },
    
    button(1,
        translations["Pause"],
        function(self)
            SCREENMAN:GetTopScreen():TogglePause()
            local paused = GAMESTATE:IsPaused()
            self:GetParent():GetChild("Text"):settext(paused and translations["Play"] or translations["Pause"])
        end
    ),
    button(2,
        translations["FastForward"],
        function(self)
            SCREENMAN:GetTopScreen():SetSongPosition(SCREENMAN:GetTopScreen():GetSongPosition() + 5)
        end,
        true
    ),
    button(3,
        translations["Rewind"],
        function(self)
            SCREENMAN:GetTopScreen():SetSongPosition(SCREENMAN:GetTopScreen():GetSongPosition() - 5)
        end,
        true
    ),
    button(4,
        translations["Results"],
        function(self)
            SCREENMAN:GetTopScreen():PostScreenMessage("SM_NotesEnded", 0)
        end
    ),
    button(5,
        translations["Exit"],
        function(self)
            SCREENMAN:GetTopScreen():Cancel()
        end
    ),
}
return scroller
