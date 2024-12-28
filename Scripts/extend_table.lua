-- Utilities for better table manipulation

-- split a string into a table
-- http://lua-users.org/wiki/SplitJoin (but with error messages)
function string.split(self, sSeparator, nMax, bRegexp)
	assert(sSeparator ~= "", "empty separator is not allowed.")
	assert(nMax == nil or nMax >= 1, "max must be a positive number.")

	local aRecord = {}

	if self:len() > 0 then
		local bPlain = not bRegexp
		nMax = nMax or -1

		local nField = 1
		nStart = 1
		local nFirst, nLast = self:find(sSeparator, nStart, bPlain)
		while nFirst and nMax ~= 0 do
			aRecord[nField] = self:sub(nStart, nFirst - 1)
			nField = nField + 1
			nStart = nLast + 1
			nFirst, nLast = self:find(sSeparator, nStart, bPlain)
			nMax = nMax - 1
		end
		aRecord[nField] = self:sub(nStart)
	end

	return aRecord
end

-- table.concat alias for convenience.
table.join = table.concat

-- insert multiple elements into a table at once
function table.push(self, ...)
	for _, v in ipairs({...}) do
		table.insert(self, v)
	end
end

-- because concat and join dont really do what i want
function table.combine(...)
	local o = {}
	for _, t in ipairs({...}) do
		for __, v in pairs(t) do
			o[#o+1] = v
		end
	end
	return o
end

-- its like table.sort but it returns a copy
-- NOT A DEEP COPY probably
function table.sorted(t, sortfunc)
	local o = {}
	for k, v in pairs(t) do
		o[k] = v
	end
	if sortfunc ~= nil then
		table.sort(o, sortfunc)
	else
		table.sort(o)
	end
	return o
end

-- given the initial table, add the contents of another table to it
function table.extend(t, othertable)
	for k,v in pairs(othertable) do
		t[k] = v
	end
end

-- given the initial table, apply a function to all the elements and return a copy
-- the func paremeters are (key, value) and the return type is also key, value
function table.withfuncapplied(t, func)
	local o = {}
	for k,v in pairs(t) do
		local transformedKey, transformedValue = func(k,v)
		if transformedKey ~= nil then
			o[transformedKey] = transformedValue
		end
	end
	return o
end