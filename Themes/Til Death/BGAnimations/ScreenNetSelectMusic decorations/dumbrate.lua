--A dumb way of getting the rate to update online. -Misterkister

--Local vars
local update = false
local steps
local song
local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320, 400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]
local greatest = 0
local steps
local meter = {}
local curateX = frameX + frameWidth + 5
local curateY = frameY + offsetY + 140
meter[1] = 0.00

--4:3 ratio. -Misterkister
if not IsUsingWideScreen() == true then
	curateX = frameX + frameWidth - 40
	curateY = frameY + offsetY - 20
end

--Hacky way of fixing these ratios outside of 16:9 and 4:3. -Misterkister

--16:10 ratio. -Misterkister
if round(GetScreenAspectRatio(), 5) == 1.6 then
	curateX = frameX + frameWidth - 8
end

--5:4 ratio. -Misterkister
if round(GetScreenAspectRatio(), 5) == 1.25 then
	curateX = frameX + frameWidth - 80
end

--8:3 ratio targeted. -Misterkister
if round(GetScreenAspectRatio(), 5) > 1.77778 then
	curateX = frameX + frameWidth + 425
end

--Actor Frame
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
		if getTabIndex() == 0 then
			self:queuecommand("On")
			self:visible(true)
			song = GAMESTATE:GetCurrentSong()
			steps = GAMESTATE:GetCurrentSteps(PLAYER_1)

			--Find max MSD value, store MSD values in meter[]
			greatest = 0
			if song and steps then
				for i = 1, #ms.SkillSets do
					meter[i + 1] = steps:GetMSD(getCurRateValue(), i)
					if meter[i + 1] > meter[greatest + 1] then
						greatest = i
					end
				end
			end

			MESSAGEMAN:Broadcast("UpdateMSDInfo")
			update = true
		else
			self:queuecommand("Off")
			update = false
		end
	end,
	CurrentRateChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	RefreshChartInfoMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	PlayerJoinedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

--Skillset label function
local function littlebits(i)
	local t =
		Def.ActorFrame {
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 35, frameY + 120 + 22 * i):halign(0):valign(0):zoom(0.5):maxwidth(110 / 0.6)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					--skillset name
					if song and steps then
						self:settext(ms.SkillSets[i] .. ":")
					else
						self:settext("")
					end
					--highlight
					if greatest == i then
						self:diffuse(getMainColor("positive"))
					else
						self:diffuse(getMainColor("negative"))
					end
				end,
				UpdateMSDInfoCommand = function(self)
					self:queuecommand("Set")
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 225, frameY + 120 + 22 * i):halign(1):valign(0):zoom(0.5):maxwidth(110 / 0.6)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					if song and steps then
						self:settextf("%05.2f", meter[i + 1])
						self:diffuse(byMSD(meter[i + 1]))
					else
						self:settext("")
					end
				end,
				CurrentRateChangedMessageCommand = function(self)
					self:queuecommand("Set")
				end,
				UpdateMSDInfoCommand = function(self)
					self:queuecommand("Set")
				end
			}
	}
	return t
end

-- Music Rate Display
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(curateX, curateY):visible(true):halign(0):zoom(0.35):maxwidth(
				capWideScale(get43size(360), 360) / capWideScale(get43size(0.45), 0.45)
			)
		end,
		SetCommand = function(self)
			self:settext(getCurRateDisplayString())
		end,
		CodeMessageCommand = function(self, params)
			local rate = getCurRateValue()
			ChangeMusicRate(rate, params)
			self:settext(getCurRateDisplayString())
		end,
		RateChangedMessageCommand = function(self, params)
			self:settext(getCurRateDisplayString())
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("set")
		end
	}

return t
