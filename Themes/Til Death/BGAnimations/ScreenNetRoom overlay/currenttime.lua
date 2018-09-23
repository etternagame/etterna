local t = Def.ActorFrame {}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "currentTime",
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH - 5, SCREEN_BOTTOM - 5):halign(1):valign(1):zoom(0.45)
		end
	}

local function Update(self)
	local year = Year()
	local month = MonthOfYear() + 1
	local day = DayOfMonth()
	local hour = Hour()
	local minute = Minute()
	local second = Second()
	self:GetChild("currentTime"):settextf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second)
	self:diffuse(getMainColor("positive"))
end

t.InitCommand = function(self)
	self:SetUpdateFunction(Update)
end

return t
