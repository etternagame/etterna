local hoverAlpha = 0.6

local update = false
local clickedForSinglePlaylist = false
local t = Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
		self:sleep(0.04):queuecommand("Invis")
	end,
	InvisCommand= function(self)
		self:visible(false)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	SetCommand = function(self)
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
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(360, 400)
local frameHeight = 350
local fontScale = 0.25

local scoreYspacing = 13
local distY = 15
local offsetX = -10
local offsetY = 20
local rankingPage = 1
local rankingWidth = frameWidth - capWideScale(15, 50)
local rankingX = capWideScale(30, 50)
local rankingY = capWideScale(40, 40)
local rankingTitleSpacing = (rankingWidth / (#ms.SkillSets))
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
local chartsperplaylist = 20

local allplaylists
local currentplaylistpage = 1
local numplaylistpages = 1
local playlistsperpage = 10

local translated_info = {
	Delete = THEME:GetString("TabPlaylists", "Delete"),
	Showing = THEME:GetString("TabPlaylists", "Showing"),
	ChartCount = THEME:GetString("TabPlaylists", "ChartCount"),
	AverageRating = THEME:GetString("TabPlaylists", "AverageRating"),
	Title = THEME:GetString("TabPlaylists", "Title"),
	ExplainAdd = THEME:GetString("TabPlaylists", "ExplainAddChart"),
	ExplainPlaylist = THEME:GetString("TabPlaylists", "ExplainNewPlaylist"),
	PlayAsCourse = THEME:GetString("TabPlaylists", "PlayAsCourse"),
	Back = THEME:GetString("TabPlaylists", "Back"),
	Next = THEME:GetString("TabPlaylists", "Next"),
	Previous = THEME:GetString("TabPlaylists", "Previous"),
}

t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(getMainColor("tabs"))
	end
}
t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, offsetY):halign(0):valign(0)
		self:diffuse(getMainColor("frames")):diffusealpha(0.5)
	end
}
t[#t + 1] = LoadFont("Common Normal") .. {
	InitCommand = function(self)
		self:xy(frameX + 5, frameY + offsetY - 11):zoom(0.65):halign(0)
		self:diffuse(Saturation(getMainColor("positive"), 0.1))
		self:settext(translated_info["Title"])
	end
}
t[#t + 1] = LoadFont("Common Normal") .. {
	InitCommand = function(self)
		self:xy(frameWidth, frameY + offsetY - 11):zoom(0.65):halign(1)
	end,
	DisplaySinglePlaylistMessageCommand = function(self)
		self:settext(translated_info["ExplainAdd"])
	end,
	DisplayAllMessageCommand = function(self)
		self:settext(translated_info["ExplainPlaylist"])
	end
}

local function BroadcastIfActive(msg)
	if update then
		MESSAGEMAN:Broadcast(msg)
	end
end

local function ButtonActive(self)
	return isOver(self) and update
end

local r = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX, frameY)
	end,
	OnCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
	end,
	DisplaySinglePlaylistMessageCommand = function(self)
		if getTabIndex() ~= 7 then return end
		if update then
			pl = SONGMAN:GetActivePlaylist()
			if pl then
				singleplaylistactive = true
				allplaylistsactive = false

				keylist = pl:GetChartkeys()
				chartlist = pl:GetAllSteps()
				for j = 1, #keylist do
					songlist[j] = SONGMAN:GetSongByChartKey(keylist[j])
					stepslist[j] = SONGMAN:GetStepsByChartKey(keylist[j])
				end

				numplaylistpages = notShit.ceil(#chartlist / chartsperplaylist)

				self:visible(true)
				MESSAGEMAN:Broadcast("DisplayPP")
			else
				singleplaylistactive = false
			end
		else
			self:visible(false)
		end
	end,
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(frameX, rankingY):zoom(0.4):halign(0):maxwidth(460)
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			pl = SONGMAN:GetActivePlaylist()
			self:settext(pl:GetName())
			self:visible(true)
		end,
		DisplayAllMessageCommand = function(self)
			if getTabIndex() == 7 then
				self:visible(false)
				singleplaylistactive = false
				allplaylistsactive = true
			end
		end
	}
}

