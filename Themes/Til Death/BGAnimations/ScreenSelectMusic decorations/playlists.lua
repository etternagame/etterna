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
		MESSAGEMAN:Broadcast("DisplayAll")
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
local rankingPage=1	
local numrankingpages = 10
local rankingWidth = frameWidth-capWideScale(15,50)
local rankingX = capWideScale(30,50)
local rankingY = capWideScale(40,40)
local rankingTitleSpacing = (rankingWidth/(#ms.SkillSets))
local buttondiffuse = 0.4
local whee

local singleplaylistactive = false
local allplaylistsactive = true

local PlaylistYspacing = 30
local playliststodisplay = 10

local row2Yoffset = 12

local pl
local keylist
local ratelist
local songlist = {}
local stepslist = {}
local chartlist = {}

local allplaylists
local playlistnames

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

local r = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(frameX,frameY)
	end,
	OnCommand=function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
	end,
	DisplayPlaylistMessageCommand=function(self)
		if update then
			pl = SONGMAN:GetActivePlaylist()
			if pl then
				singleplaylistactive = true
				allplaylistsactive = false
				
				keylist = pl:GetChartlist()
				chartlist = pl:GetChartlistActual()
				ratelist = pl:GetRatelist()
				for j=1,#keylist do
					songlist[j] = SONGMAN:GetSongByChartKey(keylist[j])
					stepslist[j] = SONGMAN:GetStepsByChartKey(keylist[j])
				end
					
				--songlist = pl:GetSonglist()
				--stepslist = pl:GetStepslist()
				self:visible(true)
				self:RunCommandsOnChildren(cmd(queuecommand, "DisplayPP"))
			else
				singleplaylistactive = false
			end
		else
			self:visible(false)
		end
	end,
	LoadFont("Common Large") .. {
		InitCommand=cmd(xy,rankingX,rankingY;zoom,0.4;halign,0;maxwidth,720),
		DisplayPlaylistMessageCommand=function(self)
			self:settext(pl:GetName())
			self:visible(true)
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(false)
			singleplaylistactive = false
			allplaylistsactive = true
		end
	}
}


local function RateDisplayButton(i)
	local o = Def.ActorFrame{
		InitCommand=cmd(x,220),
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			DisplayPlaylistCommand=function(self)
				local ratestring = string.format("%.2f", chartlist[i]:GetRate()):gsub("%.?0+$", "").."x"
				self:settext(ratestring)
				self:GetParent():queuecommand("Resize")
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=cmd(diffusealpha,buttondiffuse),
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and singleplaylistactive then
					chartlist[i]:ChangeRate(0.1)
					BroadcastIfActive("DisplayPlaylist")
				end
			end,
			MouseRightClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and singleplaylistactive then
					chartlist[i]:ChangeRate(-0.1)
					BroadcastIfActive("DisplayPlaylist")
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
			DisplayPlaylistCommand=function(self)
				self:settext(chartlist[i]:GetSongTitle())
				self:GetParent():queuecommand("Resize")
				if chartlist[i]:IsLoaded() then
					self:diffuse(getMainColor("positive"))
				else
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=cmd(diffusealpha,buttondiffuse;halign,0),
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and chartlist[i]:IsLoaded() and singleplaylistactive then
					whee:SelectSong(songlist[i])
				end
			end
		}
	}
	return o
end
	
