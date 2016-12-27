local update = false
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0),
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		
		if getTabIndex() == 4 then
		
			self:queuecommand("On")
			self:visible(true)
			update = true
		else 
			self:queuecommand("Off")
			update = false
		end
	end,
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

local skillsets = {
	Overall = 0,
	Speed 	= 0,
	Stam  	= 0,
	Jack  	= 0,
	Technical = 0
}


if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
	if profile ~= nil then
		skillsets.Overall = profile:GetPlayerRating()
		skillsets.Speed = profile:GetPlayerSpeedRating()
		skillsets.Stam = profile:GetPlayerStamRating()
		skillsets.Jack = profile:GetPlayerJackRating()
		skillsets.Technical = 1
	end
end


t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}

t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Profile Info (WIP)")}


-- should make a highlight or something to indicate the greatest value at a quick glance
local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+15,frameY+160 + 22*i;halign,0;zoom,0.5;diffuse,getMainColor('positive')),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				self:settext(ms.SkillSets[i]..":")
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+205,frameY+160 + 22*i;halign,1;zoom,0.5),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				self:settextf("%5.2f",skillsets[ms.SkillSets[i]])
				self:diffuse(ByMSD(skillsets[ms.SkillSets[i]]))
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		}
	}
	return t
end

for i=1,#ms.SkillSets do 
	t[#t+1] = littlebits(i)
end


return t