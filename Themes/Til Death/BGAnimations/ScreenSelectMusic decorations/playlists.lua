local update = false
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0),
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 7 then
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
local fontScale = 0.25
local scoresperpage = 25
local scoreYspacing = 10
local distY = 15
local offsetX = -10
local offsetY = 20
local rankingSkillset=1
local rankingPage=1	
local numrankingpages = 10
local rankingWidth = frameWidth-capWideScale(15,50)
local rankingX = capWideScale(30,50)
local rankingY = capWideScale(40,40)
local rankingTitleSpacing = (rankingWidth/(#ms.SkillSets))
local buttondiffuse = 0.4
local whee

local pl
local keylist
local ratelist
local songlist = {}
local stepslist = {}


if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
end


t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Playlists (WIP)")}

local function BroadcastIfActive(msg)
	if update then
		MESSAGEMAN:Broadcast(msg)
	end
end

local function ButtonActive(self,scale)
	return isOverScaled(self,scale) and update
end


local function makesimpletextbutton(buttontext, leftcmd, rightcmd)
	local o = Def.ActorFrame{
		OnCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetWidth(),self:GetChild("Text"):GetHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=cmd(settext,buttontext)
		},
		Def.Quad{
			Name="Button",
			InitCommand=cmd(diffusealpha,buttondiffuse),
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					self:queuecommand("ButtonLeft")
				end
			end,
			ButtonLeftCommand=leftcmd,
			MouseRightClickMessageCommand=function(self)
				if isOver(self) and update then
					self:queuecommand("ButtonRight")
				end
			end,
			ButtonRightCommand=rightcmd
		}
	}
	return o
end


local r = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(frameX,frameY)
	end,
	OnCommand=function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
	end,
	UpdateRankingMessageCommand=function(self)
		if update then
			pl = SONGMAN:GetActivePlaylist()
			if pl then	
				keylist = pl:GetChartlist()
				ratelist = pl:GetRatelist()
				
				for j=1,#keylist do
					songlist[j] = SONGMAN:GetSongByChartKey(keylist[j])
					stepslist[j] = SONGMAN:GetStepsByChartKey(keylist[j])
				end
					
				--songlist = pl:GetSonglist()
				--stepslist = pl:GetStepslist()
				self:visible(true)
				self:RunCommandsOnChildren(cmd(queuecommand, "Display"))
			end
		else
			self:visible(false)
		end
	end,
	LoadFont("Common Large") .. {
		InitCommand=cmd(xy,rankingX,rankingY;zoom,0.4;halign,0),
		DisplayCommand=function(self)
			self:settext(pl:GetName())
		end
	}
}

r[#r+1] = makesimpletextbutton("Add Chart", function() pl:AddChart(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey()) end)

local function RateDisplayButton(i)
	local o = Def.ActorFrame{
		InitCommand=cmd(x,220),
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			DisplayCommand=function(self)
				local ratestring = string.format("%.2f", ratelist[i]):gsub("%.?0+$", "").."x"
				self:settext(ratestring)
				self:GetParent():queuecommand("Resize")
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=cmd(diffusealpha,buttondiffuse),
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) then
					pl:ChangeRateAtIndex(i,0.1)
					BroadcastIfActive("UpdateRanking")
				end
			end,
			MouseRightClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) then
					pl:ChangeRateAtIndex(i,-0.1)
					BroadcastIfActive("UpdateRanking")
				end
			end
		}
	}
	return o
end

local function TitleDisplayButton(i)
	local o = Def.ActorFrame{
		InitCommand=cmd(x,15),
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=cmd(halign,0),
			DisplayCommand=function(self)
				self:settext(songlist[i]:GetDisplayMainTitle())
				self:GetParent():queuecommand("Resize")
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=cmd(diffusealpha,buttondiffuse;halign,0),
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) then
					whee:SelectSong(songlist[i])
				end
			end
		}
	}
	return o
end
	
