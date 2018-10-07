-- Tabs are 0 indexed
local tabIndex = 0
local tabSize = 9
local availTabSize = 2

local availableTabs1P = {true, true, true, true, true, true, true, true, true, true}
local availableTabs2P = {true, false, false, false, true}

--0 indexed tabs... yet 1 indexed lua tables mfw. Will probably go into infinite loop if everything is false.
-- Recursively grabs the next available tab. Looping around to start if needed.
local function getNextAvailable(players, index)
	local table = {}
	if players == 1 then
		table = availableTabs1P
	else
		table = availableTabs2P
	end
	if table[index + 1] == true then
		return index
	else
		return getNextAvailable(players, (index + 1) % tabSize)
	end
end

-- Resets the index of the tabs to 0
function resetTabIndex()
	tabIndex = 0
end

function setTabIndex(index)
	if GAMESTATE:GetNumPlayersEnabled() == 1 then
		if availableTabs1P[index + 1] then
			tabIndex = index
		end
	else
		if availableTabs2P[index + 1] then
			tabIndex = index
		end
	end
end

-- Incements the tab index by 1 given the tab is available.
function incrementTabIndex()
	local players = GAMESTATE:GetNumPlayersEnabled()
	tabIndex = (tabIndex + 1) % tabSize
	if players == 1 and availableTabs1P[tabIndex + 1] == false then
		tabIndex = getNextAvailable(players, tabIndex + 1) % tabSize
	end
	if players > 1 and availableTabs2P[tabIndex + 1] == false then
		tabIndex = getNextAvailable(players, tabIndex + 1) % tabSize
	end
end

-- Returns the current tab index
function getTabIndex()
	return tabIndex
end

-- Returns the total number of tabs
function getTabSize()
	return tabSize
end

-- Returns the highest index out of all available tabs
function getMaxAvailIndex()
	local high = 0
	local table = 0
	if GAMESTATE:GetNumPlayersEnabled() == 1 then
		table = availableTabs1P
	else
		table = availableTabs2P
	end
	for k, v in ipairs(table) do
		if v == true then
			high = math.max(high, k)
		end
	end
	return high
end

-- Returns whether a certain tab is enabled
function isTabEnabled(index)
	if GAMESTATE:GetNumPlayersEnabled() == 1 then
		return availableTabs1P[index]
	else
		return availableTabs2P[index]
	end
end
