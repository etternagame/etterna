-- refactor this using the goaltracker code framework after i finish making it not shit -mina

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
local scorestodisplay = 25
local distY = 15
local offsetX = 10
local offsetY = 20
local rankingSkillset=1
local rankingPage=1	
local rankingWidth = frameWidth-capWideScale(15,50)
local rankingX = capWideScale(30,50)
local rankingY = capWideScale(60,60)
local rankingTitleWidth = (rankingWidth/(#ms.SkillSets + 1))

if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
end


t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Profile Info (WIP)")}


local function byValidity(valid)
	if valid then
		return getMainColor('positive')
	end
	byJudgment("TapNoteScore_Miss")
end

-- The input callback for mouse clicks already exists within the tabmanager and redefining it within the local scope does nothing but create confusion - mina
local r = Def.ActorFrame{}
	
local function rankingLabel(i)
	local ths -- the top highscore object - mina
	local ck
	local thssteps
	local thssong
	
	local t = Def.ActorFrame{
		InitCommand=cmd(visible, false),
		UpdateRankingMessageCommand=function(self)
			if rankingSkillset > 1 and update then
				SCOREMAN:SortSSRs(ms.SkillSets[rankingSkillset])
				ths = SCOREMAN:GetTopSSRHighScore(i+(scorestodisplay*(rankingPage-1)), ms.SkillSets[rankingSkillset])
				if ths then
					self:visible(true)
					ck = ths:GetChartKey()						
					thssong = SONGMAN:GetSongByChartKey(ck)
					thssteps = SONGMAN:GetStepsByChartKey(ck)
					
					self:RunCommandsOnChildren(cmd(queuecommand, "Display"))
				end
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+12.5,frameY+rankingY+110-(11-i)*10;halign,0.5;zoom,0.25;diffuse,getMainColor('positive');maxwidth,100),
			DisplayCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+i..".")
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,160),
			DisplayCommand=function(self)
				self:settextf("%5.2f", ths:GetSkillsetSSR(ms.SkillSets[rankingSkillset]))
				self:diffuse(byValidity(ths:GetEtternaValid()))
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+35,frameY+rankingY+110-(11-i)*10;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,rankingWidth*2.5-160),
			DisplayCommand=function(self)
				self:settext(thssong:GetDisplayMainTitle())
				self:diffuse(byValidity(ths:GetEtternaValid()))
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+230,frameY+rankingY+110-(11-i)*10;halign,0.5;zoom,0.25;diffuse,getMainColor('positive');maxwidth,rankingWidth*4-160),
			DisplayCommand=function(self)
				local ratestring = string.format("%.2f", ths:GetMusicRate()):gsub("%.?0+$", "").."x"
				self:settext(ratestring)
				self:diffuse(byValidity(ths:GetEtternaValid()))
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+270,frameY+rankingY+110-(11-i)*10;halign,0.5;zoom,0.25;diffuse,getMainColor('positive');maxwidth,rankingWidth*4-160),
			DisplayCommand=function(self)
					self:settextf("%5.2f%%", ths:GetWifeScore()*100)
					if not ths:GetEtternaValid() then
						self:diffuse(byJudgment("TapNoteScore_Miss"))
					else
						self:diffuse(getGradeColor(ths:GetWifeGrade()))
					end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+310,frameY+rankingY+110-(11-i)*10;halign,0.5;zoom,0.25;diffuse,getMainColor('positive');maxwidth,rankingWidth*4-160),
			DisplayCommand=function(self)
				local diff = thssteps:GetDifficulty()
				self:diffuse(byDifficulty(diff))
				self:settext(getShortDifficulty(diff))
			end
		},
		Def.Quad{
			InitCommand=cmd(xy,rankingX+rankingWidth/2,frameY+rankingY+105-(11-i)*10;zoomto,rankingWidth,10;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0),
			MouseRightClickMessageCommand=function(self)
				if update and ths and isOver(self) then 
					ths:ToggleEtternaValidation()
					MESSAGEMAN:Broadcast("UpdateRanking")
					if ths:GetEtternaValid() then 
						ms.ok("Score Revalidated")
					else
						ms.ok("Score Invalidated")
					end
				end
			end,
			MouseLeftClickMessageCommand=function(self)
				if update and ths and isOver(self) then 
					local whee = SCREENMAN:GetTopScreen():GetMusicWheel()
					whee:SelectSong(thssong)
				end
			end
		}
	}
	return t
