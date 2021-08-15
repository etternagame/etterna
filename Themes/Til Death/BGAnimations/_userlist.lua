local usersZoom = 0.45
local usersWidth = capWideScale(50,66)
local usersWidthSmall = capWideScale(36,48)
local usersWidthZoom = usersWidth * (1 / usersZoom)
local usersWidthSmallZoom = usersWidthSmall * (1 / usersZoom)
local usersRowSize = 7
local usersRowSizeSmall = 10
local usersX = 6
local usersY = SCREEN_TOP + 40
local usersXGap = 3
local usersYGap = 4
local usersHeight = 8

local top = SCREENMAN:GetTopScreen()
local qty = 0
local posit = getMainColor("positive")
local negat = getMainColor("negative")
local enable = getMainColor("enabled")
local disabled = getMainColor("disabled")
local highlight = getMainColor("highlight")

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
				if SCREENMAN:GetTopScreen():GetName() ~= "ScreenNetSelectMusic" then
					return
				end
				local num = self:GetName() + 0
				qty = top:GetUserQty()
				if num <= qty then
					local str = ""
					str = str .. top:GetUser(num)
					self:settext(str)
					local state = top:GetUserState(num)
					if state == 3 then
						-- eval
						self:diffuse(posit)
					elseif state == 2 then
						-- playing
						self:diffuse(disabled)
					elseif state == 4 then
						-- options
						self:diffuse(highlight)
					else -- 1 == can play
						local ready = top:GetUserReady(num)
						if ready then
							self:diffuse(enable)
						else
							self:diffuse(negat)
						end
					end
				else
					self:settext("")
				end
				if qty < 8 then
					self:x(usersX + (usersWidth + usersXGap) * ((i-1) % usersRowSize))
					self:y(usersY + 6.5)
					self:maxwidth(usersWidthZoom)
				elseif qty < 15 then
					self:x(usersX + (usersWidth + usersXGap) * ((i-1) % usersRowSize))
					self:y(usersY + math.floor((i-1) / usersRowSize) * (usersHeight + usersYGap))
					self:maxwidth(usersWidthZoom)
				else
					self:x(usersX + (usersWidthSmall + usersXGap/3) * ((i-1) % usersRowSizeSmall))
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

for i = 1,20  do
	r[#r + 1] = userLabel(i)
end

return r
