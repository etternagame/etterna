--- Initialization of basic functions
-- This file is always executed first.
-- Override Lua's upper and lower functions with our own, which is always UTF-8.
-- @module 00_init

-- @function string.upper
-- @function string.lower
if Uppercase then
	string.upper = Uppercase
	string.lower = Lowercase
	Uppercase = nil -- don't use directly
	Lowercase = nil -- don't use directly
end

--- alias for lua.Trace
-- @tparam string message
-- @function Trace
Trace = lua.Trace
--- lua.Warn alias
-- @tparam string message
-- @function Warn
Warn = lua.Warn
--- lua.Trace alias
-- @tparam string message
-- @function print
print = Trace

-- Use MersenneTwister in place of math.random and math.randomseed.
if MersenneTwister then
	math.random = MersenneTwister.Random
	math.randomseed = MersenneTwister.Seed
end

PLAYER_1 = "PlayerNumber_P1"
PLAYER_2 = "PlayerNumber_P2"
NUM_PLAYERS = #PlayerNumber
OtherPlayer = {[PLAYER_1] = PLAYER_2, [PLAYER_2] = PLAYER_1}

--- Find the last occurence of text in the `string`
-- @string text
-- @return int 0 if not found
function string:find_last(text)
	local LastPos = 0
	while true do
		local p = string.find(self, text, LastPos + 1, true)
		if not p then
			return LastPos
		end
		LastPos = p
	end
end

--- Round to nearest integer.
-- @number n
-- @return number
function math.round(n)
	if n > 0 then
		return math.floor(n + 0.5)
	else
		return math.ceil(n - 0.5)
	end
end

--- split text into a table by the delimiter
-- @string delimiter
-- @string text
-- @return table
function split(delimiter, text)
	local list = {}
	local pos = 1
	while 1 do
		local first, last = string.find(text, delimiter, pos)
		if first then
			table.insert(list, string.sub(text, pos, first - 1))
			pos = last + 1
		else
			table.insert(list, string.sub(text, pos))
			break
		end
	end
	return list
end

--- `table.concat` alias
-- @string delimiter
-- @tparam table list
-- @return string
function join(delimiter, list)
	return table.concat(list, delimiter)
end
