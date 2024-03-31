local tzoom = 0.5
local pdh = 42 * tzoom
local ygap = 2
local packspaceY = pdh + ygap

local numpacks = 15
local offx = 5
local width = SCREEN_WIDTH * 0.6
local dwidth = width - offx * 2
local height = (numpacks + 2) * packspaceY

local adjx = 14
local c1x = 10
local c2x = c1x + (tzoom * 5 * adjx) -- guesswork adjustment for epxected text length
local c6x = dwidth -- right aligned cols
local c5x = c6x - adjx - (tzoom * 7 * adjx) -- right aligned cols
local c4x = c5x - adjx - (tzoom * 4 * adjx) -- right aligned cols
local c3x = c4x - adjx - (tzoom * 6.6 * adjx) -- right aligned cols
local c2xc3x = (c3x - adjx - (tzoom * 6 * adjx))
local headeroff = packspaceY / 1.5

local hoverAlpha = 0.6

local translated_info = {
	Name = THEME:GetString("PacklistDisplay", "Name"),
	AverageDiff = THEME:GetString("PacklistDisplay", "AverageDiff"),
	Size = THEME:GetString("PacklistDisplay", "Size"),
	Installed = THEME:GetString("PacklistDisplay", "Installed"),
	Download = THEME:GetString("PacklistDisplay", "Download"),
	Mirror = THEME:GetString("PacklistDisplay", "Mirror"),
	MB = THEME:GetString("PacklistDisplay", "MB"),
	AwaitingRequest = THEME:GetString("PacklistDisplay", "AwaitingRequest"),
	NoPacks = THEME:GetString("PacklistDisplay", "NoPacks"),
	PackPlays = THEME:GetString("PacklistDisplay", "PackPlays"),
	SongCount = THEME:GetString("PacklistDisplay", "SongCount"),
}

-- initialize the base pack search
local packlist = PackList:new()
packlist:FilterAndSearch("", {}, numpacks)

local o = Def.ActorFrame {
	Name = "PacklistDisplay",
	InitCommand = function(self)
		self:xy(0, 0)
	end,
	BeginCommand = function(self)
		self:queuecommand("PackTableRefresh")
	end,
	InvokePackSearchMessageCommand = function(self, params)
		packlist:FilterAndSearch(params.name, params.tags, numpacks)
		self:queuecommand("Update")
	end,
	PackTableRefreshCommand = function(self)
		self:queuecommand("Update")
	end,
	UpdateCommand = function(self)

	end,
	DFRFinishedMessageCommand = function(self)
		self:queuecommand("Update")
	end,
	PackListRequestFinishedMessageCommand = function(self, params)
		self:queuecommand("Update")
	end,
	NextPageCommand = function(self)
		self:queuecommand("Update")
	end,
	PrevPageCommand = function(self)
		self:queuecommand("Update")
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:zoomto(width, height - headeroff)
			self:halign(0):valign(0)
			self:diffuse(color("#888888"))
		end
	},
	-- headers
	Def.Quad {
		Name = "HeaderBG",
		InitCommand = function(self)
			self:xy(offx, headeroff)
			self:zoomto(dwidth, pdh)
			self:halign(0)
			self:diffuse(color("#333333"))
		end
	},
	LoadFont("Common Normal") .. {
		Name = "TotalPacks",
		InitCommand = function(self)
			self:xy(c1x, headeroff)
			self:zoom(tzoom)
			self:halign(0)
		end,
		UpdateCommand = function(self)
			self:settext(packlist:GetTotalResults())
		end
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "NameHeader",
		InitCommand = function(self)
			self:xy(c2x, headeroff)
			self:zoom(tzoom)
			self:halign(0)
			self:settext(translated_info["Name"])
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "AverageDiffHeader",
		InitCommand = function(self)
			self:xy(c2xc3x, headeroff)
			self:zoom(tzoom)
			self:halign(1)
			self:settext(translated_info["AverageDiff"])
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "SizeHeader",
		InitCommand = function(self)
			self:xy(c3x, headeroff)
			self:zoom(tzoom)
			self:halign(1)
			self:settext(translated_info["Size"])
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "PlaysHeader",
		InitCommand = function(self)
			self:xy(c4x, headeroff)
			self:zoom(tzoom)
			self:halign(1)
			self:settext(translated_info["PackPlays"])
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "SongCountHeader",
		InitCommand = function(self)
			self:xy(c5x, headeroff)
			self:zoom(tzoom)
			self:halign(1)
			self:settext(translated_info["SongCount"])
		end,
	},
	LoadFont("Common Large") .. {
		Name = "AwaitingOrNoResults",
		InitCommand = function(self)
			self:xy(width/2, height/2)
			self:zoom(tzoom)
			self:maxwidth(width / tzoom)
		end,
		UpdateCommand = function(self)
			if packlist:IsAwaitingRequest() then
				self:settext(translated_info["AwaitingRequest"])
				self:visible(true)
			else
				if packlist:GetTotalResults() == 0 then
					self:visible(true)
					self:settext(translated_info["NoPacks"])
				else
					self:visible(false)
				end
			end
		end,
	},
}

