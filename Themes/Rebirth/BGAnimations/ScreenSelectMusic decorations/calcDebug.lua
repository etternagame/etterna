-- this is a "secret" screen displayed by right clicking the song banner
-- you can leave the screen by pressing escape or clicking one of the buttons in the top of the screen

local lastHovered = nil
local lastusedsong = nil
local focused = false
local tt = Def.ActorFrame {
    Name = "CalcDebugFile",
    WheelSettledMessageCommand = function(self, params)
        lastHovered = params.hovered
        lastusedsong = params.song

        if focused then
            SCUFF.preview.resetmusic = false
            if lastusedsong ~= nil and SCUFF.preview.active then
                local top = SCREENMAN:GetTopScreen()
                if top.PlayCurrentSongSampleMusic then
                    -- reset music, force start, force full length
                    SCUFF.preview.resetmusic = true
                    SOUND:StopMusic()
                    top:PlayCurrentSongSampleMusic(true, true)
                end
            end
        end

        -- cascade visual update to everything
        self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
    end,
    ChangedStepsMessageCommand = function(self, params)
        lastusedsong = GAMESTATE:GetCurrentSong()
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = params.steps})
    end,
    OpenCalcDebugMessageCommand = function(self)
        if focused then return end
        focused = true
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
        self:playcommand("SlideOn")
    end,
    CloseCalcDebugMessageCommand = function(self)
        if not focused then return end
        focused = false
        self:playcommand("SlideOff")
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if not focused then return end
        focused = false
        self:playcommand("SlideOff", params)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if not focused then return end
        focused = false
        self:playcommand("SlideOff", params)
    end,
}

local ratios = {
    TopGap = 109 / 1080, -- height of the upper lip of the screen
    PreviewGraphHeight = 37 / 555,
}

local actuals = {
    TopGap = ratios.TopGap * SCREEN_HEIGHT,
    PreviewGraphHeight = ratios.PreviewGraphHeight * SCREEN_HEIGHT,
}

local showPosition = actuals.TopGap
local hidePosition = SCREEN_HEIGHT
local beginPosition = hidePosition
local animationSeconds = 0.1

local titleTextSize = 0.85
local authorTextSize = 0.75
local creditTextSize = 0.65
local packTextSize = 0.65
local textGap = 5
local edgeGap = 10

local notefieldZoom = 0.5
local previewGraphWidth = 64 * 4 * notefieldZoom * 1.2
local previewGraphHeight = actuals.PreviewGraphHeight * notefieldZoom * 1.5
local previewX = SCREEN_WIDTH - previewGraphWidth/2 - edgeGap
local previewY = previewGraphHeight / 2 + edgeGap

local msdBoxX = edgeGap
local msdBoxY = actuals.TopGap * 1.5
local msdBoxWidth = SCREEN_WIDTH * 0.075
local msdBoxSize = 25
local msdTextSize = 0.5


