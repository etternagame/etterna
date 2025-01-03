local ret = ... or {}

-- doing a global color move so i can just not repeat these lines of code,
-- which is probably dumb but i'll take responsibility for it.
colors = {
	["Left White"] = "#ffffff",
	["Left Yellow"] = "#ffff00",
	["Left Green"] = "#00ff00",
	["Left Blue"] = "#0000ff",
	["Red"] = "#ff0000",
	["Right Blue"] = "#0000ff",
	["Right Green"] = "#00ff00",
	["Right Yellow"] = "#ffff00",
	["Right White"] = "#ffffff",
}

-- i am not gonna make separate textures for these, feel free to make some if you want to not do this.
ret.RedirTable = {
	["Left White"] = "Key",
	["Left Yellow"] = "Key",
	["Left Green"] = "Key",
	["Left Blue"] = "Key",
	["Red"] = "Key",
	["Right Blue"] = "Key",
	["Right Green"] = "Key",
	["Right Yellow"] = "Key",
	["Right White"] = "Key",
}

local OldRedir = ret.Redir

ret.Redir = function(sButton, sElement)
	sButton, sElement = OldRedir(sButton, sElement)

	--Point the head files back to the tap note
	if string.find(sElement, "Head") or sElement == "Tap Fake" then
		sElement = "Tap Note"
	end

	sButton = ret.RedirTable[sButton]

	return sButton, sElement
end

local OldFunc = ret.Load
function ret.Load()
	local t = OldFunc()

	-- The main "Explosion" part just loads other actors; don't rotate
	-- it.  The "Hold Explosion" part should not be rotated.
	if Var "Element" == "Explosion" or Var "Element" == "Roll Explosion" then
		t.BaseRotationZ = nil
	end
	return t
end

-- where did ret.PartsToRotate go, to the dance store?
-- anyways you will not need it.

ret.Blank = {
	["Hold Explosion"] = true,
	["Roll Explosion"] = true,
	["Hold Topcap Active"] = true,
	["Hold Topcap Inactive"] = true,
	["Roll Topcap Active"] = true,
	["Roll Topcap Inactive"] = true,
	["Hold Tail Active"] = true,
	["Hold Tail Inactive"] = true,
	["Roll Tail Active"] = true,
	["Roll Tail Inactive"] = true
}

return ret
