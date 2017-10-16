local update = false
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
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	PlayerJoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
}

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
local buttondiffuse = 0
local whee

local singleplaylistactive = false
local allplaylistsactive = true

local PlaylistYspacing = 30
local row2Yoffset = 12

local pl
local keylist
local songlist = {}
local stepslist = {}
local chartlist = {}

local currentchartpage = 1
local numchartpages
local chartsperplaylist = 25

local allplaylists
local currentplaylistpage = 1
local numplaylistpages = 1
local playlistsperpage = 10

t[#t+1] = Def.Quad{InitCommand=function(self)
	self:xy(frameX,frameY):zoomto(frameWidth,frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
end}
t[#t+1] = Def.Quad{InitCommand=function(self)
	self:xy(frameX,frameY):zoomto(frameWidth,offsetY):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(0.5)
end}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=function(self)
	self:xy(frameX+5,frameY+offsetY-9):zoom(0.6):halign(0):diffuse(getMainColor('positive')):settext("Playlists (WIP)")
end}
t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=function(self)
		self:xy(frameWidth,frameY+offsetY-9):zoom(0.6):halign(1):diffuse(getMainColor('positive'))
	end,
	DisplayPlaylistMessageCommand=function(self)
		self:settext("Ctrl+A to add a new chart")
	end,
	DisplayAllMessageCommand=function(self)
		self:settext("Ctrl+P to add a new playlist")
	end,
}

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
				for j=1,#keylist do
					songlist[j] = SONGMAN:GetSongByChartKey(keylist[j])
					stepslist[j] = SONGMAN:GetStepsByChartKey(keylist[j])
				end

				numplaylistpages = notShit.ceil(#chartlist/chartsperplaylist)

				self:visible(true)
				if not self and self.GetChildren then
					for child in self:GetChildren() do
						child:queuecommand("DisplayPP")
					end
				end
			else
				singleplaylistactive = false
			end
		else
			self:visible(false)
		end
	end,
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:xy(rankingX,rankingY):zoom(0.4):halign(0):maxwidth(360)
		end,
		DisplayPlaylistMessageCommand=function(self)
		pl = SONGMAN:GetActivePlaylist()
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
		InitCommand=function(self)
			self:x(220)
		end,
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			DisplayPlaylistCommand=function(self)
				local ratestring = string.format("%.2f", chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetRate()):gsub("%.?0+$", "").."x"
				self:settext(ratestring)
				self:GetParent():queuecommand("Resize")
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=function(self)
				self:diffusealpha(buttondiffuse)
			end,
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and singleplaylistactive then
					chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:ChangeRate(0.1)
					BroadcastIfActive("DisplayPlaylist")
				end
			end,
			MouseRightClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and singleplaylistactive then
					chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:ChangeRate(-0.1)
					BroadcastIfActive("DisplayPlaylist")
				end
			end
		}
	}
	return o
end

local function TitleDisplayButton(i)
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:x(15)
		end,
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=function(self)
				self:halign(0)
			end,
			DisplayPlaylistCommand=function(self)
				self:settext(chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetSongTitle())
				self:GetParent():queuecommand("Resize")
				if chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:IsLoaded() then
					self:diffuse(getMainColor("positive"))
				else
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=function(self)
				self:diffusealpha(buttondiffuse):halign(0)
			end,
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and chartlist[i + ((currentchartpage - 1) * chartsperplaylist)] and chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:IsLoaded() and singleplaylistactive then
					whee:SelectSong(songlist[i + ((currentchartpage - 1) * chartsperplaylist)])
				end
			end
		}
	}
	return o
end

