local update = false


local frameX = 10
local frameY = 45
local frameWidth = capWideScale(360,400)
local frameHeight = 350
local fontScale = 0.25

local scoreYspacing = 10
local distY = 15
local offsetX = -10
local offsetY = 20
local rankingPage=1	
local rankingWidth = frameWidth-capWideScale(15,50)
local rankingX = capWideScale(30,50)
local rankingY = capWideScale(40,40)
local rankingTitleSpacing = (rankingWidth/(#ms.SkillSets))
local sizeoffet = capWideScale(20,0)
local buttondiffuse = 0
local whee
local listYOffset=100

local Yspacing = 20
local row2Yoffset = 12

local pl


local currentpage = 1
local numpages = 1
local perpage = 10

local packlist = DLMAN:GetPackList()
numpages = notShit.ceil(#packlist/perpage)



local t = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(0,0):diffusealpha(1)
	end,
	MouseRightClickMessageCommand=function(self)
		if update == true then
			inputting = 0
			curInput = 0
			MESSAGEMAN:Broadcast("DlInputEnded")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			MESSAGEMAN:Broadcast("UpdatePacks")
		end
	end,
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 8 then
			SCREENMAN:SetNewScreen("ScreenPackDownloader")
			self:queuecommand("On")
			self:visible(true)
			update = true
		else 
			self:queuecommand("Off")
			update = false
		end
		MESSAGEMAN:Broadcast("DisplayAll")
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
}
return t