local function rankingLabel(i)
	local chart
	local chartloaded
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY + 10 + (i-1)*scoreYspacing)
			self:RunCommandsOnChildren(cmd(halign,0;zoom,fontScale))
			self:visible(false)
		end,
		DisplayAllMessageCommand=cmd(visible,false),
		DisplayPPMessageCommand=function(self)
			if update then
				chart = chartlist[i]
				if chart then
					chartloaded = chartlist[i]:IsLoaded()
					self:visible(true)
					self:RunCommandsOnChildren(cmd(queuecommand, "DisplayPlaylist",visible,true))
				end
			else
				self:visible(true)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=cmd(maxwidth,100),
			DisplayPlaylistCommand=function(self)
				self:halign(0.5)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+i..".")
			end
		},
		LoadFont("Common Large") .. {	-- pack mouseover for later
			InitCommand=cmd(x,15;maxwidth,580),
			DisplayPlaylistCommand=function(self)
				--self:settext(songlist[i]:GetGroupName())
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(x,256;maxwidth,160),
			DisplayPlaylistCommand=function(self)
				if chartloaded then
					local rating = stepslist[i]:GetMSD(chart:GetRate(),1)
					self:settextf("%.2f", rating)
					self:diffuse(ByMSD(rating))
				else
					local rating = 0
					self:settextf("%.2f", rating)
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(x,300),
			DisplayPlaylistCommand=function(self)
				self:halign(0.5)
				local diff = chart:GetDifficulty()
				if chartloaded then
					self:diffuse(byDifficulty(diff))
					self:settext(getShortDifficulty(diff))
				else
					self:diffuse(byJudgment("TapNoteScore_Miss"))
					self:settext(getShortDifficulty(diff))
				end
			end
		}
	}
	t[#t+1] = RateDisplayButton(i)
	t[#t+1] = TitleDisplayButton(i)
	return t
end


-- Buttons for individual playlist manipulation
local b2 = Def.ActorFrame{
	InitCommand=cmd(xy,65,frameHeight-10),
	DisplayAllMessageCommand=cmd(visible,false),
	DisplayPlaylistMessageCommand=cmd(visible,true)
}

-- Add chart button
b2[#b2+1] = LoadFont("Common Large") .. {InitCommand=cmd(zoom,0.3;x,245;settext,"Add Chart")}
b2[#b2+1] = Def.Quad{
	InitCommand=cmd(x,245;diffusealpha,buttondiffuse;zoomto,80,20),
	MouseLeftClickMessageCommand=function(self)
		if ButtonActive(self) and singleplaylistactive then
			pl:AddChart(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey())
		end
	end
}
-- Queue Playlist button
b2[#b2+1] = LoadFont("Common Large") .. {InitCommand=cmd(zoom,0.3;x,130;settext,"Queue As Playlist")}
b2[#b2+1] = Def.Quad{
	InitCommand=cmd(x,130;diffusealpha,buttondiffuse;zoomto,130,20),
	MouseLeftClickMessageCommand=function(self)
		if ButtonActive(self,0.3) and singleplaylistactive then
		end
	end
}
-- Play As Course button
b2[#b2+1] = LoadFont("Common Large") .. {InitCommand=cmd(zoom,0.3;settext,"Play As Course")}
b2[#b2+1] = Def.Quad{
	InitCommand=cmd(diffusealpha,buttondiffuse;zoomto,110,20),
	MouseLeftClickMessageCommand=function(self)
		if ButtonActive(self,0.3) and singleplaylistactive then
			SCREENMAN:GetTopScreen():StartPlaylistAsCourse(pl:GetName())
		end
	end
}
r[#r+1] = b2




local function PlaylistTitleDisplayButton(i)
	local o = Def.ActorFrame{
		InitCommand=cmd(x,15),
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=cmd(halign,0;maxwidth,frameWidth * 3 + 140),
			AllDisplayCommand=function(self)
				if allplaylists[i] then
					self:settext(allplaylists[i]:GetName())
					self:GetParent():queuecommand("Resize")
				end
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=cmd(diffusealpha,buttondiffuse;halign,0),
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and allplaylistsactive then
					SONGMAN:SetActivePlaylist(allplaylists[i]:GetName())
					pl = allplaylists[i]
					MESSAGEMAN:Broadcast("DisplayPlaylist")
				end
			end
		}
	}
	return o
end

local function PlaylistSelectLabel(i)
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY + 20 + (i-1)*PlaylistYspacing)
			self:RunCommandsOnChildren(cmd(halign,0;zoom,fontScale))
			self:visible(true)
		end,
		DisplayPlaylistMessageCommand=cmd(visible,false),
		DisplayAllMessageCommand=function(self)
			if update and allplaylists[i] then				
				self:visible(true)
				self:RunCommandsOnChildren(cmd(queuecommand, "AllDisplay"))
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=cmd(maxwidth,100),
			AllDisplayCommand=function(self)
				self:halign(0.5)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+i..".")
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,15,row2Yoffset),
			AllDisplayCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settextf("Number of charts: %d", allplaylists[i]:GetNumCharts())
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,200,row2Yoffset),
			AllDisplayCommand=function(self)
				local rating = 0
				self:settextf("Average Rating:%5.2f", rating)
				self:diffuse(getMainColor("positive"))
			end
		},
	}
	t[#t+1] = PlaylistTitleDisplayButton(i)
	return t
end

local playlists = Def.ActorFrame{
	OnCommand=function(self)
		allplaylists = SONGMAN:GetPlaylists()
	end,
	DisplayAllMessageCommand=function(self)
		self:visible(true)
		allplaylists = SONGMAN:GetPlaylists()
		self:RunCommandsOnChildren(cmd(queuecommand, "Display"))
	end
}

-- Buttons for general playlist manipulation
local b = Def.ActorFrame{
	InitCommand=cmd(xy,100,frameHeight+30),
	DisplayPlaylistMessageCommand=cmd(visible,false),
	DisplayAllMessageCommand=cmd(visible,true)
}

-- New Playlist
b[#b+1] = LoadFont("Common Large") .. {InitCommand=cmd(zoom,0.3;settext,"New Playlist")}
b[#b+1] = Def.Quad{
	InitCommand=cmd(diffusealpha,buttondiffuse;zoomto,110,20),
	MouseLeftClickMessageCommand=function(self)
		if ButtonActive(self,0.3) and allplaylistsactive then
			SONGMAN:NewPlaylist()
		end
	end
}

playlists[#playlists+1] = b

for i=1,scoresperpage do
	r[#r+1] = rankingLabel(i)
end

for i=1,playliststodisplay do
	playlists[#playlists+1] = PlaylistSelectLabel(i)
end




t[#t+1] = playlists
t[#t+1] = r
return t