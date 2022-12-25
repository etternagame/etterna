local t = Def.ActorFrame {
    Name = "ProfileSelectFile",
    InitCommand = function(self)
        self:x(SCREEN_WIDTH)
    end,
    BeginCommand = function(self)
        self:smooth(0.5)
        self:x(0)
    end,
}

local ratios = {
    FrameLeftGap = 1303 / 1920, -- left of screen to left of item bg
    FrameUpperGap = 50 / 1080, -- from top of screen to top of first item bg
    Width = 555 / 1920,
    ItemHeight = 109 / 1080,

    ItemGlowVerticalSpan = 12 / 1080, -- measurement of visible portion of the glow doubled
    ItemGlowHorizontalSpan = 12 / 1920, -- same
    ItemGap = 57 / 1080, -- distance between the bg of items

    AvatarWidth = 109 / 1920,

    NameLeftGap = 8 / 1920, -- from edge of avatar to left edge of words
    NameUpperGap = 11 / 1080, -- top edge to top edge
    InfoLeftGap = 9 / 1920, -- from edge of avatar to left edge of lines below profile name
    Info1UpperGap = 39 / 1080, -- top edge to top edge
    Info2UpperGap = 62 / 1080, -- middle line
    Info3UpperGap = 85 / 1080, -- bottom line
    NameToRatingRequiredGap = 25 / 1920, -- we check this amount on player name lengths
    -- if the distance is not at least this much, truncate the name until it works

    RatingLeftGap = 393 / 1920, -- left edge to left edge of text
    -- this means the allowed space for this text is Width - RatingLeftGap
    RatingInfoUpperGap = 11 / 1080,
    OnlineUpperGap = 43 / 1080,
    OfflineUpperGap = 79 / 1080,
}

local actuals = {
    FrameLeftGap = ratios.FrameLeftGap * SCREEN_WIDTH,
    FrameUpperGap = ratios.FrameUpperGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    ItemHeight = ratios.ItemHeight * SCREEN_HEIGHT,
    ItemGlowVerticalSpan = ratios.ItemGlowVerticalSpan * SCREEN_HEIGHT,
    ItemGlowHorizontalSpan = ratios.ItemGlowHorizontalSpan * SCREEN_WIDTH,
    ItemGap = ratios.ItemGap * SCREEN_HEIGHT,
    AvatarWidth = ratios.AvatarWidth * SCREEN_WIDTH,
    NameLeftGap = ratios.NameLeftGap * SCREEN_WIDTH,
    NameUpperGap = ratios.NameUpperGap * SCREEN_HEIGHT,
    InfoLeftGap = ratios.InfoLeftGap * SCREEN_WIDTH,
    Info1UpperGap = ratios.Info1UpperGap * SCREEN_HEIGHT,
    Info2UpperGap = ratios.Info2UpperGap * SCREEN_HEIGHT,
    Info3UpperGap = ratios.Info3UpperGap * SCREEN_HEIGHT,
    NameToRatingRequiredGap = ratios.NameToRatingRequiredGap * SCREEN_WIDTH,
    RatingLeftGap = ratios.RatingLeftGap * SCREEN_WIDTH,
    RatingInfoUpperGap = ratios.RatingInfoUpperGap * SCREEN_HEIGHT,
    OnlineUpperGap = ratios.OnlineUpperGap * SCREEN_HEIGHT,
    OfflineUpperGap = ratios.OfflineUpperGap * SCREEN_HEIGHT,
}

local translations = {
    Plays = THEME:GetString("ScreenTitleMenu", "Plays"),
    ArrowsSmashed = THEME:GetString("ScreenTitleMenu", "ArrowsSmashed"),
    Playtime = THEME:GetString("ScreenTitleMenu", "Playtime"),
    PlayerRatings = THEME:GetString("ScreenTitleMenu", "PlayerRatings"),
    OnlineRating = THEME:GetString("ScreenTitleMenu", "OnlineRating"),
    OfflineRating = THEME:GetString("ScreenTitleMenu", "OfflineRating"),
    MakeFirstProfileQuestion = THEME:GetString("ScreenTitleMenu", "MakeFirstProfileQuestion"),
    MakeProfileError = THEME:GetString("ScreenTitleMenu", "MakeProfileError"),
    SelectProfile = THEME:GetString("ScreenTitleMenu", "SelectProfile"),
}

