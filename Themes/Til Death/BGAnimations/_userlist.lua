local usersZoom = 0.45
local usersWidth = 50
local usersWidthSmall = 25
local usersWidthZoom = 50 * (1 / usersZoom)
local usersWidthSmallZoom = 25 * (1 / usersZoom)
local usersRowSize = 4
local usersX = SCREEN_WIDTH / 4
local usersY = SCREEN_TOP + 15
local usersHeight = 10
local showVisualizer = themeConfig:get_data().global.ShowVisualizer

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
	local x = usersX + usersWidth * ((i-1) % usersRowSize)
	local y = usersY + math.floor((i-1) / usersRowSize) * usersHeight
	local aux =
		LoadFont("Common Normal") ..
		{
			Name = i,
			BeginCommand = function(self)
				if showVisualizer then
					y = y + 25
				end
				self:xy(x, y):zoom(usersZoom):diffuse(posit):queuecommand("Set")
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
