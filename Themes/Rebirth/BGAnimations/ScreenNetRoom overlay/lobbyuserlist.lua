local usersZoom = 0.7
local usersWidth = 70
local usersWidthSmall = 32
local usersWidthZoom = usersWidth * (1 / usersZoom)
local usersWidthSmallZoom = usersWidthSmall * (1 / usersZoom)
local usersRowSize = 10
local usersRowSizeSmall = 20
local usersX = 15
local usersY = SCREEN_TOP + 66
local usersXGap = 7
local usersYGap = 15
local usersHeight = 8

local lobbos
local top = SCREENMAN:GetTopScreen()
local posit = COLORS:getColor("multiplayer", "UserInLobby")
local r = Def.ActorFrame {
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
	local aux = LoadFont("Common Normal") .. {
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

for i = 1, 64 do
	r[#r + 1] = userLabel(i)
end

return r
