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
local frameWidth = capWideScale(360,400)
local frameHeight = 350
local fontScale = 0.4
local goalsperpage = 25
local distY = 15
local offsetX = 10
local offsetY = 20
local rankingSkillset=0
local goalFilter=1
local rankingWidth = frameWidth-capWideScale(15,50)
local rankingX = capWideScale(30,50)
local rankingY = capWideScale(60,60)
local rankingTitleWidth = (rankingWidth/(3 + 1))

if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
end

local playergoals = profile:GetAllGoals()
local displayindex = {}

t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Goal Tracker (WIP)")}

-- prolly a clever way to cut this to 5 lines - mina
local function filterDisplay (playergoals)
	local index = {}
	if goalFilter == 2 then
		for i=1,#playergoals do
			if playergoals[i]:IsAchieved() then
				index[#index+1] = i
			end
		end
		return index
	elseif goalFilter == 3 then
		for i=1,#playergoals do
			if not playergoals[i]:IsAchieved() then
				index[#index+1] = i
			end
		end
		return index
	end
	for i=1,#playergoals do
		index[#index+1] = i
	end
	return index
end

local function byAchieved(scoregoal)
	if not scoregoal or scoregoal:IsAchieved() then
		return getMainColor('positive')
	end
	return byJudgment("TapNoteScore_Miss")
end
	

local r = Def.ActorFrame{
	UpdateGoalsMessageCommand=function(self)
		playergoals = profile:GetAllGoals()
		displayindex = filterDisplay(playergoals)
	end
}

local function makescoregoal(i)
	local sg		-- scoregoal object -mina
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+12.5,frameY+rankingY+110-(11-i)*10;halign,0.5;zoom,0.25;diffuse,getMainColor('positive');maxwidth,100),
			SetCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settext(((goalFilter-1)*25)+i..".")
			end,
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,400),
			SetCommand=function(self)
				if update then
					sg = playergoals[displayindex[i]]
					if sg then 
						local ck = sg:GetChartKey()
							self:settextf(SONGMAN:GetSongByChartKey(ck):GetDisplayMainTitle())
						else
						self:settext( ' - ' )
					end
					self:diffuse(byAchieved(sg))
				end
			end,
			UpdateGoalsMessageCommand=cmd(queuecommand,"Set");
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+100,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,160),
			SetCommand=function(self)
				if update then 
					if sg then 
						self:settextf("%5.2fx", sg:GetRate())
					else
						self:settext( ' - ' )
					end
					self:diffuse(byAchieved(sg))
				end
			end,
			UpdateGoalsMessageCommand=cmd(queuecommand,"Set");
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+150,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,160),
			SetCommand=function(self)
				if update then 
					if sg then 
						self:settextf("%5.2f", sg:GetPercent())
					else
						self:settext( ' - ' )
					end
					self:diffuse(byAchieved(sg))
				end
			end,
			UpdateGoalsMessageCommand=cmd(queuecommand,"Set");
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+200,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,260),
			SetCommand=function(self)
				if update then 
					if sg then 
						self:settextf(sg:WhenAssigned())
					else
						self:settext( ' - ' )
					end
					self:diffuse(byAchieved(sg))
				end
			end,
			UpdateGoalsMessageCommand=cmd(queuecommand,"Set");
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+300,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,160),
			SetCommand=function(self)
				if update then 
					if sg then 
						self:settextf(sg:GetPriority())
					else
						self:settext( ' - ' )
					end
					self:diffuse(byAchieved(sg))
				end
			end,
			UpdateGoalsMessageCommand=cmd(queuecommand,"Set");
		},
	}
	return t
end



local fawa = {"All Goals","Completed","Incomplete"}
local function rankingButton(i)
	local t = Def.ActorFrame{
		Def.Quad{
		InitCommand=cmd(xy,frameX+rankingX+(i-1+i*(1/(1+3)))*rankingTitleWidth,frameY+rankingY-30;zoomto,rankingTitleWidth,30;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
		SetCommand=function(self)
			if i-1 == rankingSkillset then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.35)
			end
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				goalFilter = i
				MESSAGEMAN:Broadcast("UpdateGoals")
			end
		end,
		UpdateGoalsMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+(i-1+i*(1/(1+3)))*rankingTitleWidth,frameY+rankingY-15;halign,0.5;diffuse,getMainColor('positive');maxwidth,rankingTitleWidth;maxheight,25),
			BeginCommand=function(self)
				self:settext(fawa[i])
			end,
		}
	}
	return t
end


--[
-- should actor frame prev/next/page displays to prevent redundancy - mina
r[#r+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+frameWidth-30,frameY+rankingY+265;zoomto,40,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset > 0 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset > 0 then
			--Move right
			if goalFilter == 10 then
				goalFilter=1
			else
				goalFilter=goalFilter+1
			end
			MESSAGEMAN:Broadcast("UpdateGoals")
		end;
	end;
	UpdateGoalsMessageCommand=cmd(queuecommand,"Set"),
	}
r[#r+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+frameWidth-30,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive');settext,"Next"),
		SetCommand=function(self)
			if rankingSkillset > 0 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
	}
	
r[#r+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+40,frameY+rankingY+265;zoomto,65,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset > 0 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset > 0 then
			--Move left
			if goalFilter == 1 then
				goalFilter=10
			else
				goalFilter=goalFilter-1
			end
			MESSAGEMAN:Broadcast("UpdateGoals")
		end;
	end;
	UpdateGoalsMessageCommand=cmd(queuecommand,"Set"),
	}
	
r[#r+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+40,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive');settext,"Previous"),
		SetCommand=function(self)
			if rankingSkillset > 0 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
	}
	
r[#r+1] = LoadFont("Common Large") .. {
	InitCommand=cmd(xy,frameX+frameWidth/2,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive')),
	SetCommand=function(self)
		if rankingSkillset > 0 then
			self:visible(true)
			self:settextf("%i-%i", ((goalFilter-1)*25)+1, goalFilter*25)
		else
			self:visible(false)
		end
	end,
	UpdateGoalsMessageCommand=cmd(queuecommand,"Set"),
}	
	
	

for i=1,goalsperpage do 
	r[#r+1] = makescoregoal(i)
end
-- Technically the "overall" skillset is used for single value display during music select/eval and isn't factored in to the profile rating
-- Only the specific skillsets are, and so overall should be used to display the specific skillset breakdowns separately - mina
for i=1,3 do
	r[#r+1] = rankingButton(i)
end

t[#t+1] = r

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+80,frameY+rankingY+265;zoomto,100,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset == 0 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset == 0 then
			local saved = PROFILEMAN:SaveProfile(PLAYER_1)
			if saved then
				ms.ok("Save successful")
			else
				ms.ok("Save failed")
			end
		end
	end,
	UpdateRankingMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+80,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive');settext,"Save Profile"),
		SetCommand=function(self)
			if rankingSkillset == 0 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set")
	}

return t