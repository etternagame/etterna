local t = Def.ActorFrame {}
local topFrameHeight = 35
local bottomFrameHeight = 54
local borderWidth = 4

local t =
	Def.ActorFrame {
	Name = "PlayerAvatar"
}

local profileP1

local profileNameP1 = THEME:GetString("GeneralInfo", "NoProfile")
local playCountP1 = 0
local playTimeP1 = 0
local noteCountP1 = 0

local AvatarXP1 = 5
local AvatarYP1 = 50

local bpms = {}
if GAMESTATE:GetCurrentSong() then
	bpms = GAMESTATE:GetCurrentSong():GetDisplayBpms()
	bpms[1] = math.round(bpms[1])
	bpms[2] = math.round(bpms[2])
end

-- P1 Avatar
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(AvatarXP1 - 15, AvatarYP1 - 5):halign(0):valign(0):zoomto(250, 40):diffuse(color("#000000")):diffusealpha(.8)
	end
}

t[#t + 1] =
	Def.Actor {
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
			profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
			if profileP1 ~= nil then
				profileNameP1 = profileP1:GetDisplayName()
				playCountP1 = profileP1:GetTotalNumSongsPlayed()
				playTimeP1 = profileP1:GetTotalSessionSeconds()
				noteCountP1 = profileP1:GetTotalTapsAndHolds()
			else
				profileNameP1 = THEME:GetString("GeneralInfo", "NoProfile")
				playCountP1 = 0
				playTimeP1 = 0
				noteCountP1 = 0
			end
		else
			profileNameP1 = THEME:GetString("GeneralInfo", "NoProfile")
			playCountP1 = 0
			playTimeP1 = 0
			noteCountP1 = 0
		end
	end
}

t[#t + 1] =
	Def.ActorFrame {
	Name = "Avatar" .. PLAYER_1,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		if profileP1 == nil then
			self:visible(false)
		else
			self:visible(true)
		end
	end,
	Def.Sprite {
		Name = "Image",
		InitCommand = function(self)
			self:visible(true):halign(0):valign(0):xy(AvatarXP1, AvatarYP1)
		end,
		BeginCommand = function(self)
			self:queuecommand("ModifyAvatar")
		end,
		ModifyAvatarCommand = function(self)
			self:finishtweening()
			self:Load(getAvatarPath(PLAYER_1))
			self:zoomto(30, 30)
		end
	},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(AvatarXP1 + 33, AvatarYP1 + 9):halign(0):zoom(0.45)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settextf("%s%s", profileNameP1, THEME:GetString("ScreenPlayerOptions", "ScrollSpeed"))
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(AvatarXP1 + 33, AvatarYP1 + 20):halign(0):zoom(0.40)
			end,
			BeginCommand = function(self)
				local speed, mode = GetSpeedModeAndValueFromPoptions(PLAYER_1)
				self:playcommand("SpeedChoiceChanged", {pn = PLAYER_1, mode = mode, speed = speed})
			end,
			SpeedChoiceChangedMessageCommand = function(self, param)
				if param.pn == PLAYER_1 then
					local text = ""
					if param.mode == "x" then
						if not bpms[1] then
							text = "??? - ???"
						elseif bpms[1] == bpms[2] then
							text = math.round(bpms[1] * param.speed / 100)
						else
							text = string.format("%d - %d", math.round(bpms[1] * param.speed / 100), math.round(bpms[2] * param.speed / 100))
						end
					elseif param.mode == "C" then
						text = param.speed
					else
						if not bpms[1] then
							text = "??? - " .. param.speed
						elseif bpms[1] == bpms[2] then
							text = param.speed
						else
							local factor = param.speed / bpms[2]
							text = string.format("%d - %d", math.round(bpms[1] * factor), param.speed)
						end
					end
					self:settext(text)
				end
			end
		}
}

--Frames
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(0, 0):halign(0):valign(0):zoomto(SCREEN_WIDTH, topFrameHeight):diffuse(color("#000000"))
	end
}
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(0, topFrameHeight):halign(0):valign(1):zoomto(SCREEN_WIDTH, borderWidth):diffuse(getMainColor("highlight")):diffusealpha(
			0.5
		)
	end
}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("positive"))
			self:settextf("%s:", THEME:GetString("ScreenPlayerOptions", "Title"))
		end
	}

local NSPreviewSize = 0.5
local NSPreviewX = 20
local NSPreviewY = 125
local NSPreviewXSpan = 35
local NSPreviewReceptorY = -30
local OptionRowHeight = 35
local NoteskinRow = 0
local NSDirTable = GameToNSkinElements()

local function NSkinPreviewWrapper(dir, ele)
	return Def.ActorFrame {
		InitCommand = function(self)
			self:zoom(NSPreviewSize)
		end,
		LoadNSkinPreview("Get", dir, ele, PLAYER_1)
	}
end
local function NSkinPreviewExtraTaps()
	local out = Def.ActorFrame {}
	for i = 2, #NSDirTable do
		out[#out+1] = Def.ActorFrame {
			Def.ActorFrame {
				InitCommand = function(self)
					self:x(NSPreviewXSpan * (i-1))
				end,
				NSkinPreviewWrapper(NSDirTable[i], "Tap Note")
			},
			Def.ActorFrame {
				InitCommand = function(self)
					self:x(NSPreviewXSpan * (i-1)):y(NSPreviewReceptorY)
				end,
				NSkinPreviewWrapper(NSDirTable[i], "Receptor")
			}
		}
	end
	return out
end
t[#t + 1] =
	Def.ActorFrame {
	OnCommand = function(self)
		self:xy(NSPreviewX, NSPreviewY)
		for i = 0, SCREENMAN:GetTopScreen():GetNumRows() - 1 do
			if SCREENMAN:GetTopScreen():GetOptionRow(i) and SCREENMAN:GetTopScreen():GetOptionRow(i):GetName() == "NoteSkins" then
				NoteskinRow = i
			end
		end
		self:SetUpdateFunction(
			function(self)
				local row = SCREENMAN:GetTopScreen():GetCurrentRowIndex(PLAYER_1)
				local pos = 0
				if row > 4 then
					pos =
						NSPreviewY + NoteskinRow * OptionRowHeight -
						(SCREENMAN:GetTopScreen():GetCurrentRowIndex(PLAYER_1) - 4) * OptionRowHeight
				else
					pos = NSPreviewY + NoteskinRow * OptionRowHeight
				end
				self:y(pos)
				self:visible(NoteskinRow - row > -5 and NoteskinRow - row < 7)
			end
		)
	end,
	Def.ActorFrame {
		NSkinPreviewWrapper(NSDirTable[1], "Tap Note")
	},
	Def.ActorFrame {
		InitCommand = function(self)
			self:y(NSPreviewReceptorY)
		end,
		NSkinPreviewWrapper(NSDirTable[1], "Receptor")
	}
}
if GetScreenAspectRatio() > 1.7 then
	t[#t][#(t[#t]) + 1] = NSkinPreviewExtraTaps()
end
return t