local profileIDs = PROFILEMAN:GetLocalProfileIDs()
local renameNewProfile = false
local focused = false

-- how many items to put on screen -- will fit for any screen height
local numItems = #profileIDs > 1 and math.floor(SCREEN_HEIGHT / (actuals.ItemHeight + actuals.ItemGap)) or 1

local nameTextSize = 0.75
local playcountTextSize = 0.5
local arrowsTextSize = 0.5
local playTimeTextSize = 0.5
local playerRatingsTextSize = 0.75
local onlineTextSize = 0.6
local offlineTextSize = 0.6

local textzoomFudge = 5

-- reset fadeout state
TITLE.triggeredFadeOut = false

-- if there are no profiles, make a new one
if #profileIDs == 0 then
    local new = PROFILEMAN:CreateDefaultProfile()
    profileIDs = PROFILEMAN:GetLocalProfileIDs()
    renameNewProfile = true
end

local primaryTextColor = COLORS:getTitleColor("PrimaryText")
local secondaryTextColor = COLORS:getTitleColor("SecondaryText")
local itemBGColor = COLORS:getTitleColor("ProfileBackground")

-- convenience to control the delete profile dialogue logic and input redir scope
local function deleteProfileDialogue(id)
    if id == nil then id = "(SOMETHING WENT WRONG)" end

    local redir = SCREENMAN:get_input_redirected(PLAYER_1)
    local function off()
        if redir then
            SCREENMAN:set_input_redirected(PLAYER_1, false)
        end
    end
    local function on()
        if redir then
            SCREENMAN:set_input_redirected(PLAYER_1, true)
        end
    end
    off()


    askForInputStringWithFunction(
        string.format("To delete this profile, navigate to Save/LocalProfiles\nDelete the folder '%s'\nRestart the game", id),
        0,
        false,
        function(answer) on() end,
        function(answer) return true, "Response invalid." end,
        function() on() end
    )
end

