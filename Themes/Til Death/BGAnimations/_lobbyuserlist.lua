local usersZoom = 0.45
local usersWidth = 50
local usersWidthSmall = capWideScale(25,32)
local usersWidthZoom = usersWidth * (1 / usersZoom)
local usersWidthSmallZoom = usersWidthSmall * (1 / usersZoom)
local usersRowSize = 10
local usersRowSizeSmall = 20
local usersX = SCREEN_WIDTH / 7
local usersY = SCREEN_TOP + 10
local usersXGap = 4
local usersYGap = 4
local usersHeight = 8

local lobbos
local top = SCREENMAN:GetTopScreen()
local posit = getMainColor("positive")
local negat = getMainColor("negative")
local enable = getMainColor("enabled")
local r =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	InitCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		top = SCREENMAN:GetTopScreen()
		lobbos = NSMAN:GetLobbyUserList()
	end,
	UsersUpdateMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

local function userLabel(i)
	local aux =
		LoadFont("Common Normal") ..
		{
			Name = i,
			BeginCommand = function(self)
				self:halign(0)
				self:zoom(usersZoom):diffuse(posit):queuecommand("Set")
			end,
			SetCommand = function(self)
				if SCREENMAN:GetTopScreen():GetName() ~= "ScreenNetRoom" then
					return
				end
				local num = self:GetName() + 0
				if num <= #lobbos then
					-- if top:GetUserState(num) == 2 or top:GetUserState(num) == 1 then
					-- 	self:diffuse(posit)
					-- elseif top:GetUserState(num) == 4 then
					-- 	self:diffuse(negat)
					-- else
					-- 	self:diffuse(enable)
					-- end
					self:settext(lobbos[num])
				else
					self:settext("")
				end
				if #lobbos < 21 then
					self:x(usersX + (usersWidth + usersXGap) * ((i-1) % usersRowSize))
					self:y(usersY + math.floor((i-1) / usersRowSize) * (usersHeight + usersYGap))
					self:maxwidth(usersWidthZoom)
				else
					self:x(usersX + (usersWidthSmall + usersXGap/2) * ((i-1) % usersRowSizeSmall))
					self:y(usersY + math.floor((i-1) / usersRowSizeSmall) * (usersHeight + usersYGap))
					self:maxwidth(usersWidthSmallZoom)
				end
			end,
			PlayerJoinedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			PlayerUnjoinedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			UsersUpdateMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
	return aux
end

for i = 1, 40 do
	r[#r + 1] = userLabel(i)
end

return r
