-- additional utility functions that could be used in places but not overshadowing other util functions


-- return the width of the widest child
-- works recursively: considers all children in the tree
function getLargestChildWidth(actorFrame)
    local largest = 0
    if actorFrame == nil then
        return largest
    end

    actorFrame:RunCommandsRecursively(
        function(self)
            local w = self:GetZoomedWidth()
            if w > largest then
                largest = w
            end
        end
    )

    return largest
end

-- recursively print the names of all children of this actorframe
-- kind of does it in a tree-like fashion
function nameAllChildren(actorFrame)
    local s = actorFrame:GetName()
    local function finddepth(self)
        local i = 0
        local p = self
        while p ~= nil do
            p = p:GetParent()
            i = i+1
        end
        return i
    end
    local function spaces(i)
        local o = ""
        for s = 1, i do
            o = o .. " "
        end
        return o
    end
    actorFrame:RunCommandsRecursively(
        function(self)
            local buffer = finddepth(self)
            local space = spaces(buffer)

            if self:GetName() == nil or self:GetName() == "" then
                s = s .. "\n" .. space .. "[no name]"
            else
                s = s .. "\n" .. space .. self:GetName()
            end
        end
    )
    ms.ok(s)
end

-- find the height and width while maintaining aspect ratio
function getHWKeepAspectRatio(h, w, ratio)
    local he = h / math.sqrt(ratio * ratio + 1)
    local we = w / math.sqrt(1 / (ratio * ratio) + 1)

    return he, we
end

-- string split, return a list given a string and a separator between items
function strsplit(given, separator)
    if separator == nil then
        separator = "%s" -- whitespace
    end
    local t = {}
    for str in string.gmatch(given, "([^"..separator.."]+)") do
        table.insert(t, str)
    end
    return t
end

-- string trim, remove all whitespace from the edges of a string
function strtrim(str)
    if str == '' then
        return str
    else
        local startPos = 1
        local endPos   = #str

        while (startPos < endPos and str:byte(startPos) <= 32) do
            startPos = startPos + 1
        end

        if startPos >= endPos then
            return ''
        else
            while (endPos > 0 and str:byte(endPos) <= 32) do
                endPos = endPos - 1
            end

            return str:sub(startPos, endPos)
        end
    end
end

function strCapitalize(str)
    return str:gsub("(%l)(%w*)", function(a,b) return string.upper(a)..b end)
end

-- convert a shortened date string into month day year
function expandDateString(given)
    if given == nil then
        return MonthToLocalizedString(1), "1st", "0001"
    end
    local arglist = strsplit(given)

    if #arglist == 2 or #arglist == 1 then
        arglist = strsplit(arglist[1], "-")
    else
        return MonthToLocalizedString(1), "1st", "0001"
    end

    local month = MonthToLocalizedString(tonumber(arglist[2]) - 1)
    local day = tonumber(arglist[3])
    local year = arglist[1]

    if day % 100 >= 11 and day % 100 <= 13 then
        day = tostring(day) .. "th"
    else
        if day % 10 == 1 then
            day = tostring(day) .. "st"
        elseif day % 10 == 2 then
            day = tostring(day) .. "nd"
        elseif day % 10 == 3 then
            day = tostring(day) .. "rd"
        else
            day = tostring(day) .. "th"
        end
    end
    return month, day, year
end

-- convert the "YYYY-MM-DD HH:MM:SS" format to "YYYY-MM-DD" only
function extractDateFromDateString(given)
    if given == nil then
        return "0001-01-01"
    end
    local arglist = strsplit(given)

    -- 2 entries means it is the correct format
    if #arglist == 2 then
        return arglist[1]
    elseif #arglist == 1 then
        -- 1 entry means it is just the date already
        return given
    else
        -- ????
        return given
    end
end

-- convert a long number string into a shorter one
function shortenNumber(num)
    local suffixes = {
        "", -- less than 1k
        "k",
        "m",
        "b",
        "t",
        "q", -- nothing should be any higher than m but...
    }

    local numNUM = tonumber(num)
    local ind = 1
    while math.abs(numNUM) >= 1000 and ind < #suffixes do
        ind = ind + 1
        numNUM = numNUM / 1000
    end

    if ind == 1 then return tostring(num) end
    return string.format("%.3f%s", math.floor(numNUM * 10^3) / 10^3, suffixes[ind])
end

-- convenience to reduce copy paste
function shortenIfOver1Mil(num)
    if num >= 1000000 then
        return shortenNumber(num)
    else
        return tostring(num)
    end
end

-- a kind of alias of "not IsVisible" to include the diffuse
-- also recursive
function Actor.IsInvisible(self)
    local a = self:GetFakeParent()
    if a == nil then
        a = self:GetParent()
        if a == nil then
            -- no parents, no fake parents
            return self:GetDiffuseAlpha() == 0 or not self:IsVisible()
        else
            -- no parent, has fake parent
            return self:GetDiffuseAlpha() == 0 or a:IsInvisible()
        end
    else
        -- has parent
        return self:GetDiffuseAlpha() == 0 or a:IsInvisible()
    end
