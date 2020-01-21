local update = false
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 5 then
			self:queuecommand("On")
			self:visible(true)
			update = true
		else
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	PlayerJoinedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320, 400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX1 = 100
local offsetX2 = 10
local offsetY = 20

local stringList = {
	{"Etterna Version:", ProductFamily() .. " " .. ProductVersion()},
	{"Theme Version:", getThemeName() .. " " .. getThemeVersion()},
	{"Total Songs:", SONGMAN:GetNumSongs() .. " Songs in " .. SONGMAN:GetNumSongGroups() .. " Groups"},
	{"Total Courses:", SONGMAN:GetNumCourses() .. " Courses in " .. SONGMAN:GetNumCourseGroups() .. " Groups"},
	{"Global Offset:", string.format("%2.4f", (PREFSMAN:GetPreference("GlobalOffsetSeconds") or 0) * 1000) .. " ms"},
	{"Life Difficulty:", GetLifeDifficulty()},
	{"Timing Difficulty:", GetTimingDifficulty()},
	{"Max Machine Scores:", PREFSMAN:GetPreference("MaxHighScoresPerListForMachine") or 0},
	{"Max Personal Scores:", PREFSMAN:GetPreference("MaxHighScoresPerListForPlayer") or 0}
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
	end,
	CodeMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, offsetY):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(
			0.5
		)
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
	end,
	CodeMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 5, frameY + offsetY - 9):zoom(0.6):halign(0):diffuse(getMainColor("positive"))
		end,
		BeginCommand = function(self)
			self:settext("Other Info")
		end
	}

local function makeText1(index)
	return LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + offsetX2, frameY + offsetY + (index * distY)):zoom(fontScale):halign(0):maxwidth(
					offsetX1 / fontScale
				)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settext(stringList[index][1])
			end,
			CodeMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
end

local function makeText2(index)
	return LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + offsetX1 + offsetX2 * 2, frameY + offsetY + (index * distY)):zoom(fontScale):halign(0):maxwidth(
					(frameWidth - offsetX1) / fontScale
				)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settext(stringList[index][2])
			end,
			CodeMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
end

for i = 1, #stringList do
	t[#t + 1] = makeText1(i)
	t[#t + 1] = makeText2(i)
end

return t