local function generateItems()
    -- add 1 to number of profiles so we can have a button to add profiles always as the last item
    local maxPage = math.ceil((#profileIDs) / numItems)
    local page = 1
    local selectionIndex = 1

    local function createProfileDialogue(listframe)
        -- uhh this shouldnt be hard...
        -- make profile, update id list, rename new profile
        local new = PROFILEMAN:CreateDefaultProfile()
        profileIDs = PROFILEMAN:GetLocalProfileIDs()
        maxPage = math.ceil((#profileIDs) / numItems)
        renameProfileDialogue(new, true)
    end

    -- select current option with keyboard or mouse double click
    local function selectCurrent()
        -- if holding control when this happens, trigger the delete profile dialogue
        -- (secret functionality but just for the keyboard user convenience)
        if INPUTFILTER:IsControlPressed() then
            deleteProfileDialogue(profileIDs[selectionIndex])
            return
        end

        PROFILEMAN:SetProfileIDToUse(profileIDs[selectionIndex])
        -- the sound should not play an additional time if we never allowed the profiles to be selected in the first place
        -- this function is used to force immediate selection of the first profile when only 1 profile is present
        if #profileIDs > 1 then
            SCREENMAN:GetTopScreen():PlaySelectSound()
        end
        SCREENMAN:GetTopScreen():SetSelectionIndex(0) -- instantly select Game Start
        TITLE:HandleFinalGameStart()
    end

    -- move page with keyboard or mouse
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

        MESSAGEMAN:Broadcast("MovedPage")
    end

    -- move current selection using keyboard
    local function move(n)
        local beforeindex = selectionIndex
        selectionIndex = clamp(selectionIndex + n, 1, #profileIDs)
        local lowerbound = numItems * (page-1) + 1
        local upperbound = numItems * page
        if lowerbound > selectionIndex or upperbound < selectionIndex then
            page = clamp(math.floor((selectionIndex-1) / numItems) + 1, 1, maxPage)
            MESSAGEMAN:Broadcast("MovedPage")
        else
            MESSAGEMAN:Broadcast("MovedIndex")
        end
        if beforeindex ~= selectionIndex then
            SCREENMAN:GetTopScreen():PlayChangeSound()
        end
    end

    -- change focus back to scroller options with keyboard
    local function backOut()
        TITLE:ChangeFocus()
    end


    local function generateItem(i)
        local index = i
        local profile = nil
        local id = nil

        return Def.ActorFrame {
            Name = "Choice_"..i,
            InitCommand = function(self)
                self:y((i-1) * (actuals.ItemHeight + actuals.ItemGap))
                self:diffusealpha(0)
            end,
            BeginCommand = function(self)
                self:playcommand("Set")
            end,
            ProfileRenamedMessageCommand = function(self)
                self:playcommand("Set")
            end,
            MovedPageMessageCommand = function(self)
                index = (page-1) * numItems + i
                self:playcommand("Set")
            end,
            SetCommand = function(self)
                if profileIDs[index] then
                    id = profileIDs[index]
                    profile = PROFILEMAN:GetLocalProfile(id)
                    self:finishtweening()
                    self:smooth(0.1)
                    self:diffusealpha(1)
                else
                    id = nil
                    profile = nil
                    self:finishtweening()
                    self:smooth(0.1)
                    self:diffusealpha(0)
                end
            end,

            UIElements.QuadButton(3) .. {
                Name = "BG",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoomto(actuals.Width, actuals.ItemHeight)
                    self:diffuse(itemBGColor)
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then
                        -- handle profile creation here
                        if index == #profileIDs + 1 then
                            createProfileDialogue(self:GetParent():GetParent())
                            return
                        end

                        -- because of button layering, pretend we are clicking through this button when it is invisible
                        -- (this stuff immediately below is the expected behavior)
                        if focused then
                           TITLE:ChangeFocus() 
                        end
                    else
                        if params.event == "DeviceButton_left mouse button" then
                            if INPUTFILTER:IsControlPressed() then
                                -- if holding ctrl, throw delete profile dialogue
                                -- ... well, I would do that if the game could delete profiles (thanks mina)
                                -- so instead I'll just give the necessary info
                                deleteProfileDialogue(id)
                            else
                                -- otherwise, move cursor or select current profile
                                if selectionIndex == index then
                                    selectCurrent()
                                else
                                    selectionIndex = index
                                    MESSAGEMAN:Broadcast("MovedIndex")
                                end
                            end
                        elseif params.event == "DeviceButton_right mouse button" then
                            -- right clicking allows profile name change
                            -- the Actor parameter is asking for the frame that holds the whole list
                            renameProfileDialogue(profile) 
                        end
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then
                        if index == #profileIDs + 1 then
                            -- show New Profile box (only on bottommost empty profile)
                            MESSAGEMAN:Broadcast("NewProfileToggle", {index = i})
                        end
                    end
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then
                        if index == #profileIDs + 1 then
                            -- hide New Profile box
                            MESSAGEMAN:Broadcast("NewProfileToggle", {index = nil})
                        end
                    end
                end,
                SetCommand = function(self)
                    if self:IsInvisible() then
                        if isOver(self) then
                            if index == #profileIDs + 1 then
                                -- show New Profile box
                                MESSAGEMAN:Broadcast("NewProfileToggle", {index = i})
                            end
                        end
                    else
                        if isOver(self) then
                            -- hide New Profile box
                            MESSAGEMAN:Broadcast("NewProfileToggle", {index = nil})
                        end
                    end
                end
            },
            Def.Sprite {
                Name = "Avatar",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                end,
                SetCommand = function(self)
                    self:Load(getAssetPathFromProfileID("avatar", id))
                    self:zoomto(actuals.AvatarWidth, actuals.ItemHeight)
                end
            },
            Def.ActorFrame {
                Name = "LeftText",
                InitCommand = function(self)
                    self:x(actuals.AvatarWidth)
                end,

                LoadFont("Common Normal") .. {
                    Name = "NameRank",
                    InitCommand = function(self)
                        self:x(actuals.NameLeftGap)
                        self:y(actuals.NameUpperGap)
                        self:valign(0):halign(0)
                        self:zoom(nameTextSize)
                        -- this maxwidth probably wont cause issues
                        -- .... but if it does.....
                        self:maxwidth((actuals.RatingLeftGap - actuals.AvatarWidth - actuals.NameLeftGap) / nameTextSize - textzoomFudge)
                        self:diffuse(primaryTextColor)
                        self:diffusealpha(1)
                    end,
                    SetCommand = function(self)
                        if profile then
                            local name = profile:GetDisplayName()
                            self:visible(false)
                            self:truncateToWidth(name, (actuals.RatingLeftGap - actuals.AvatarWidth - actuals.NameLeftGap) - textzoomFudge - 25)
                            self:visible(true)
                        end
                    end
                },
                LoadFont("Common Normal") .. {
                    Name = "Playcount",
                    InitCommand = function(self)
                        self:x(actuals.InfoLeftGap)
                        self:y(actuals.Info1UpperGap)
                        self:valign(0):halign(0)
                        self:zoom(playcountTextSize)
                        self:maxwidth((actuals.RatingLeftGap - actuals.AvatarWidth - actuals.InfoLeftGap) / playcountTextSize - textzoomFudge)
                        self:diffuse(secondaryTextColor)
                        self:diffusealpha(1)
                    end,
                    SetCommand = function(self)
                        if profile then
                            local scores = profile:GetTotalNumSongsPlayed()
                            self:settextf("%d %s", scores, translations["Plays"])
                        end
                    end
                },
                LoadFont("Common Normal") .. {
                    Name = "Arrows",
                    InitCommand = function(self)
                        self:x(actuals.InfoLeftGap)
                        self:y(actuals.Info2UpperGap)
                        self:valign(0):halign(0)
                        self:zoom(arrowsTextSize)
                        self:maxwidth((actuals.RatingLeftGap - actuals.AvatarWidth - actuals.InfoLeftGap) / arrowsTextSize - textzoomFudge)
                        self:diffuse(secondaryTextColor)
                        self:diffusealpha(1)
                    end,
                    SetCommand = function(self)
                        if profile then
                            local taps = profile:GetTotalTapsAndHolds()
                            self:settextf("%d %s", taps, translations["ArrowsSmashed"])
                        end
                    end
                },
                LoadFont("Common Normal") .. {
                    Name = "Playtime",
                    InitCommand = function(self)
                        self:x(actuals.InfoLeftGap)
                        self:y(actuals.Info3UpperGap)
                        self:valign(0):halign(0)
                        self:zoom(playTimeTextSize)
                        self:maxwidth((actuals.RatingLeftGap - actuals.AvatarWidth - actuals.InfoLeftGap) / playTimeTextSize - textzoomFudge)
                        self:diffuse(secondaryTextColor)
                        self:diffusealpha(1)
                    end,
                    SetCommand = function(self)
                        if profile then
                            local secs = profile:GetTotalSessionSeconds()
                            self:settextf("%s %s", SecondsToHHMMSS(secs), translations["Playtime"])
                        end
                    end
                }
            },
            Def.ActorFrame {
                Name = "RightText",
                InitCommand = function(self)
                    self:x(actuals.RatingLeftGap)
                end,

                LoadFont("Common Normal") .. {
                    Name = "PlayerRatings",
                    InitCommand = function(self)
                        self:y(actuals.RatingInfoUpperGap)
                        self:valign(0):halign(0)
                        self:zoom(playerRatingsTextSize)
                        self:maxwidth((actuals.Width - actuals.RatingLeftGap) / playerRatingsTextSize - textzoomFudge)
                        self:settextf("%s:", translations["PlayerRatings"])
                        self:diffuse(primaryTextColor)
                        self:diffusealpha(1)
                    end
                },
                --[[-- online ratings for individual profiles have no direct api
                LoadFont("Common Normal") .. {
                    Name = "Online",
                    InitCommand = function(self)
                        self:y(actuals.OnlineUpperGap)
                        self:valign(0):halign(0)
                        self:zoom(onlineTextSize)
                        self:maxwidth((actuals.Width - actuals.RatingLeftGap) / onlineTextSize - textzoomFudge)
                        self:diffuse(primaryTextColor)
                        self:diffusealpha(1)
                    end,
                    SetCommand = function(self)
                        self:settextf("%s - 00.00", translations["OnlineRating"])
                    end
                },]]
                LoadFont("Common Normal") .. {
                    Name = "Offline",
                    InitCommand = function(self)
                        self:y(actuals.OfflineUpperGap)
                        self:valign(0):halign(0)
                        self:zoom(offlineTextSize)
                        self:maxwidth((actuals.Width - actuals.RatingLeftGap) / offlineTextSize - textzoomFudge)
                        self:diffuse(primaryTextColor)
                        self:diffusealpha(1)
                    end,
                    SetCommand = function(self)
                        if profile then
                            local rating = profile:GetPlayerRating()
                            self:settextf("%s - %5.2f", translations["OfflineRating"], rating)
                        end
                    end
                }
            }
        }
    end

    local t = Def.ActorFrame {
        Name = "ItemList",
        InitCommand = function(self)
            self:xy(actuals.FrameLeftGap, actuals.FrameUpperGap)
        end,
        BeginCommand = function(self)
            -- make sure the focus is set on the scroller options
            -- false means that we are focused on the profile choices
            TITLE:SetFocus(true)
            SCREENMAN:set_input_redirected(PLAYER_1, false)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type == "InputEventType_FirstPress" then
                    if INPUTFILTER:IsControlPressed() and event.DeviceInput.button == "DeviceButton_n" then
                        createProfileDialogue(self)
                        return
                    end
                    if focused then
                        if event.button == "MenuUp" or event.button == "Up" 
                        or event.button == "MenuLeft" or event.button == "Left" then
                            move(-1)
                        elseif event.button == "MenuDown" or event.button == "Down" 
                        or event.button == "MenuRight" or event.button == "Right" then
                            move(1)
                        elseif event.button == "Start" then
                            selectCurrent()
                        elseif event.button == "Back" then
                            backOut()
                        end
                    end
                end
            end)
        end,
        FirstUpdateCommand = function(self)
            -- waiting for the crashdump upload dialog to go away
            if renameNewProfile then
                if PREFSMAN:GetPreference("ShowMinidumpUploadDialogue") then
                    self:sleep(0.2):queuecommand("Keepwastingtimestart")
                else
                    self:playcommand("Finishwastingtime")
                end
            end
        end,
        KeepwastingtimestartCommand = function(self)
            -- looping to wait for the dialog to go away
            if renameNewProfile then
                if PREFSMAN:GetPreference("ShowMinidumpUploadDialogue") then
                    self:sleep(0.2):queuecommand("Keepwastingtimestart")
                else
                    self:playcommand("Finishwastingtime")
                end
            end
        end,
        FinishwastingtimeCommand = function(self)
            if renameNewProfile then
                local profile = PROFILEMAN:GetLocalProfile(profileIDs[1])
                local redir = SCREENMAN:get_input_redirected(PLAYER_1)
                local function off()
                    if redir then
                        SCREENMAN:set_input_redirected(PLAYER_1, false)
                    end
                end
                local function on()
                    if redir then
                        SCREENMAN:set_input_redirected(PLAYER_1, true)
                    end
                end
                off()
                
                local function f(answer)
                    profile:RenameProfile(answer)
                    self:playcommand("ProfileRenamed")
                    on()
                end
                local question = translations["MakeFirstProfileQuestion"]
                askForInputStringWithFunction(
                    question,
                    64,
                    false,
                    f,
                    function(answer)
                        local result = answer ~= nil and answer:gsub("^%s*(.-)%s*$", "%1") ~= "" and not answer:match("::") and answer:gsub("^%s*(.-)%s*$", "%1"):sub(-1) ~= ":"
                        if not result then
                            SCREENMAN:GetTopScreen():GetChild("Question"):settext(question .. "\n" .. translations["MakeProfileError"])
                        end
                        return result, "Response invalid."
                    end,
                    function()
                        -- do nothing
                        -- the profile name is Default Profile
                        -- cringe name tbh
                        on()
                    end
                )
            end
        end,
        ToggledTitleFocusMessageCommand = function(self, params)
            focused = not params.scrollerFocused
            -- focused means we must pay attention to the profiles instead of the left scroller
            if focused then
                if #profileIDs == 1 then
                    -- there is only 1 choice, no need to care about picking a profile
                    -- skip forward
                    TITLE:HandleFinalGameStart()
                else
                    -- consider our options...
                    -- (locking input here because of a race condition that counts our enter button press twice)
                    SCREENMAN:GetTopScreen():lockinput(0.05)
                    SCREENMAN:set_input_redirected(PLAYER_1, true)
                end
            else
                SCREENMAN:set_input_redirected(PLAYER_1, false)
            end
            self:GetChild("FocusBG"):playcommand("FocusChange")
            self:GetChild("InfoText"):playcommand("FocusChange")
        end,

        UIElements.QuadButton(1, 1) .. {
            Name = "FocusBG",
            InitCommand = function(self)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0)
            end,
            BeginCommand = function(self)
                -- offset position to fill whole screen
                self:xy(-self:GetParent():GetX(), -self:GetParent():GetY())
                self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
                self:halign(0):valign(0)
            end,
            FocusChangeCommand = function(self)
                if focused then
                    self:hurrytweening(0.5)
                    self:smooth(0.4)
                    self:diffusealpha(0.75)
                    self:z(2)
                else
                    self:hurrytweening(0.5)
                    self:smooth(0.4)
                    self:diffusealpha(0)
                    self:z(1)
                end
            end,
            MouseDownCommand = function(self, params)
                -- only allow clicking when focused on profile select
                -- though this button covers the whole screen, the button system should
                --  properly handle the behavior of overlapping the profile buttons and this
                -- Button Layers --
                --      -- (Focus On)
                -- [Scroller Buttons] [THIS] [Profile Buttons]
                --      -- (Focus Off)
                -- [THIS] [Scroller Buttons] [Profile Buttons]
                -- Because depth alone wont help, we have to employ Z coordinates in focus changes to assist.
                -- Also, the invisible buttons still take priority because we don't account for it in the implementation
                -- So invisible buttons do exactly the same as this:
                -- (why is this comment so long)
                if focused then
                    TITLE:ChangeFocus()
                end
            end
        },
        LoadFont("Common Large") .. {
            Name = "InfoText",
            InitCommand = function(self)
                self:halign(1)
                self:xy(actuals.Width, -actuals.FrameUpperGap/2)
                self:zoom(0.5)
                self:settext(translations["SelectProfile"])
                self:diffusealpha(0)
            end,
            FocusChangeCommand = function(self)
                if focused then
                    self:hurrytweening(0.5)
                    self:smooth(0.4)
                    self:diffusealpha(1)
                else
                    self:hurrytweening(0.5)
                    self:smooth(0.4)
                    self:diffusealpha(0)
                end
            end,
        },
        Def.Quad {
            Name = "MouseWheelRegion",
            InitCommand = function(self)
                self:diffusealpha(0)
            end,
            BeginCommand = function(self)
                -- offset position to fill whole screen
                self:xy(-self:GetParent():GetX(), -self:GetParent():GetY())
                self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
                self:halign(0):valign(0)
            end,
            MouseScrollMessageCommand = function(self, params)
                if params.direction == "Up" then
                    movePage(-1)
                else
                    movePage(1)
                end
            end
        },


        Def.Sprite {
            Name = "Cursor",
            Texture = THEME:GetPathG("", "profileselectorGlow"),
            InitCommand = function(self)
                self:xy(-actuals.ItemGlowHorizontalSpan / 2, -actuals.ItemGlowVerticalSpan / 2)
                self:halign(0):valign(0)
                self:zoomto(actuals.Width + actuals.ItemGlowHorizontalSpan, actuals.ItemHeight + actuals.ItemGlowVerticalSpan)
            end,
            MovedPageMessageCommand = function(self)
                local lowerbound = numItems * (page-1) + 1
                local upperbound = math.min(numItems * page, #profileIDs)
                if lowerbound > selectionIndex or upperbound < selectionIndex then
                    local cursorpos = (selectionIndex-1) % numItems
                    local newpos = cursorpos + (page-1) * numItems + 1
                    if profileIDs[newpos] == nil then
                        -- dont let the cursor get into an impossible position
                        selectionIndex = clamp(newpos, lowerbound, upperbound)
                    else
                        selectionIndex = newpos
                    end
                end
                self:playcommand("MovedIndex")
            end,
            MovedIndexMessageCommand = function(self)
                local cursorindex = (selectionIndex-1) % numItems
                self:finishtweening()
                self:linear(0.05)
                self:y(cursorindex * (actuals.ItemHeight + actuals.ItemGap) - actuals.ItemGlowVerticalSpan / 2)
            end
        },

        Def.Sprite {
            Name = "NewProfileButton",
            Texture = THEME:GetPathG("", "newProfile"),
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.Width, actuals.ItemHeight)
            end,
            NewProfileToggleMessageCommand = function(self, params)
                -- provide an index to place the button in a spot
                -- otherwise, dont and it will disappear
                if params.index then
                    self:y((params.index-1) * (actuals.ItemHeight + actuals.ItemGap))
                    self:finishtweening()
                    self:x(0)
                    self:smooth(0.075)
                    self:diffusealpha(1)
                else
                    self:x(1000)
                    self:diffusealpha(0)
                end
            end
        }
    }

    for i = 1, numItems do
        t[#t+1] = generateItem(i)
    end
    return t
end

t[#t+1] = generateItems()

return t