end

-- funny alias to just remove the Music at the end of the string
function getRateDisplayString2(x)
    if x == "1x" then
        x = "1.0x"
    elseif x == "2x" then
        x = "2.0x"
    end
    return x
end

-- alias for wifesundries ChangeMusicRate which is really bad
function changeMusicRate(direction)
    local now = getCurRateValue()
    -- the classic clamps are 0.7 and 3
    -- the game wont allow 0 and wont allow over 3
    local next = clamp(now + direction, 0.05, 3)

    GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(next)
    GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(next)
    GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(next)

    MESSAGEMAN:Broadcast("CurrentRateChanged")
end

function askForInputStringWithFunction(question, maxInputLength, obfuscate, onOK, validateFunc, onCancel)
    SCREENMAN:AddNewScreenToTop("ScreenTextEntry")
    local settings = {
        Question = question,
        MaxInputLength = maxInputLength,
        Password = obfuscate,
        OnOK = function(answer)
            onOK(answer)
        end,
        Validate = function(answer)
            return validateFunc(answer)
        end,
        OnCancel = function()
            onCancel()
        end,
    }
    SCREENMAN:GetTopScreen():Load(settings)
end

-- set the given text but truncate it if a width is reached
function BitmapText.truncateToWidth(self, text, maxwidth)
    for i = 1, #text do
        self:settext(text:sub(1, i).."...")
        if self:GetZoomedWidth() > maxwidth then
            break
        end
        if i == #text then
            self:settext(text)
        end
    end
end

function tableconcat(...)
    local arg = {...}
    local t = {}
    for i = 1, #arg do
        for i, v in ipairs(arg[i]) do
            t[#t + 1] = v
        end
    end
    return t
end

function tablesplit(t, x)
    local t1, t2 = {}, {}
    local idx = nil
    -- Used to simulate a for break
    local aux = function()
        for i, v in ipairs(t) do
            if v == x then
                idx = t[i + 1] and i + 1 or nil
                return
            end
            t1[i] = v
        end
    end
    aux()
    while idx ~= nil do
        t2[#t2 + 1] = t[idx]
        idx = t[idx + 1] and idx + 1 or nil
    end
    return t1, t2
end

-- find the key of a value in a table
function findKeyOf(t, x)
    for k, v in pairs(t) do
        if v == x then
            return k
        end
    end
    return nil
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

-- alias for getting "current" (preferred) PlayerOptions
function getPlayerOptions()
    return GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
end
-- alias for getting "current" (preferred) SongOptions
function getSongOptions()
    return GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred")
end

-- safely get a very deep child of an ActorFrame
-- just in case something in between doesnt exist
-- the names should be in the order you would chain the GetChild usages
-- GetChild("Top"):GetChild("childchild"):GetChild("greatgrandchild") ...
-- if something doesnt exist, return nil
function ActorFrame.safeGetChild(self, ...)
    local names = {...}
    local final = self
    for i, name in ipairs(names) do
        if final ~= nil and final.GetChild ~= nil then
            final = final:GetChild(name)
        else
            return final
        end
    end
    return final
end

-- convert a receptor size to a mini because this math is really annoying to memorize
function ReceptorSizeToMini(percent)
    return 2 - percent / 0.5
end

-- read the function name
function updateDiscordStatusForGameplay()
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
    local endTime = os.time() + GetPlayableTime()
    GAMESTATE:UpdateDiscordPresence(largeImageTooltip, detail, state, endTime)
end

-- writes to the install directory a nowplaying.txt
-- will be blank if not in gameplay
-- useful for stream overlays
function updateNowPlaying()
    local snm = SCREENMAN:GetTopScreen()
    local steps = GAMESTATE:GetCurrentSteps()
    local song = GAMESTATE:GetCurrentSong()
    local fout = ""
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

-- convenience to control the rename profile dialogue logic and input redir scope
function renameProfileDialogue(profile)
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
        MESSAGEMAN:Broadcast("ProfileRenamed")
        on()
    end
    local question = "RENAME PROFILE\nPlease enter a new profile name."
    askForInputStringWithFunction(
        question,
        255,
        false,
        f,
        function(answer)
            local result = answer ~= nil and answer:gsub("^%s*(.-)%s*$", "%1") ~= "" and not answer:match("::") and answer:gsub("^%s*(.-)%s*$", "%1"):sub(-1) ~= ":"
            if not result then
                SCREENMAN:GetTopScreen():GetChild("Question"):settext(question .. "\nDo not leave this space blank. Do not use ':'\nTo exit, press Esc.")
            end
            return result, "Response invalid."
        end,
        function()
            -- upon exit, do nothing
            -- profile name is unchanged
            MESSAGEMAN:Broadcast("ProfileRenamed")
            on()
        end
    )
end