local function makePackDisplay(i)
	local packinfo
	local installed
	local o = Def.ActorFrame {
		Name = "Pack"..i,
		InitCommand = function(self)
			self:y(packspaceY * i + headeroff)
		end,
		UpdateCommand = function(self)
			if packlist:IsAwaitingRequest() then
				self:visible(false)
				return
			end

			packinfo = packlist:GetPacks()[i]
			if packinfo then
				installed = SONGMAN:DoesSongGroupExist(packinfo:GetName())
				self:queuecommand("Display")
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		Def.Quad {
			Name = "BG",
			InitCommand = function(self)
				self:x(offx)
				self:zoomto(dwidth, pdh)
				self:halign(0)
			end,
			DisplayCommand = function(self)
				if installed then
					self:diffuse(color("#444444CC"))
				else
					self:diffuse(color("#111111CC"))
				end
			end
		},
		LoadFont("Common normal") .. {
			Name = "PackIndex",
			InitCommand = function(self)
				self:x(c1x)
				self:zoom(tzoom)
				self:halign(0)
			end,
			DisplayCommand = function(self)
				self:settextf("%i.", i + ((packlist:GetCurrentPage()-1) * numpacks))
			end
		},
		UIElements.TextToolTip(1, 1, "Common normal") .. {
			Name = "PackName",
			InitCommand = function(self)
				self:x(c2x)
				self:zoom(tzoom)

				 -- x of left aligned col 2 minus x of right aligned col 3 minus roughly how wide column 3 is plus margin
				self:maxwidth((c2xc3x - c2x - c2x*0.8) / tzoom)
				self:halign(0)
			end,
			DisplayCommand = function(self)
				self:settext(packinfo:GetName())
				self:diffuse(bySkillRange(packinfo:GetAvgDifficulty()))
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" then
					local urlstringyo = "https://etternaonline.com/pack/" .. packinfo:GetID() -- not correct value for site id
					GAMESTATE:ApplyGameCommand("urlnoexit," .. urlstringyo)
				end
			end
		},
		LoadFont("Common normal") .. {
			Name = "PackAverageDiff",
			InitCommand = function(self)
				self:x(c2xc3x)
				self:zoom(tzoom)
				self:halign(1)
			end,
			DisplayCommand = function(self)
				local avgdiff = packinfo:GetAvgDifficulty()
				self:settextf("%0.2f", avgdiff)
				self:diffuse(byMSD(avgdiff))
			end
		},
		LoadFont("Common normal") .. {
			Name = "PackSize",
			InitCommand = function(self)
				self:x(c3x):zoom(tzoom):halign(1)
			end,
			DisplayCommand = function(self)
				local psize = packinfo:GetSize() / 1024 / 1024
				self:settextf("%i%s", psize, translated_info["MB"])
				self:diffuse(byFileSize(psize))
			end
		},
		LoadFont("Common normal") .. {
			Name = "PackPlays",
			InitCommand = function(self)
				self:x(c4x):zoom(tzoom):halign(1)
			end,
			DisplayCommand = function(self)
				self:settextf("%d", packinfo:GetPlayCount())
			end
		},
		LoadFont("Common normal") .. {
			Name = "PackSongs",
			InitCommand = function(self)
				self:x(c5x):zoom(tzoom):halign(1)
			end,
			DisplayCommand = function(self)
				self:settextf("%d", packinfo:GetSongCount())
			end
		},
		UIElements.TextToolTip(1, 1, "Common normal") .. {
			Name = "PackDownload",
			InitCommand = function(self)
				self:x(c6x)
				self:zoom(tzoom)
				self:halign(1)
			end,
			DisplayCommand = function(self)
				if installed then
					self:settext(translated_info["Installed"])
				else
					self:settext(translated_info["Download"])
				end
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" then
					if packinfo:GetSize() > 2000000000 then
						GAMESTATE:ApplyGameCommand("urlnoexit," .. packinfo:GetURL())
					else
						packinfo:DownloadAndInstall(false)
					end
				end
			end
		},
	}
	return o
end

for i = 1, numpacks do
	o[#o + 1] = makePackDisplay(i)
end

return o
