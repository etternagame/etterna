local usersZoom = 0.45
local usersWidth = 50
local usersWidthSmall = 25
local usersWidthZoom = 50 * (1 / usersZoom)
local usersWidthSmallZoom = 25 * (1 / usersZoom)
local usersRowSize = 4
local usersX = SCREEN_WIDTH / 4
local usersY = SCREEN_TOP + 15
local usersHeight = 10

local lobbos
local top = SCREENMAN:GetTopScreen()
local qty = 0
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
	local x = usersX + usersWidth * ((i-1) % usersRowSize)
	local y = usersY + math.floor((i-1) / usersRowSize) * usersHeight
	local aux =
		LoadFont("Common Normal") ..
		{
			Name = i,
			BeginCommand = function(self)
				self:xy(x, y):zoom(usersZoom):diffuse(posit):queuecommand("Set")
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
				if qty < 9 then
					self:maxwidth(usersWidthZoom)
				else
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

for i = 1, 32 do
	r[#r + 1] = userLabel(i)
end

return r