local function DeleteChartButton(i)
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:x(315)
		end,
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=function(self)
				self:halign(0)
			end,
			DisplayPlaylistCommand=function(self)
				self:settext("Del")
				self:GetParent():queuecommand("Resize")
				self:diffuse(byJudgment("TapNoteScore_Miss"))
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=function(self)
				self:diffusealpha(buttondiffuse):halign(0)
			end,
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and singleplaylistactive then
					pl:DeleteChart(i + ((currentchartpage - 1) * chartsperplaylist))
					MESSAGEMAN:Broadcast("DisplayAll")
					MESSAGEMAN:Broadcast("DisplayPlaylist")
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
			self:visible(false)
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayPPMessageCommand=function(self)
			if update then
				chart = chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]
				if chart then
					chartloaded = chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:IsLoaded()
					self:visible(true)
					if not self and self.GetChildren then
						for child in self:GetChildren() do
							child:queuecommand("DisplayPlaylist"):visible(true)
						end
					end
				else
					if not self and self.GetChildren then
						for child in self:GetChildren() do
							child:visible(false)
						end
					end
				end
			else
				self:visible(true)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:maxwidth(100)
				self:halign(0):zoom(fontScale)
			end,
			DisplayPlaylistCommand=function(self)
				self:halign(0.5)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+ i + ((currentchartpage - 1) * chartsperplaylist.."."))
			end
		},
		LoadFont("Common Large") .. {	-- pack mouseover for later
			InitCommand=function(self)
				self:x(15):maxwidth(580)
				self:halign(0):zoom(fontScale)
			end,
			DisplayPlaylistCommand=function(self)
				--self:settext(songlist[i]:GetGroupName())
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:x(256):maxwidth(160)
				self:halign(0):zoom(fontScale)
			end,
			DisplayPlaylistCommand=function(self)
				if chartloaded then
					local rating = stepslist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetMSD(chart:GetRate(),1)
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
			InitCommand=function(self)
				self:x(300)
				self:halign(0):zoom(fontScale)
			end,
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
	t[#t+1] = DeleteChartButton(i)
	return t
end


-- Buttons for individual playlist manipulation
local b2 = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(215,rankingY)
	end,
	DisplayAllMessageCommand=function(self)
		self:visible(false)
	end,
	DisplayPlaylistMessageCommand=function(self)
		self:visible(true)
	end	
}

--Add chart button
-- b2[#b2+1] = LoadFont("Common Large") .. {InitCommand=cmd(zoom,0.3;x,245;settext,"Add Chart")}
-- b2[#b2+1] = Def.Quad{
	-- InitCommand=function(self)
	-- 	self:x(245):diffusealpha(buttondiffuse):zoomto(80,20)
	-- end,
	-- MouseLeftClickMessageCommand=function(self)
		-- if ButtonActive(self) and singleplaylistactive then
			-- pl:AddChart(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey())
		-- end
	-- end
-- }
-- Play As Course button
b2[#b2+1] = LoadFont("Common Large") .. {InitCommand=function(self)
	self:zoom(0.3):x(85):settext("Play As Course")
end}
b2[#b2+1] = Def.Quad{
	InitCommand=function(self)
		self:x(85):diffusealpha(buttondiffuse):zoomto(110,20)
	end,
	MouseLeftClickMessageCommand=function(self)
		if ButtonActive(self,0.3) and singleplaylistactive then
			SCREENMAN:GetTopScreen():StartPlaylistAsCourse(pl:GetName())
		end
	end
}

-- Back button
b2[#b2+1] = LoadFont("Common Large") .. {InitCommand=function(self)
	self:zoom(0.3):settext("Back")
end}
b2[#b2+1] = Def.Quad{
	InitCommand=function(self)
		self:diffusealpha(buttondiffuse):zoomto(110,20)
	end,
	MouseLeftClickMessageCommand=function(self)
		if ButtonActive(self,0.3) and singleplaylistactive then
			MESSAGEMAN:Broadcast("DisplayAll")
		end
	end
}
r[#r+1] = b2

-- next/prev pages for individual playlists, i guess these could be merged with the allplaylists buttons for efficiency but meh
r[#r+1] = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(frameX+10,frameY+rankingY+250)
	end,
	Def.Quad{
		InitCommand=function(self)
			self:xy(300,-8):zoomto(40,20):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and currentchartpage < numplaylistpages and singleplaylistactive then
				currentchartpage = currentchartpage + 1
				MESSAGEMAN:Broadcast("DisplayPlaylist")
				MESSAGEMAN:Broadcast("DisplayPP")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:x(300):halign(0):zoom(0.3):diffuse(getMainColor('positive')):settext("Next")
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(true)
		end	
	},
	Def.Quad{
		InitCommand=function(self)
			self:y(-8):zoomto(65,20):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and currentchartpage > 1 and singleplaylistactive then
				currentchartpage = currentchartpage - 1
				MESSAGEMAN:Broadcast("DisplayPlaylist")
				MESSAGEMAN:Broadcast("DisplayPP")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:halign(0):zoom(0.3):diffuse(getMainColor('positive')):settext("Previous")
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(true)
		end	
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:x(175):halign(0.5):zoom(0.3):diffuse(getMainColor('positive'))
		end,
		SetCommand=function(self)
			self:settextf("Showing %i-%i of %i", math.min(((currentchartpage-1)*chartsperplaylist)+1, #chartlist), math.min(currentchartpage*chartsperplaylist, #chartlist), #chartlist)
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(true):queuecommand("Set")
		end	
	}
}










local function PlaylistTitleDisplayButton(i)
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:x(15)
		end,
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=function(self)
				self:halign(0):maxwidth(frameWidth * 3 + 140)
			end,
			AllDisplayCommand=function(self)
				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					self:settext(allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetName())
					self:GetParent():queuecommand("Resize")
				end
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=function(self)
				self:diffusealpha(buttondiffuse):halign(0)
			end,
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and allplaylistsactive then
					SONGMAN:SetActivePlaylist(allplaylists[i]:GetName())
					pl = allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]
					MESSAGEMAN:Broadcast("DisplayPlaylist")
				end
			end
		}
	}
	return o
end

local function DeletePlaylistButton(i)
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:x(315)
		end,
		ResizeCommand=function(self)
			self:GetChild("Button"):zoomto(self:GetChild("Text"):GetZoomedWidth(),self:GetChild("Text"):GetZoomedHeight())
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=function(self)
				self:halign(0):maxwidth(frameWidth * 3 + 140)
			end,
			AllDisplayCommand=function(self)
				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					self:settext("Del")
					self:GetParent():queuecommand("Resize")
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=function(self)
				self:diffusealpha(buttondiffuse):halign(0)
			end,
			MouseLeftClickMessageCommand=function(self)
				if ButtonActive(self,fontScale) and allplaylistsactive then
					SONGMAN:DeletePlaylist(allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetName())
					MESSAGEMAN:Broadcast("DisplayAll")
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
			self:visible(true)
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayAllMessageCommand=function(self)
			if update and allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then				
				self:visible(true)
				if not self and self.GetChildren then
					for child in self:GetChildren() do
						child:queuecommand("AllDisplay")
					end
				end
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
				self:maxwidth(100)
			end,
			AllDisplayCommand=function(self)
				self:halign(0.5)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+i + ((currentplaylistpage - 1) * playlistsperpage)..".")
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
				self:xy(15,row2Yoffset)
			end,
			AllDisplayCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settextf("Number of charts: %d", allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetNumCharts())
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
				self:xy(200,row2Yoffset)
			end,
			AllDisplayCommand=function(self)
				self:settextf("Average Rating:")
				self:diffuse(getMainColor("positive"))
			end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
				self:xy(295,row2Yoffset)
			end,
			AllDisplayCommand=function(self)
				local rating = allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetAverageRating()
				self:settextf("%.2f", rating)
				self:diffuse(ByMSD(rating))
			end
		},
	}
	t[#t+1] = PlaylistTitleDisplayButton(i)
	t[#t+1] = DeletePlaylistButton(i)
	return t
end

local playlists = Def.ActorFrame{
	OnCommand=function(self)
		allplaylists = SONGMAN:GetPlaylists()
		numplaylistpages = notShit.ceil(#allplaylists/playlistsperpage)
	end,
	DisplayAllMessageCommand=function(self)
		self:visible(true)
		allplaylists = SONGMAN:GetPlaylists()
		numplaylistpages = notShit.ceil(#allplaylists/playlistsperpage)
		if not self and self.GetChildren then
			for child in self:GetChildren() do
				child:queuecommand("Display")
			end
		end
	end
}

-- Buttons for general playlist manipulation
local b = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(100,frameHeight+30)
	end,
	DisplayPlaylistMessageCommand=function(self)
		self:visible(false)
	end,
	DisplayAllMessageCommand=function(self)
		self:visible(true)
	end	
}

-- zzzz button positioning is lame... use shortcut key for now whynot
-- New Playlist
-- b[#b+1] = LoadFont("Common Large") .. {InitCommand=cmd(zoom,0.3;settext,"New Playlist")}
-- b[#b+1] = Def.Quad{
	-- InitCommand=function(self)
	-- 	self:diffusealpha(buttondiffuse):zoomto(110,20)
	-- end,
	-- MouseLeftClickMessageCommand=function(self)
		-- if ButtonActive(self,0.3) and allplaylistsactive then
			-- SONGMAN:NewPlaylist()
			-- MESSAGEMAN:Broadcast("DisplayAll")
		-- end
	-- end
-- }

playlists[#playlists+1] = b

for i=1,chartsperplaylist do
	r[#r+1] = rankingLabel(i)
end

for i=1,playlistsperpage do
	playlists[#playlists+1] = PlaylistSelectLabel(i)
end


-- next/prev for all playlists
r[#r+1] = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(frameX+10,frameY+rankingY+250)
	end,
	Def.Quad{
		InitCommand=function(self)
			self:xy(300,-8):zoomto(40,20):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and currentplaylistpage < numplaylistpages and allplaylistsactive then
				currentplaylistpage = currentplaylistpage + 1
				MESSAGEMAN:Broadcast("DisplayAll")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:x(300):halign(0):zoom(0.3):diffuse(getMainColor('positive')):settext("Next")
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(true)
		end	
	},
	Def.Quad{
		InitCommand=function(self)
			self:y(-8):zoomto(65,20):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and currentplaylistpage > 1 and allplaylistsactive then
				currentplaylistpage = currentplaylistpage - 1
				MESSAGEMAN:Broadcast("DisplayAll")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:halign(0):zoom(0.3):diffuse(getMainColor('positive')):settext("Previous")
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(false)
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(true)
		end	
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:x(175):halign(0.5):zoom(0.3):diffuse(getMainColor('positive'))
		end,
		SetCommand=function(self)
			self:settextf("Showing %i-%i of %i", math.min(((currentplaylistpage-1)*playlistsperpage)+1, #allplaylists), math.min(currentplaylistpage*playlistsperpage, #allplaylists), #allplaylists)
		end,
		DisplayAllMessageCommand=function(self)
			self:visible(true):queuecommand("Set")
		end,
		DisplayPlaylistMessageCommand=function(self)
			self:visible(false)
		end	
	}
}



t[#t+1] = playlists
t[#t+1] = r
return t