local function RateDisplayButton(i)
	local o = Def.ActorFrame {
		Name = "RateDisplay",
		InitCommand = function(self)
			self:x(220):diffuse(getMainColor("positive"))
		end,
		UIElements.TextToolTip(1, 1, "Common Large") .. {
			Name = "Text",
			DisplaySinglePlaylistLevel2MessageCommand = function(self)
				local ratestring =
					string.format("%.2f", chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetRate()):gsub("%.?0+$", "") ..
					"x"
				self:settext(ratestring)
				self:zoom(fontScale)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and update and singleplaylistactive then
					chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:ChangeRate(0.1)
					BroadcastIfActive("DisplaySinglePlaylist")
				elseif params.event == "DeviceButton_right mouse button" and update and singleplaylistactive then
					chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:ChangeRate(-0.1)
					BroadcastIfActive("DisplaySinglePlaylist")
				end
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
		}
	}
	return o
end

local function TitleDisplayButton(i)
	local o = Def.ActorFrame {
		Name = "TitleDisplay",
		InitCommand = function(self)
			self:x(15)
		end,
		UIElements.QuadButton(1, 1) .. {
			InitCommand = function(self)
				self:x(-22):zoomto(212, scoreYspacing):halign(0):diffusealpha(0)
			end,
			MouseDownCommand = function(self, params)
				-- wtf
				if params.event == "DeviceButton_left mouse button" and update and chartlist[i + ((currentchartpage - 1) * chartsperplaylist)] and
						chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:IsLoaded() and
						singleplaylistactive and not clickedForSinglePlaylist
				 then
					whee:SelectSong(songlist[i + ((currentchartpage - 1) * chartsperplaylist)])
				end
			end,
			MouseOverCommand = function(self)
				self:GetParent():GetChild("Text"):diffusealpha(0.7)
			end,
			MouseOutCommand = function(self)
				self:GetParent():GetChild("Text"):diffusealpha(1)
			end,
		},
		LoadFont("Common Large") .. {
			Name = "Text",
			InitCommand = function(self)
				self:halign(0)
			end,
			DisplaySinglePlaylistLevel2MessageCommand = function(self)
				self:zoom(fontScale)
				self:maxwidth(480)
				local chartentry = chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]
				if chartentry == nil then return end
				if chartentry:IsLoaded() then
					local songentry = songlist[i + ((currentchartpage - 1) * chartsperplaylist)]
					self:diffuse(getMainColor("positive"))
					self:settext(songentry:GetDisplayMainTitle())
				else
					self:diffuse(byJudgment("TapNoteScore_Miss"))
					self:settext(chartentry:GetSongTitle())
				end
			end,
			DisplayLanguageChangedMessageCommand = function(self)
				self:playcommand("DisplaySinglePlaylistLevel2")
			end,
		}
	}
	return o
end

local function DeleteChartButton(i)
	local o = Def.ActorFrame {
		Name = "DeleteButton",
		InitCommand = function(self)
			self:x(315)
		end,
		UIElements.TextToolTip(1, 1, "Common Large") .. {
			Name = "Text",
			InitCommand = function(self)
				self:halign(0)
				self:zoom(fontScale)
				self:settext(translated_info["Delete"])
				self:diffuse(byJudgment("TapNoteScore_Miss"))
			end,
			DisplaySinglePlaylistLevel2Command = function(self)
				if pl:GetName() == "Favorites" then
					self:visible(false)
				else
					self:visible(true)
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and update and singleplaylistactive then
					pl:DeleteChart(i + ((currentchartpage - 1) * chartsperplaylist))
					MESSAGEMAN:Broadcast("DisplayAll")
					MESSAGEMAN:Broadcast("DisplaySinglePlaylist")
				end
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
		}
	}
	return o
end

