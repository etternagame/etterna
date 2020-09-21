local t = Def.ActorFrame {Name = "ProfileSelectFile"}

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

local profileIDs = PROFILEMAN:GetLocalProfileIDs()
local renameNewProfile = false
local focused = false

-- how many items to put on screen -- will fit for any screen height
local numItems = #profileIDs > 1 and math.floor(SCREEN_HEIGHT / (actuals.ItemHeight + actuals.ItemGap)) or 1
local itemBGColor = color("0,0,0,1")

local nameTextSize = 0.75
local playcountTextSize = 0.5
local arrowsTextSize = 0.5
local playTimeTextSize = 0.5
local playerRatingsTextSize = 0.75
local onlineTextSize = 0.6
local offlineTextSize = 0.6

local textzoomFudge = 5

-- if there are no profiles, make a new one
if #profileIDs == 0 then
    local new = PROFILEMAN:CreateDefaultProfile()
    profileIDs = PROFILEMAN:GetLocalProfileIDs()
    renameNewProfile = true
end

local function generateItems()
    local function generateItem(i)
        local index = i
        local profile = nil
        local id = nil

        return Def.ActorFrame {
            Name = "Choice_"..i,
            InitCommand = function(self)
                self:y((i-1) * (actuals.ItemHeight + actuals.ItemGap))
            end,
            BeginCommand = function(self)
                self:playcommand("Set")
            end,
            UpdateProfilesCommand = function(self)
                self:playcommand("Set")
            end,
            SetCommand = function(self)
                if profileIDs[index] then
                    id = profileIDs[index]
                    profile = PROFILEMAN:GetLocalProfile(id)
                    self:visible(true)
                else
                    self:visible(false)
                end
            end,

            UIElements.QuadButton(1) .. {
                Name = "BG",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoomto(actuals.Width, actuals.ItemHeight)
                    self:diffuse(itemBGColor)
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
                        self:maxwidth((actuals.RatingLeftGap - actuals.AvatarWidth - actuals.NameLeftGap) / nameTextSize - textzoomFudge)
                    end,
                    SetCommand = function(self)
                        local name = profile:GetDisplayName()
                        self:settextf("%s (#9999)", name)
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
                    end,
                    SetCommand = function(self)
                        -- wrong
                        local scores = SCOREMAN:GetTotalNumberOfScores()
                        self:settextf("%d plays", scores)
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
                    end,
                    SetCommand = function(self)
                        local taps = profile:GetTotalTapsAndHolds()
                        self:settextf("%d arrows smashed", taps)
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
                    end,
                    SetCommand = function(self)
                        local secs = profile:GetTotalSessionSeconds()
                        self:settextf("%s playtime", SecondsToHHMMSS(secs))
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
                        self:settext("Player Ratings:")
                    end
                },
                LoadFont("Common Normal") .. {
                    Name = "Online",
                    InitCommand = function(self)
                        self:y(actuals.OnlineUpperGap)
                        self:valign(0):halign(0)
                        self:zoom(onlineTextSize)
                        self:maxwidth((actuals.Width - actuals.RatingLeftGap) / onlineTextSize - textzoomFudge)
                    end,
                    SetCommand = function(self)
                        self:settext("Online - 00.00")
                    end
                },
                LoadFont("Common Normal") .. {
                    Name = "Offline",
                    InitCommand = function(self)
                        self:y(actuals.OfflineUpperGap)
                        self:valign(0):halign(0)
                        self:zoom(offlineTextSize)
                        self:maxwidth((actuals.Width - actuals.RatingLeftGap) / offlineTextSize - textzoomFudge)
                    end,
                    SetCommand = function(self)
                        local rating = profile:GetPlayerRating()
                        self:settextf("Offline - %5.2f", rating)
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
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if focused then
                    if event.type == "InputEventType_FirstPress" then
                        if event.button == "MenuUp" or event.button == "Up" 
                        or event.button == "MenuLeft" or event.button == "Left" then
                            move(-1)
                        elseif event.button == "MenuDown" or event.button == "Down" 
                        or event.button == "MenuRight" or event.button == "Right" then
                            move(1)
                        elseif event.button == "Start" then
                            selectCurrent()
                        elseif event.button == "Cancel" then
                            backOut()
                        end
                    end
                end
            end)
        end,
        FirstUpdateCommand = function(self)
            if renameNewProfile then
                local profile = PROFILEMAN:GetLocalProfile(profileIDs[1])
                local function f(answer)
                    profile:RenameProfile(answer)
                    self:playcommand("UpdateProfiles")
                end
                local question = "No Profiles detected! A new one was made for you.\nPlease enter a new profile name."
                askForInputStringWithFunction(
                    question,
                    64,
                    false,
                    f,
                    function(answer)
                        local result = answer ~= nil and answer:gsub("^%s*(.-)%s*$", "%1") ~= "" and not answer:match("::") and answer:gsub("^%s*(.-)%s*$", "%1"):sub(-1) ~= ":"
                        if not result then
                            SCREENMAN:GetTopScreen():GetChild("Question"):settext(question .. "\nDo not leave this space blank. Do not use ':'")
                        end
                        return result, "Response invalid."
                    end
                )
            end
        end,
        ToggledTitleFocusMessageCommand = function(self, params)
            focused = params.scrollerFocused
            if not focused then
                if #profileIDs == 1 then
                    -- there is only 1 choice, no need to care about picking a profile
                    -- skip forward
                    TITLE:HandleFinalGameStart()
                else
                    -- consider our options...

                end
            end
        end,


        Def.Sprite {
            Name = "Cursor",
            Texture = THEME:GetPathG("", "profileselectorGlow"),
            InitCommand = function(self)
                self:xy(-actuals.ItemGlowHorizontalSpan / 2, -actuals.ItemGlowVerticalSpan / 2)
                self:halign(0):valign(0)
                self:zoomto(actuals.Width + actuals.ItemGlowHorizontalSpan, actuals.ItemHeight + actuals.ItemGlowVerticalSpan)
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