local t = Def.ActorFrame {
    Name = "Frame",
    InitCommand = function(self)
        self:y(beginPosition)
        self:diffusealpha(0)
    end,
    BeginCommand = function(self)
        local snm = SCREENMAN:GetTopScreen():GetName()
        local anm = self:GetName()
        -- this keeps track of whether or not the user is allowed to use the keyboard to change tabs
        CONTEXTMAN:RegisterToContextSet(snm, "CalcDebug", anm)

        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            -- if locked out, dont allow
            if not CONTEXTMAN:CheckContextSet(snm, "CalcDebug") then return end
            if event.type == "InputEventType_FirstPress" then
                if event.DeviceInput.button == "DeviceButton_space" then
                    -- this should propagate off to the right places
                    self:GetParent():playcommand("CloseCalcDebug")
                end
            end
        end)
    end,
    SlideOnCommand = function(self)
        self:finishtweening()
        self:diffusealpha(1)
        self:decelerate(animationSeconds)
        self:y(showPosition)
        MESSAGEMAN:Broadcast("HideWheel")
        MESSAGEMAN:Broadcast("HideRightFrame")
        local snm = SCREENMAN:GetTopScreen():GetName()
        CONTEXTMAN:ToggleContextSet(snm, "CalcDebug", true)
        if not SCUFF.preview.active then
            -- chart preview was not on
            SCUFF.preview.active = true
            if not SCUFF.preview.resetmusic and lastusedsong ~= nil then
                local top = SCREENMAN:GetTopScreen()
                if top.PlayCurrentSongSampleMusic then
                    -- reset music, force start, force full length
                    SCUFF.preview.resetmusic = true
                    SOUND:StopMusic()
                    top:PlayCurrentSongSampleMusic(true, true)
                end
            end
        end
    end,
    SlideOffCommand = function(self, params)
        self:finishtweening()
        self:decelerate(animationSeconds)
        self:y(hidePosition)
        self:diffusealpha(0)
        SCUFF.preview.active = false
        MESSAGEMAN:Broadcast("ShowWheel")
        if params == nil or params.tab == nil then
            MESSAGEMAN:Broadcast("GeneralTabSet", {tab = SCUFF.generaltabindex})
        end
        local snm = SCREENMAN:GetTopScreen():GetName()
        CONTEXTMAN:ToggleContextSet(snm, "CalcDebug", false)
    end,
}

t[#t+1] = Def.Quad {
    Name = "BG",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT - actuals.TopGap)
        self:diffusealpha(0.45)
        registerActorToColorConfigElement(self, "main", "PrimaryBackground")
    end,
}

