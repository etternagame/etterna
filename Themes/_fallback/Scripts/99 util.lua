--- Utilities again
-- like the 8th utility file
-- @module 99_util

-- update discord rpc for eval and gameplay
function updateDiscordStatus(inEvaluation)
    local profile = GetPlayerOrMachineProfile(PLAYER_1)
    local song = GAMESTATE:GetCurrentSong()
    local steps = GAMESTATE:GetCurrentSteps()
    -- Discord thingies
    local largeImageTooltip = string.format(
        "%s: %5.2f",
        profile:GetDisplayName(),
        profile:GetPlayerRating()
    )
    local mode = GAMESTATE:GetGameplayMode()
    local detail = string.format(
        "%s: %s [%s]",
        song:GetDisplayMainTitle(),
        string.gsub(getCurRateDisplayString(), "Music", ""),
        song:GetGroupName()
    )
    if mode == "GameplayMode_Replay" then
        detail = "Replaying: "..detail
    elseif mode == "GameplayMode_Practice" then
        detail = "Practicing: "..detail
    end
    -- truncated to 128 characters(discord hard limit)
    detail = #detail < 128 and detail or string.sub(detail, 1, 124) .. "..."
    local state = string.format(
        "MSD: %05.2f",
        steps:GetMSD(getCurRateValue(), 1)
    )
    local endTime = 0
    if inEvaluation then
        local score = SCOREMAN:GetMostRecentScore()
        if not score then
            score = SCOREMAN:GetTempReplayScore()
        end

        state = string.format(
            "%s - %05.2f%% %s",
            state,
            notShit.floor(score:GetWifeScore() * 10000) / 100,
            THEME:GetString("Grade", ToEnumShortString(score:GetWifeGrade()))
        )
    else
        endTime = os.time() + GetPlayableTime()
    end

    GAMESTATE:UpdateDiscordPresence(largeImageTooltip, detail, state, endTime)
end

-- update discord rpc for ingame menus
function updateDiscordStatusForMenus()
    local profile = GetPlayerOrMachineProfile(PLAYER_1)
    local detail = string.format(
        "%s: %5.2f",
        profile:GetDisplayName(),
        profile:GetPlayerRating()
    )
    GAMESTATE:UpdateDiscordMenu(detail)
end

-- writes to the install directory a nowplaying.txt
-- will be blank if not in gameplay
-- useful for stream overlays
function updateNowPlaying()
    local snm = SCREENMAN:GetTopScreen()
    local steps = GAMESTATE:GetCurrentSteps()
    local song = GAMESTATE:GetCurrentSong()
    local fout = " "
    if snm ~= nil and string.find(snm:GetName(), "Gameplay") ~= nil then
        local state = string.format(
            "MSD: %05.2f",
            steps:GetMSD(getCurRateValue(), 1)
        )
        fout = string.format(
            "Now playing %s by %s in %s %s",
            song:GetDisplayMainTitle(),
            song:GetDisplayArtist(),
            song:GetGroupName(),
            state
        )
    end

    File.Write("nowplaying.txt", fout)
end
