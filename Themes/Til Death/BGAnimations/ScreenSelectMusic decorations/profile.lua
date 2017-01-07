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
local rankingSkillset=0
local rankingPage=1
local rankingWidth = 250
local rankingX = capWideScale(75,90)
local rankingY = capWideScale(200,180)
local rankingTitleWidth = (rankingWidth/(#ms.SkillSets + 1))

if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
end


t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}

t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Profile Info (WIP)")}


local function input(event)
	if event.type ~= "InputEventType_Release" and active then
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		end
	end
	return false
end


local r = Def.ActorFrame{
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end,
	}
	
local function rankingLabel(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,160),
			SetCommand=function(self)
				local a=profile:GetTopSSRValue(i+(10*(rankingPage-1)), rankingSkillset)
				if a==0 then
					self:settext( ' - ' )
				else
					self:settextf("%5.2f", a)
				end
			end,
			UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+40,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,rankingWidth*4-160),
			SetCommand=function(self)
				local a=profile:GetTopSSRValue(i+(10*(rankingPage-1)), rankingSkillset)
				if a==0 then
					self:settext( ' ' )
				else
					self:settext(profile:GetTopSSRSongName(i+(10*(rankingPage-1)), rankingSkillset) )
				end
			end,
			UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
		},
	}
	return t
end
local function rankingButton(i)
	local t = Def.ActorFrame{
		Def.Quad{
		InitCommand=cmd(xy,frameX+rankingX+(i-1+i*(1/(1+#ms.SkillSets)))*rankingTitleWidth,frameY+rankingY-30;zoomto,rankingTitleWidth,30;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
		SetCommand=function(self)
			if i-1 == rankingSkillset then
				self:diffusealpha(0.7)
			else
				self:diffusealpha(0.35)
			end;
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				rankingSkillset = i-1
				MESSAGEMAN:Broadcast("UpdateRanking")
			end;
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+(i-1+i*(1/(1+#ms.SkillSets)))*rankingTitleWidth,frameY+rankingY-15;halign,0;diffuse,getMainColor('positive');maxwidth,rankingTitleWidth;maxheight,25),
			BeginCommand=function(self)
				self:settext(ms.SkillSets[i])
			end,
		}
	}
	return t
end


r[#r+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+rankingX+rankingWidth,frameY+rankingY+40;zoomto,40,20;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) then
			--Move right
			if rankingPage == 10 then
				rankingPage=1
			else
				rankingPage=rankingPage+1
			end
			MESSAGEMAN:Broadcast("UpdateRanking")
		end;
	end;
	}
r[#r+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+rankingX+rankingWidth,frameY+rankingY+50;halign,0;zoom,0.3;diffuse,getMainColor('positive')),
		BeginCommand=function(self)
			self:settext( 'Next' )
		end,
	}
	
r[#r+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+rankingX-75,frameY+rankingY+40;zoomto,65,20;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) then
			--Move left
			if rankingPage == 1 then
				rankingPage=10
			else
				rankingPage=rankingPage-1
			end
			MESSAGEMAN:Broadcast("UpdateRanking")
		end;
	end;
	}
r[#r+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+rankingX-75,frameY+rankingY+50;halign,0;zoom,0.3;diffuse,getMainColor('positive')),
		BeginCommand=function(self)
			self:settext( 'Previous' )
		end,
	}
for i=1,10 do 
	r[#r+1] = rankingLabel(i)
end
for i=1,#ms.SkillSets do 
	r[#r+1] = rankingButton(i)
end

t[#t+1] = r

-- should make a highlight or something to indicate the greatest value at a quick glance
local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+(frameWidth/2)-100,frameY+20 + 22*i;halign,0;zoom,0.5;diffuse,getMainColor('positive')),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				self:settext(ms.SkillSets[i]..":")
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+(frameWidth/2)+90,frameY+20 + 22*i;halign,1;zoom,0.5),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				local rating = profile:GetPlayerSkillsetRating(i)
				self:settextf("%5.2f",rating)
				self:diffuse(ByMSD(rating))
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