t[#t+1] = Def.ActorFrame {
    Name = "SongInfoFrame",
    InitCommand = function(self)
        self:xy(edgeGap, edgeGap)
    end,
    BeginCommand = function(self)
        local title = self:GetChild("Title")
        local artist = self:GetChild("Artist")
        local credit = self:GetChild("Credit")
        local pack = self:GetChild("Pack")
        artist:y(title:GetY() + title:GetZoomedHeight() + textGap)
        credit:y(artist:GetY() + artist:GetZoomedHeight() + textGap)
        pack:y(credit:GetY() + credit:GetZoomedHeight() + textGap)
        self:GetParent():GetChild("MSDFrame"):y(pack:GetY() + pack:GetZoomedHeight() + textGap * 3)
    end,
    DisplayLanguageChangedMessageCommand = function(self)
        if not focused then return end
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
    end,

    LoadFont("Common Normal") .. {
        Name = "Title",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(titleTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / titleTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("%s", params.song:GetDisplayMainTitle())
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Artist",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(authorTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / authorTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("~ %s", params.song:GetDisplayArtist())
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Credit",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(creditTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / creditTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("Charter: %s", params.song:GetOrTryAtLeastToGetSimfileAuthor())
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Pack",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(packTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / packTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("Pack: %s", params.song:GetGroupName())
        end,
    },
    LoadActorWithParams("stepsdisplay", {ratios = {
        Width = 0.5,
        DiffFrameLeftGap = 0,
        DiffFrameRightGap = 0,
        LeftTextLeftGap = 0,
    }, actuals = {
        Width = SCREEN_WIDTH,
        DiffFrameLeftGap = 0 * SCREEN_WIDTH,
        DiffFrameRightGap = previewGraphWidth + 64,
        LeftTextLeftGap = 0 * SCREEN_WIDTH,
        DiffFrameUpperGap = edgeGap/2,
    }}) .. {

    }
}

local function msditem(i)
    local skillset = ms.SkillSets[i]
    return Def.ActorFrame {
        Name = "msd_"..skillset,
        InitCommand = function(self)
            if i <= 1 then return end
            local c = self:GetParent():GetChild("msd_"..ms.SkillSets[i-1])
            self:y(c:GetY() + c:GetChild("skillset"):GetZoomedHeight() + textGap)
        end,

        LoadFont("Common Normal") .. {
            Name = "skillset",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:zoom(msdTextSize)
                self:x(msdBoxWidth/2)
                self:maxwidth((msdBoxWidth/2) / msdTextSize)
                self:settextf("%s:", skillset)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
        },
        LoadFont("Common Normal") .. {
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoom(msdTextSize)
                self:x(msdBoxWidth/2 + edgeGap/2)
                self:maxwidth(msdBoxWidth / msdTextSize)
            end,
            SetCommand = function(self, params)
                if focused then
                    if params.steps == nil then return end
                    local meter = params.steps:GetMSD(getCurRateValue(), i)
                    self:diffuse(colorByMSD(meter))
                    self:settextf("%5.2f", meter)
                end
            end,
        }
    }
end
local function msditems()
    local t = Def.ActorFrame {
        Name = "MSDContainer",
    }
    for i=1, #ms.SkillSets do
        t[#t+1] = msditem(i)
    end
    return t
end
t[#t+1] = Def.ActorFrame {
    Name = "MSDFrame",
    InitCommand = function(self)
        self:x(edgeGap)
    end,
    msditems(),
}

t[#t+1] = Def.ActorFrame {
    Name = "MiniChartPreviewFrame",
    InitCommand = function(self)
        self:xy(previewX, previewY)
    end,
    SetCommand = function(self, params)
        if not focused then return end
        self:playcommand("LoadNoteData", {steps = params.steps})
    end,

    Def.NoteFieldPreview {
        Name = "NoteField",
        DrawDistanceBeforeTargetsPixels = (SCREEN_HEIGHT - actuals.TopGap) / notefieldZoom,
        DrawDistanceAfterTargetsPixels = 0, -- notes disappear at the receptor
    
        InitCommand = function(self)
            self:xy(0, previewGraphHeight)
            self:zoom(notefieldZoom)
            -- make mods work
            self:SetFollowPlayerOptions(true)
            self:SetUpdateFunction(function(self)
                ArrowEffects.Update()
            end)
            self:show_interval_bars(true)
        end,
        BeginCommand = function(self)
            -- we need to redo the draw order for the notefield and graph
            -- the notefield ends up being on top of everything in the actorframe otherwise
            self:draworder(1)
            self:GetParent():GetChild("ChordDensityGraphFile"):draworder(2)
            self:GetParent():SortByDrawOrder()
        end,
        LoadNoteDataCommand = function(self, params)
            local steps = params.steps
            if steps ~= nil then
                self:LoadNoteData(steps, true)
            else
                self:LoadDummyNoteData()
            end
            self:SetConstantMini(ReceptorSizeToMini(notefieldZoom))
            self:UpdateYReversePixels(0)
        end,
        OptionUpdatedMessageCommand = function(self, params)
            if params ~= nil then
                -- listen for the notedata modifying mods being toggled and apply their changes immediately
                local options = {
                    Mirror = true,
                    Turn = true,
                    ["Pattern Transform"] = true,
                    ["Hold Transform"] = true,
                    Remove = true,
                    Insert = true,
                    Mines = true,
                    ["Scroll Direction"] = true,
                }
                if options[params.name] ~= nil then
                    self:playcommand("LoadNoteData", {steps = GAMESTATE:GetCurrentSteps()})
                end
    
                if params.name == "Music Wheel Position" then
                    self:playcommand("SetPosition")
                end
            end
        end,
    },
    LoadActorWithParams("../chordDensityGraph.lua", {sizing = {
        Width = previewGraphWidth,
        Height = previewGraphHeight,
        NPSThickness = 2,
        TextSize = 0,
    }}) .. {
        InitCommand = function(self)
            self:xy(-previewGraphWidth/2, -previewGraphHeight/2)
        end,
        LoadNoteDataCommand = function(self, params)
            local steps = params.steps
            self:playcommand("LoadDensityGraph", {steps = steps, song = params.song})
        end,
    }
}



tt[#tt+1] = t
return tt