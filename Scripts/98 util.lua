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
function nameAllChildren(actorFrame)
    local s = actorFrame:GetName()
    actorFrame:RunCommandsRecursively(
        function(self)
            s = s .. "\n" .. self:GetName()
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