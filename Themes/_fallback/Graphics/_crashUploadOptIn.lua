-- dont unncessarily load
if PREFSMAN:GetPreference("EnableMinidumpUpload") then return Def.ActorFrame {} end
if not PREFSMAN:GetPreference("ShowMinidumpUploadDialogue") then return Def.ActorFrame {} end

local enabled = true
local textsize = 0.7
local boxw = SCREEN_WIDTH/1.5
local boxh = SCREEN_HEIGHT/3.3
local bufferspace = 5
local redirb4 = false

local translations = {
    Prompt = THEME:GetString("CrashpadDialogue", "Prompt"),
    OptedIn = THEME:GetString("CrashpadDialogue", "OptedIn"),
    OptedOut = THEME:GetString("CrashpadDialogue", "OptedOut"),
    Accept = THEME:GetString("CrashpadDialogue", "Accept"),
}

local t = Def.ActorFrame {
    Name = "CrashUploadOptInDialogue",
    InitCommand = function(self)
        self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
    end,
    BeginCommand = function(self)
        -- input redirection management FAILS IF YOU RESTART/EXIT THE SCREEN UNNATURALLY (like with F3)
        -- BEWARE THEMERS
        -- if you get input locked, just ctrl+operator out of it
        redirb4 = SCREENMAN:get_input_redirected(PLAYER_1)
        SCREENMAN:set_input_redirected(PLAYER_1, true)

        -- have to forcibly set redir true after a little bit of time
        -- the chat overlay turns it off randomly
        self:sleep(0.5):queuecommand("InputRedirSet")

        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            if not enabled then return end
            if event.type ~= "InputEventType_FirstPress" then return true end

            -- mega lazy triple copy paste
            if event.DeviceInput.button == "DeviceButton_y" then
                -- "press Y"
                self:visible(false)
                enabled = false
                ms.ok(translations["OptedIn"])
                SCREENMAN:set_input_redirected(PLAYER_1, redirb4)
                PREFSMAN:SetPreference("EnableMinidumpUpload", true)
                PREFSMAN:SetPreference("ShowMinidumpUploadDialogue", false)
            elseif event.DeviceInput.button == "DeviceButton_escape" or event.button == "Back" then
                -- "press Escape"
                self:visible(false)
                enabled = false
                ms.ok(translations["OptedOut"])
                SCREENMAN:set_input_redirected(PLAYER_1, redirb4)
                PREFSMAN:SetPreference("ShowMinidumpUploadDialogue", false)
            elseif event.DeviceInput.button == "DeviceButton_left mouse button" then
                if isOver(self:GetChild("Button")) then
                    -- "click button"
                    self:visible(false)
                    enabled = false
                    ms.ok(translations["OptedIn"])
                    SCREENMAN:set_input_redirected(PLAYER_1, redirb4)
                    PREFSMAN:SetPreference("EnableMinidumpUpload", true)
                    PREFSMAN:SetPreference("ShowMinidumpUploadDialogue", false)
                elseif not isOver(self:GetChild("DialogueBox")) then
                    -- "click away"
                    self:visible(false)
                    enabled = false
                    ms.ok(translations["OptedOut"])
                    SCREENMAN:set_input_redirected(PLAYER_1, redirb4)
                    PREFSMAN:SetPreference("ShowMinidumpUploadDialogue", false)
                end
            end

            -- eat input, dont let it pass through to anything else
            return true
        end)
    end,
    InputRedirSetCommand = function(self)
        SCREENMAN:set_input_redirected(PLAYER_1, true)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
            self:diffuse(color("0,0,0"))
            self:diffusealpha(0.7)
        end,
    },
    Def.Quad {
        Name = "DialogueBox",
        InitCommand = function(self)
            self:zoomto(boxw, boxh)
            self:diffuse(color("0,0,0"))
            self:diffusealpha(1)
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Text",
        InitCommand = function(self)
            self:zoom(textsize)
            self:y(-boxh/2 + bufferspace)
            self:valign(0)
            self:maxwidth(boxw * 0.95 / textsize)
            self:maxheight((boxh - boxh/5) * 0.95 / textsize)
            self:settext(translations["Prompt"])
        end,
    },
    Def.Quad {
        Name = "Button",
        InitCommand = function(self)
            self:valign(1)
            self:y(boxh/2 - bufferspace)
            self:zoomto(boxw/5, boxh/5)
            self:diffuse(color("0.5,0.5,0.5"))
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "ButtonTxt",
        InitCommand = function(self)
            self:y(boxh/2 - (boxh/10) - bufferspace)
            self:zoom(textsize)
            self:maxwidth(boxw/5 / textsize)
            self:settext(translations["Accept"])
        end,
    },
}

return t