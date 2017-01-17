local usersZoom = 0.35
local usersWidth = 50
local usersWidthSmall = 25
local usersWidthZoom = 50 * (1/usersZoom)
local usersWidthSmallZoom = 25 * (1/usersZoom)
local usersRowSize = 4
local usersX = SCREEN_WIDTH/4
local usersY = SCREEN_TOP+15
local usersHeight = 10

local top = SCREENMAN:GetTopScreen()
local qty = 0
local posit = getMainColor('positive')
local negat = getMainColor('negative')
local enable = getMainColor('enabled')
local r = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set"),
	InitCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self)
		top = SCREENMAN:GetTopScreen()
	end,
	UsersUpdateMessageCommand=cmd(queuecommand,"Set"),
}


local function userLabel(i)
	local x = 0
	local y = 0
	if i <= usersRowSize then
		x = (usersX) + (i*usersWidth)
		y = usersY+usersHeight
	elseif i <= usersRowSize*2 then
		x = (usersX) + ((i-usersRowSize)*usersWidth)
		y = usersY
	elseif i <= usersRowSize*3 then
		x = (usersX) + ((i-usersRowSize*2)*usersWidth) + usersWidthSmall
		y = usersY+usersHeight
	elseif i <= usersRowSize*4 then
		x = (usersX) + ((i-usersRowSize*3)*usersWidth) + usersWidthSmall
		y = usersY
	elseif i <= usersRowSize*5 then
		x = (usersX) + (usersRowSize*usersWidth) + usersWidthSmall * (i-usersRowSize*4)
		y = usersY
	else
		x = (usersX) + (usersRowSize*usersWidth) + usersWidthSmall * (i-usersRowSize*5)
		y = usersY+usersHeight
	end
	local aux = LoadFont("Common Normal") .. {
		Name = i,
		BeginCommand=cmd(xy,x,y;zoom,usersZoom;diffuse,posit;queuecommand,"Set"),
		SetCommand=function(self)
			local num = self:GetName()+0
			qty = top:GetUserQty()
			if num <= qty then
				local str = ""
				str = str ..  top:GetUser(num)
				self:settext(str)
				if top:GetUserState(num) == 2 or top:GetUserState(num) == 1 then
					self:diffuse(posit)
				elseif top:GetUserState(num) == 4 then
					self:diffuse(negat)
				else
					self:diffuse(enable)
				end
			else
				self:settext("")
			end
			if qty < 9 then
				self:maxwidth(usersWidthZoom )
			else
				self:maxwidth(usersWidthSmallZoom)
			end
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		UsersUpdateMessageCommand=cmd(queuecommand,"Set"),
	}
	return aux
end

for i=1,32 do
	r[#r+1] = userLabel(i)
end

return r