--- Utilities again
-- like the 8th utility file
-- @module 99_util

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

-- return a letter to add based on input
-- nil return is invalid
function inputToCharacter(event)
    local btn = event.DeviceInput.button
    local char = event.char
    local shift = INPUTFILTER:IsShiftPressed()
    if btn == "DeviceButton_space" then
        return " "
    elseif char and char:match('[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%\'%"%>%<%?%/%~%|%w%[%]%{%}%`%\\]') then
        return char
    end
    return nil
end


-- convert a percentage (distance horizontally across the graph) to an index 
function getVectorIndexFromPercentage(x, vec, lowerlimit, upperlimit)
    local output = x
    if output < 0 then output = 0 end
    if output > 1 then output = 1 end
    if lowerlimit == nil then lowerlimit = 1 end
    if upperlimit == nil then upperlimit = #vec end

    local ind = notShit.round(output * #vec[1])
    if ind < 1 then ind = 1 end
    return ind
end

--[[
    Calc Debug Enums have really long names and saying things like CalcPatternMod_.... over and over is boring
    So instead what we can do is confuse the reader and programmer by messing with the enum string representation
    Turning "CalcPatternMod_JS" into just "JS" is both very clean and very confusing
    But it's for the better, I promise
]]
function shortenEnum(prefix, e)
    return e:gsub(prefix.."_", "")
end