local function rankingLabel(i)	
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY + 10 + (i-1)*scoreYspacing)
			self:RunCommandsOnChildren(cmd(halign,0;zoom,fontScale))
			self:visible(false)
		end,
		UpdateRankingMessageCommand=function(self)
			if rankingSkillset > 1 and update then
				pl = SONGMAN:GetActivePlaylist()
				keylist = pl:GetChartlist()
				ratelist = pl:GetRatelist()
				
				for j=1,#keylist do
					songlist[j] = SONGMAN:GetSongByChartKey(keylist[j])
					stepslist[j] = SONGMAN:GetStepsByChartKey(keylist[j])
				end
				--songlist = pl:GetSonglist()
				--stepslist = pl:GetStepslist()
				self:visible(true)
				self:RunCommandsOnChildren(cmd(queuecommand, "Display"))
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=cmd(maxwidth,100),
			DisplayCommand=function(self)
				self:halign(0.5)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+i..".")
			end
		},
		LoadFont("Common Large") .. {	-- pack mouseover for later
			InitCommand=cmd(x,15;maxwidth,580),
			DisplayCommand=function(self)
				--self:settext(songlist[i]:GetGroupName())
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(x,256;maxwidth,160),
			DisplayCommand=function(self)
				local rating = stepslist[i]:GetMSD(1,ratelist[i])
				self:settextf("%.2f", rating)
				self:diffuse(ByMSD(rating))
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(x,300),
			DisplayCommand=function(self)
				local diff = stepslist[i]:GetDifficulty()
				self:halign(0.5)
				self:diffuse(byDifficulty(diff))
				self:settext(getShortDifficulty(diff))
			end
		}
	}
	t[#t+1] = RateDisplayButton(i)
	t[#t+1] = TitleDisplayButton(i)
	return t
end

local function rankingButton(i)
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + (i-1)*rankingTitleSpacing, rankingY)
		end,
		Def.Quad{
			InitCommand=cmd(zoomto,rankingTitleSpacing,30;diffuse,getMainColor('frames');diffusealpha,0.35),
			SetCommand=function(self)
				if i == rankingSkillset then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.35)
				end
			end,
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self) then
					rankingSkillset = i
					rankingPage = 1
					SCOREMAN:SortSSRs(ms.SkillSets[rankingSkillset])
					BroadcastIfActive("UpdateRanking")
				end
			end,
			UpdateRankingMessageCommand=cmd(queuecommand,"Set")
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(diffuse,getMainColor('positive');maxwidth,rankingTitleSpacing;maxheight,25;zoom,0.85),
			BeginCommand=function(self)
				self:settext(ms.SkillSets[i])
			end
		}
	}
	return t
end

-- prev/next page
r[#r+1] = Def.ActorFrame{
	InitCommand=cmd(xy, 10, frameHeight - offsetY;visible,false),
	UpdateRankingMessageCommand=function(self)
		if rankingSkillset > 1 then 
			self:visible(true)
			self:RunCommandsOnChildren(cmd(queuecommand, "Display"))
		else
			self:visible(false)
		end
	end,
	Def.Quad{
		InitCommand=cmd(xy,300,-8;zoomto,40,20;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,buttondiffuse),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				if	rankingPage < numrankingpages then
					rankingPage = rankingPage + 1
				else
					rankingPage = 1
				end
				BroadcastIfActive("UpdateRanking")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(x,300;halign,0;zoom,0.3;diffuse,getMainColor('positive');settext,"Next"),
	},	
	Def.Quad{
		InitCommand=cmd(y,-8;zoomto,65,20;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,buttondiffuse),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then 
				if rankingPage > 1 then
					rankingPage = rankingPage - 1
				else
					rankingPage = numrankingpages
				end
				BroadcastIfActive("UpdateRanking")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(halign,0;zoom,0.3;diffuse,getMainColor('positive');settext,"Previous"),
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(x,175;halign,0.5;zoom,0.3;diffuse,getMainColor('positive')),
		DisplayCommand=function(self)
			self:settextf("%i-%i", ((rankingPage-1)*25)+1, rankingPage*25)
		end
	}
}

for i=1,scoresperpage do 
	r[#r+1] = rankingLabel(i)
end

-- Technically the "overall" skillset is used for single value display during music select/eval and isn't factored in to the profile rating
-- Only the specific skillsets are, and so overall should be used to display the specific skillset breakdowns separately - mina
for i=1,#ms.SkillSets do
	r[#r+1] = rankingButton(i)
end



t[#t+1] = r
return t