local function rankingLabel(i)
	local chart
	local chartloaded
	local t = Def.ActorFrame {
		InitCommand = function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY + 10 + (i - 1) * scoreYspacing)
			self:visible(false)
		end,
		DisplayAllMessageCommand = function(self)
			self:visible(false)
		end,
		DisplayPPMessageCommand = function(self)
			if update then
				chart = chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]
				if chart then
					chartloaded = chartlist[i + ((currentchartpage - 1) * chartsperplaylist)]:IsLoaded()
					self:visible(true)
					self:GetChild("DeleteButton"):queuecommand("DisplaySinglePlaylistLevel2")
					self:GetChild("TitleDisplay"):queuecommand("DisplaySinglePlaylistLevel2")
					self:GetChild("RateDisplay"):queuecommand("DisplaySinglePlaylistLevel2")
					self:GetChild("DeleteButton"):visible(true)
					self:GetChild("TitleDisplay"):visible(true)
					self:GetChild("RateDisplay"):visible(true)
					self:GetChild("PackMouseOver"):visible(true)
					self:GetChild("ChartNumber"):visible(true)
				else
					self:GetChild("DeleteButton"):visible(false)
					self:GetChild("TitleDisplay"):visible(false)
					self:GetChild("RateDisplay"):visible(false)
					self:GetChild("PackMouseOver"):visible(false)
					self:GetChild("ChartNumber"):visible(false)
				end
			else
				self:visible(true)
			end
		end,
		LoadFont("Common Large") .. {
			Name = "ChartNumber",
			InitCommand = function(self)
				self:maxwidth(100)
				self:halign(0):zoom(fontScale)
			end,
			DisplayPPMessageCommand = function(self)
				self:halign(0.5)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage - 1) * chartsperplaylist) + i + ((currentchartpage - 1) * chartsperplaylist) .. ".")
			end
		},
		Def.ActorFrame {
			Name = "PackMouseOver",
			UIElements.QuadButton(1, 1) .. {
				InitCommand = function(self)
					Name = "mouseover",
					self:x(-7):zoomto(212, scoreYspacing):halign(0):diffusealpha(0)
				end,
				MouseOverCommand = function(self)
					self:GetParent():queuecommand("DisplayPack")
				end,
				MouseOutCommand = function(self)
					self:GetParent():queuecommand("UNDisplayPack")
				end,
			},
			Def.ActorFrame {
				Name = "mouseovertextcontainer",
				InitCommand = function(self)
					self:xy(15, -12)
				end,
				DisplayPackCommand = function(self)
					if songlist[i + ((currentchartpage - 1) * chartsperplaylist)] then
						local txt = self:GetChild("text")
						local bg = self:GetChild("BG")
						txt:settext(songlist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetGroupName())
						bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * 1.4)
						self:finishtweening()
						self:diffusealpha(1)
					end
				end,
				UNDisplayPackCommand = function(self)
					if songlist[i + ((currentchartpage - 1) * chartsperplaylist)] then
						local txt = self:GetChild("text")
						local bg = self:GetChild("BG")
						txt:settext(songlist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetGroupName())
						bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * 1.4)
						self:linear(0.25)
						self:diffusealpha(0)
					end
				end,
				Def.Quad {
					Name = "BG",
					InitCommand = function(self)
						self:halign(0)
						self:diffuse(color("0,0,0,0.6"))
					end,
				},
				LoadFont("Common Large") .. {
					Name = "text",
					InitCommand = function(self)
						self:maxwidth(580)
						self:halign(0)
						self:zoom(fontScale)
					end,
				},
			},
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(256):maxwidth(160)
				self:halign(0):zoom(fontScale)
			end,
			DisplaySinglePlaylistLevel2MessageCommand = function(self)
				if chartloaded then
					local rating = stepslist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetMSD(chart:GetRate(), 1)
					self:settextf("%.2f", rating)
					self:diffuse(byMSD(rating))
				else
					local rating = 0
					self:settextf("%.2f", rating)
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(300)
				self:halign(0):zoom(fontScale)
			end,
			DisplaySinglePlaylistLevel2MessageCommand = function(self)
				self:halign(0.5)
				local diff = stepslist[i + ((currentchartpage - 1) * chartsperplaylist)]:GetDifficulty()
				if chartloaded then
					self:diffuse(byDifficulty(diff))
					self:settext(getShortDifficulty(diff))
				else
					local diff = chart:GetDifficulty()
					self:diffuse(byJudgment("TapNoteScore_Miss"))
					self:settext(getShortDifficulty(diff))
				end
			end
		}
	}
	t[#t + 1] = RateDisplayButton(i)
	t[#t + 1] = TitleDisplayButton(i)
	t[#t + 1] = DeleteChartButton(i)
	return t
end

-- Buttons for individual playlist manipulation
local b2 = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(215, rankingY)
	end,
	DisplayAllMessageCommand = function(self)
		self:visible(false)
	end,
	DisplaySinglePlaylistMessageCommand = function(self)
		self:visible(true)
	end
}

b2[#b2 + 1] = UIElements.TextToolTip(1, 1, "Common Large") .. {
	InitCommand = function(self)
		self:zoom(0.3):x(capWideScale(86,107)):diffuse(getMainColor("positive"))
		self:settext(translated_info["PlayAsCourse"])
	end,
	MouseDownCommand = function(self, params)
		if params.event == "DeviceButton_left mouse button" and update and singleplaylistactive then
			SCREENMAN:GetTopScreen():StartPlaylistAsCourse(pl:GetName())
		end
	end,
	MouseOverCommand = function(self)
		self:diffusealpha(hoverAlpha)
	end,
	MouseOutCommand = function(self)
		self:diffusealpha(1)
	end,
}

-- Back button
b2[#b2 + 1] = UIElements.TextToolTip(1, 1, "Common Large") .. {
	InitCommand = function(self)
		self:zoom(0.3):x(capWideScale(5,20)):diffuse(getMainColor("positive"))
		self:settext(translated_info["Back"])
	end,
	MouseDownCommand = function(self, params)
		if params.event == "DeviceButton_left mouse button" and update and singleplaylistactive then
			MESSAGEMAN:Broadcast("DisplayAll")
		end
	end,
	MouseOverCommand = function(self)
		self:diffusealpha(hoverAlpha)
	end,
	MouseOutCommand = function(self)
		self:diffusealpha(1)
	end,
}

r[#r + 1] = b2

-- next/prev pages for individual playlists, i guess these could be merged with the allplaylists buttons for efficiency but meh
r[#r + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX + 10, frameY + rankingY + 250)
	end,
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:x(capWideScale(280,300)):halign(0):zoom(0.3):diffuse(getMainColor("positive"))
			self:settext(translated_info["Next"])
		end,
		DisplayAllMessageCommand = function(self)
			self:visible(false)
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			self:visible(true)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and currentchartpage < numplaylistpages and singleplaylistactive then
				currentchartpage = currentchartpage + 1
				MESSAGEMAN:Broadcast("DisplaySinglePlaylist")
				MESSAGEMAN:Broadcast("DisplayPP")
			end
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:halign(0):zoom(0.3):diffuse(getMainColor("positive"))
			self:settext(translated_info["Previous"])
		end,
		DisplayAllMessageCommand = function(self)
			self:visible(false)
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			self:visible(true)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and currentchartpage > 1 and singleplaylistactive then
				currentchartpage = currentchartpage - 1
				MESSAGEMAN:Broadcast("DisplaySinglePlaylist")
				MESSAGEMAN:Broadcast("DisplayPP")
			end
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:x(175):halign(0.5):zoom(0.3)
		end,
		SetCommand = function(self)
			self:settextf(
				"%s %i-%i (%i)",
				translated_info["Showing"],
				math.min(((currentchartpage - 1) * chartsperplaylist) + 1, #chartlist),
				math.min(currentchartpage * chartsperplaylist, #chartlist),
				#chartlist
			)
		end,
		DisplayAllMessageCommand = function(self)
			self:visible(false)
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			self:visible(true):queuecommand("Set")
		end
	}
}

local function PlaylistTitleDisplayButton(i)
	local o = Def.ActorFrame {
		InitCommand = function(self)
			self:x(15)
		end,
		UIElements.QuadButton(1, 1) .. {
			InitCommand = function(self)
				self:xy(-21,-5):zoomto(rankingWidth - 30, scoreYspacing * 2.25):align(0,0)
				self:diffusealpha(0)
			end,
			MouseOverCommand = function(self)
				self:GetParent():GetChild("Text"):diffusealpha(0.7)
			end,
			MouseOutCommand = function(self)
				self:GetParent():GetChild("Text"):diffusealpha(1)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and update and allplaylistsactive then
					SONGMAN:SetActivePlaylist(allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetName())
					pl = allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]
					MESSAGEMAN:Broadcast("DisplaySinglePlaylist")
					clickedForSinglePlaylist = true
				end
			end,
			MouseUpMessageCommand = function(self)
				clickedForSinglePlaylist = false
			end,
		},
		LoadFont("Common Large") .. {
			Name = "Text",
			InitCommand = function(self)
				self:halign(0):maxwidth(frameWidth * 3 + 140)
				self:diffuse(getMainColor("positive"))
			end,
			AllDisplayMessageCommand = function(self)
				self:zoom(fontScale)
				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					self:settext(allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetName())
				end
			end,
		}
	}
	return o
end

local function DeletePlaylistButton(i)
	local o = Def.ActorFrame {
		InitCommand = function(self)
			self:x(315)
		end,
		UIElements.TextToolTip(1, 1, "Common Large") .. {
			Name = "Text",
			InitCommand = function(self)
				self:halign(0):maxwidth(frameWidth * 3 + 140)
			end,
			AllDisplayMessageCommand = function(self)
				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					self:settext(translated_info["Delete"])
					self:zoom(fontScale)
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end

				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetName() == "Favorites" then
						self:visible(false)
					else
						self:visible(true)
					end
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and update and allplaylistsactive then
					SONGMAN:DeletePlaylist(allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetName())
					allplaylists = SONGMAN:GetPlaylists()
					numplaylistpages = notShit.ceil(#allplaylists / playlistsperpage)
					MESSAGEMAN:Broadcast("DisplayAll")
				end
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
		}
	}
	return o
end

local function PlaylistSelectLabel(i)
	local t = Def.ActorFrame {
		InitCommand = function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY + 20 + (i - 1) * PlaylistYspacing)
			self:visible(true)
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			self:visible(false)
		end,
		DisplayAllMessageCommand = function(self)
			if update and allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
				self:visible(true)
				MESSAGEMAN:Broadcast("AllDisplay")
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:halign(0):zoom(fontScale)
				self:maxwidth(100)
			end,
			AllDisplayMessageCommand = function(self)
				self:halign(0.5)
				self:settext(((rankingPage - 1) * chartsperplaylist) + i + ((currentplaylistpage - 1) * playlistsperpage) .. ".")
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:halign(0):zoom(fontScale)
				self:xy(15, row2Yoffset)
			end,
			AllDisplayMessageCommand = function(self)
				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					self:settextf(
						"%s: %d",
						translated_info["ChartCount"],
						allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetNumCharts()
					)
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:halign(0):zoom(fontScale)
				self:xy(200, row2Yoffset)
			end,
			AllDisplayMessageCommand = function(self)
				self:settextf("%s:", translated_info["AverageRating"])
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:halign(0):zoom(fontScale)
				self:xy(295, row2Yoffset)
			end,
			AllDisplayMessageCommand = function(self)
				if allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)] then
					local rating = allplaylists[i + ((currentplaylistpage - 1) * playlistsperpage)]:GetAverageRating()
					self:settextf("%.2f", rating)
					self:diffuse(byMSD(rating))
				end
			end
		}
	}
	t[#t + 1] = PlaylistTitleDisplayButton(i)
	t[#t + 1] = DeletePlaylistButton(i)
	return t
end

local playlists = Def.ActorFrame {
	OnCommand = function(self)
		allplaylists = SONGMAN:GetPlaylists()
		numplaylistpages = notShit.ceil(#allplaylists / playlistsperpage)
	end,
	DisplayAllMessageCommand = function(self)
		self:visible(true)
		allplaylists = SONGMAN:GetPlaylists()
		numplaylistpages = notShit.ceil(#allplaylists / playlistsperpage)
	end
}

-- Buttons for general playlist manipulation
local b = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(100, frameHeight + 30)
	end,
	DisplaySinglePlaylistMessageCommand = function(self)
		self:visible(false)
	end,
	DisplayAllMessageCommand = function(self)
		self:visible(true)
	end
}

playlists[#playlists + 1] = b

for i = 1, chartsperplaylist do
	r[#r + 1] = rankingLabel(i)
end

for i = 1, playlistsperpage do
	playlists[#playlists + 1] = PlaylistSelectLabel(i)
end

-- next/prev for all playlists
r[#r + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX + 10, frameY + rankingY + 250)
	end,
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:x(capWideScale(280,300)):halign(0):zoom(0.3):diffuse(getMainColor("positive"))
			self:settext(translated_info["Next"])
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			if update then
				self:visible(false)
			end
		end,
		DisplayAllMessageCommand = function(self)
			if update then
				self:visible(true)
			end
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and currentplaylistpage < numplaylistpages and allplaylistsactive then
				currentplaylistpage = currentplaylistpage + 1
				MESSAGEMAN:Broadcast("DisplayAll")
			end
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:halign(0):zoom(0.3):diffuse(getMainColor("positive"))
			self:settext(translated_info["Previous"])
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			self:visible(false)
		end,
		DisplayAllMessageCommand = function(self)
			self:visible(true)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and currentplaylistpage > 1 and allplaylistsactive then
				currentplaylistpage = currentplaylistpage - 1
				MESSAGEMAN:Broadcast("DisplayAll")
			end
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:x(175):halign(0.5):zoom(0.3)
		end,
		SetCommand = function(self)
			self:settextf(
				"%s %i-%i (%i)",
				translated_info["Showing"],
				math.min(((currentplaylistpage - 1) * playlistsperpage) + 1, #allplaylists),
				math.min(currentplaylistpage * playlistsperpage, #allplaylists),
				#allplaylists
			)
		end,
		DisplayAllMessageCommand = function(self)
			self:visible(true):queuecommand("Set")
		end,
		DisplaySinglePlaylistMessageCommand = function(self)
			self:visible(false)
		end
	}
}

t[#t + 1] = playlists
t[#t + 1] = r
return t
