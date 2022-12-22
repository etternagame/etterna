local boxWidth = SCREEN_WIDTH / 2.5
local boxHeight = SCREEN_HEIGHT / 3

local sideMargin = 15 / 1920 * SCREEN_WIDTH
local bottomMargin = 15 / 1080 * SCREEN_HEIGHT
local INFORMATIONupperGap = 9 / 1080 * SCREEN_HEIGHT
local explainTextUpperGap = 20 / 1080 * SCREEN_HEIGHT
local wtf = 1 / 1080 * SCREEN_HEIGHT -- wtf
local INFORMATIONBOXHEIGHT = 50 / 1080 * SCREEN_HEIGHT
local exitwidth = 70 / 1920 * SCREEN_WIDTH
local exitheight = 50 / 1080 * SCREEN_HEIGHT

local translations = {
    Information = THEME:GetString("ScreenTextEntry", "Information"),
    Exit = THEME:GetString("ScreenTextEntry", "Exit"),
}

local dimAlpha = 0.6
local boxAlpha = 0.5
local hoverAlpha = 0.6
local bgColor = color("0,0,0")
local boxColor = color("0,0,0")

local exittextsize = 0.86
local explaintextsize = 1
local INFORMATIONtextsize = 1

return Def.ActorFrame {
    Name = "TextEntryUnderlayFile",
    InitCommand = function(self)
        self:diffusealpha(0)
    end,
    OnCommand = function(self)
        local question = self:GetParent():GetChild("Question")
        local answer = self:GetParent():GetChild("Answer")

        self:smooth(0.5)
        self:diffusealpha(1)
        if question then
            question:maxwidth(boxWidth / question:GetZoom())
        end
        if answer then
            answer:maxwidth(boxWidth / answer:GetZoom())
        end
    end,

    Def.Quad {
        Name = "DimBG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
            self:diffuse(bgColor)
            self:diffusealpha(dimAlpha)
        end
    },
    Def.ActorFrame {
        InitCommand = function(self)
            self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
        end,
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type == "InputEventType_FirstPress" then
                    if event.DeviceInput.button == "DeviceButton_left mouse button" then
                        self:playcommand("PressyMyMouseButton")
                    end
                end
            end)
            self:SetUpdateFunction(function(self) self:playcommand("HighlightyMyMouseHovering") end)
        end,

        Def.Quad {
            Name = "MainBG",
            InitCommand = function(self)
                self:zoomto(boxWidth, boxHeight)
                self:diffuse(boxColor)
                self:diffusealpha(boxAlpha)
            end
        },
        Def.Sprite {
            Texture = THEME:GetPathG("", "dialogTop"),
            Name = "HeaderBox",
            InitCommand = function(self)
                self:valign(0)
                self:y(-wtf - boxHeight/2)
                self:zoomto(boxWidth, INFORMATIONBOXHEIGHT)
            end
        },
        Def.Sprite {
            Texture = THEME:GetPathG("", "dialogExit"),
            Name = "ExitButton",
            InitCommand = function(self)
                self:halign(1):valign(1)
                self:zoomto(exitwidth, exitheight)
                self:xy(boxWidth/2 - sideMargin, boxHeight/2 - bottomMargin)
            end,
            PressyMyMouseButtonCommand = function(self)
                if isOver(self) then
                    -- true means cancelled, so the input is rejected
                    SCREENMAN:GetTopScreen():End(true)
                end
            end,
            HighlightyMyMouseHoveringCommand = function(self)
                if isOver(self) then
                    self:diffusealpha(hoverAlpha)
                else
                    self:diffusealpha(1)
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "INFORMATION",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(-boxWidth/2 + sideMargin, -boxHeight/2 + INFORMATIONupperGap)
                self:zoom(INFORMATIONtextsize)
                self:maxwidth(boxWidth / INFORMATIONtextsize)
                self:settext(translations["Information"])
            end
        },
        LoadFont("Common Normal") .. {
            Name = "Exit",
            InitCommand = function(self)
                self:zoom(exittextsize)
                self:maxwidth(60 / 1920 * SCREEN_WIDTH / exittextsize)
                self:xy(boxWidth/2 - sideMargin - exitwidth/2, boxHeight/2 - bottomMargin - exitheight/1.7)
                self:settext(translations["Exit"])
            end
        }
    }
}