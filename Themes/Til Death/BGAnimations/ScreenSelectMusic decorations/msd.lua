local update = false
local steps
local song
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0),
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 1 then
			self:queuecommand("On")
			self:visible(true)
			song = GAMESTATE:GetCurrentSong()
			steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
			MESSAGEMAN:Broadcast("UpdateMSDInfo")
			update = true
		else 
			self:queuecommand("Off")
			update = false
		end
	end,
	RefreshChartInfoMessageCommand=cmd(queuecommand,"Set"),
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320,400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]


t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}

-- should make a highlight or something to indicate the greatest value at a quick glance
local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+15,frameY+160 + 22*i;halign,0;valign,0;zoom,0.5;maxwidth,110/0.6),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			if song and steps then
				self:settext(ms.SkillSets[i]..":")
			else
				self:settext("")
			end
		end,
		UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+205,frameY+160 + 22*i;halign,1;valign,0;zoom,0.5;maxwidth,110/0.6),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			if song and steps then
				local meter = steps:GetMSD(getCurRateValue(), i-1)		-- c++ indexing
				self:settextf("%05.2f",meter)
				self:diffuse(ByMSD(meter))
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand=cmd(queuecommand,"Set"),
		UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
		}
	}
	return t
end

t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"MSD Breakdown (Wip)")}
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,frameX+5,frameY+35;zoom,0.6;halign,0;diffuse,getMainColor('positive');maxwidth,SCREEN_CENTER_X/0.7),
	SetCommand=function(self)
		if song then
			self:settext(song:GetDisplayMainTitle())
		else
			self:settext("")
		end
	end,
	UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX+5,frameY+55;zoom,0.6;halign,0),
	SetCommand=function(self)
		if song then
			self:settext(getDifficulty(GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty()))
			self:diffuse(getDifficultyColor(GetCustomDifficulty(GAMESTATE:GetCurrentSteps(PLAYER_1):GetStepsType(),GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty())))
		else
			self:settext("")
		end
	end,
	UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
}

for i=1,#ms.SkillSets do 
	t[#t+1] = littlebits(i)
end

return t