end

local function rankingButton(i)
	local t = Def.ActorFrame{
		Def.Quad{
		InitCommand=cmd(xy,frameX+rankingX+(i-1+i*(1/(1+#ms.SkillSets)))*rankingTitleWidth,frameY+rankingY-30;zoomto,rankingTitleWidth,30;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
		SetCommand=function(self)
			if i == rankingSkillset then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.35)
			end
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				rankingSkillset = i
				rankingPage = 1
				MESSAGEMAN:Broadcast("UpdateRanking")
			end
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+rankingX+(i-1+i*(1/(1+#ms.SkillSets)))*rankingTitleWidth,frameY+rankingY-15;halign,0.5;diffuse,getMainColor('positive');maxwidth,rankingTitleWidth;maxheight,25),
			BeginCommand=function(self)
				self:settext(ms.SkillSets[i])
			end,
		}
	}
	return t
end



-- should actor frame prev/next/page displays to prevent redundancy - mina
r[#r+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+frameWidth-30,frameY+rankingY+265;zoomto,40,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset > 1 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset > 0 then
			--Move right
			if rankingPage == 10 then
				rankingPage=1
			else
				rankingPage=rankingPage+1
			end
			MESSAGEMAN:Broadcast("UpdateRanking")
		end;
	end;
	UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
	}
r[#r+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+frameWidth-30,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive');settext,"Next"),
		SetCommand=function(self)
			if rankingSkillset > 1 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set")
	}
	
r[#r+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+40,frameY+rankingY+265;zoomto,65,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset > 1 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset > 0 then
			--Move left
			if rankingPage == 1 then
				rankingPage=10
			else
				rankingPage=rankingPage-1
			end
			MESSAGEMAN:Broadcast("UpdateRanking")
		end;
	end;
	UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
	}
	
r[#r+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+40,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive');settext,"Previous"),
		SetCommand=function(self)
			if rankingSkillset > 1 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set")
	}
	
r[#r+1] = LoadFont("Common Large") .. {
	InitCommand=cmd(xy,frameX+frameWidth/2,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive')),
	SetCommand=function(self)
		if rankingSkillset > 1 then
			self:visible(true)
			self:settextf("%i-%i", ((rankingPage-1)*25)+1, rankingPage*25)
		else
			self:visible(false)
		end
	end,
	UpdateRankingMessageCommand=cmd(queuecommand,"Set"),
}	
	
for i=1,scorestodisplay do 
	r[#r+1] = rankingLabel(i)
end

-- Technically the "overall" skillset is used for single value display during music select/eval and isn't factored in to the profile rating
-- Only the specific skillsets are, and so overall should be used to display the specific skillset breakdowns separately - mina
for i=1,#ms.SkillSets do
	r[#r+1] = rankingButton(i)
end

t[#t+1] = r

-- should make a highlight or something to indicate the greatest value at a quick glance
local function littlebits(i)
	local t = Def.ActorFrame{
		UpdateRankingMessageCommand=function(self)
			if rankingSkillset == 1 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+30,frameY+120 + 22*i;halign,0;zoom,0.5;diffuse,getMainColor('positive')),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				self:settext(ms.SkillSets[i]..":")
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+270,frameY+120 + 22*i;halign,1;zoom,0.5),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				local rating = profile:GetPlayerSkillsetRating(ms.SkillSets[i])
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

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+80,frameY+rankingY+265;zoomto,100,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset == 1 then
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

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+320,frameY+rankingY+265;zoomto,100,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset == 1 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset == 0 then
			local saved = PROFILEMAN:ConvertProfile(PLAYER_1)
			if saved then
				ms.ok("Convert")
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
			if rankingSkillset == 1 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set")
	}
	
t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,frameX+210,frameY+rankingY+265;zoomto,100,20;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
	SetCommand=function(self)
		if rankingSkillset == 1 then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and rankingSkillset == 0 then
			profile:ValidateAllScores()
		end
	end,
	UpdateRankingMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+210,frameY+rankingY+275;halign,0.5;zoom,0.3;diffuse,getMainColor('positive');settext,"Validate All"),
		SetCommand=function(self)
			if rankingSkillset == 1 then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		UpdateRankingMessageCommand=cmd(queuecommand,"Set")
	}

return t