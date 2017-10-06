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
		if getTabIndex() == 8 then
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


local Yspacing = 30
local row2Yoffset = 12

local pl


local currentpage = 1
local numpages = 1
local perpage = 10

local packlist = DLMAN:GetPackList()

t[#t+1] = Def.Quad{InitCommand=function(self)
	self:xy(frameX,frameY):zoomto(frameWidth,frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
end}
t[#t+1] = Def.Quad{InitCommand=function(self)
	self:xy(frameX,frameY):zoomto(frameWidth,offsetY):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(0.5)
end}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=function(self)
	self:xy(frameX+5,frameY+offsetY-9):zoom(0.6):halign(0):diffuse(getMainColor('positive')):settext("Downloads (WIP)")
end}


local function BroadcastIfActive(msg)
	if update then
		MESSAGEMAN:Broadcast(msg)
	end
end

local function ButtonActive(self,scale)
	return isOverScaled(self,scale) and update
end



local function DownloadButton(i)
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:x(15):y(25)
		end,
		LoadFont("Common Large") .. {
			Name="Text",
			InitCommand=function(self)
			self:halign(0):maxwidth(frameWidth * 3 + 140):zoom(fontScale)
			self:settext("Download")
			self:GetParent():queuecommand("Resize")
			end
		},
		Def.Quad{
			Name="Button",
			InitCommand=function(self)
				self:diffusealpha(buttondiffuse):halign(0):zoomto(100, 20)
			end,
			MouseLeftClickMessageCommand=function(self)
				if update and isOver(self) and packlist[i + ((currentpage - 1) * perpage)] then
					packlist[i + ((currentpage - 1) * perpage)]:DownloadAndInstall()
				end
			end,
			PackDownloadedMessageCommand=function(self, params) SCREENMAN:SystemMessage(params.pack:GetName()) end
		}
	}
	return o
end

local function PackLabel(i)
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY  +12+ (i-1)*Yspacing)
		end,
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
				self:maxwidth(100)
				self:halign(0.5)
				self:y(row2Yoffset)
				self:queuecommand("PacksPageMessage")
			end,
			PacksPageMessageCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settext(((rankingPage-1)*25)+i + ((currentpage - 1) * perpage)..".")
			end,
			DisplayAllMessageCommand = function(self) self:queuecommand("PacksPage") end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale):maxwidth(250/fontScale)
				self:xy(15,row2Yoffset)
				self:queuecommand("PacksPageMessage")
			end,
			PacksPageMessageCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settext(packlist[i + ((currentpage - 1) * perpage)] and packlist[i + ((currentpage - 1) * perpage)]:GetName() or "")
			end,
			DisplayAllMessageCommand = function(self) self:queuecommand("PacksPage") end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
				self:xy(315,row2Yoffset)
				self:queuecommand("PacksPageMessage")
			end,
			PacksPageMessageCommand=function(self)
				local rating = packlist[i + ((currentpage - 1) * perpage)] and packlist[i + ((currentpage - 1) * perpage)]:GetAvgDifficulty() or 0
				self:settextf("%.2f", rating)
				self:diffuse(ByMSD(rating))
			end,
			DisplayAllMessageCommand = function(self) self:queuecommand("PacksPage") end
		},
	}
	t[#t+1] = DownloadButton(i)
	return t
end

local packs = Def.ActorFrame{
	OnCommand=function(self)
		numpages = notShit.ceil(#packlist/perpage)
	end
}

packs[#packs+1] = b

for i=1,perpage do
	packs[#packs+1] = PackLabel(i)
end

packs[#packs+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(rankingX + offsetX+290, rankingY + 8+offsetY)
		self:halign(0):zoom(fontScale)
		self:settextf("Avg Rating")
		self:diffuse(getMainColor("positive"))
	end
}

-- next/prev page buttons
pageButtons = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(frameX+10,frameY+rankingY+300)
	end,
	Def.Quad{
		InitCommand=function(self)
			self:xy(300,-8):zoomto(40,20):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand=function(self)
			if update and isOver(self) then
				if currentpage < numpages then
					currentpage = currentpage + 1
					MESSAGEMAN:Broadcast("PacksPage")
				else
					currentpage = 1
					MESSAGEMAN:Broadcast("PacksPage")
				end
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:x(300):halign(0):zoom(0.3):diffuse(getMainColor('positive')):settext("Next")
		end
	},
	Def.Quad{
		InitCommand=function(self)
			self:y(-8):zoomto(65,20):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand=function(self)
			if update and isOver(self) then
				if currentpage > 1 then
					currentpage = currentpage-1
					MESSAGEMAN:Broadcast("PacksPage")
				else
					currentpage = numpages
					MESSAGEMAN:Broadcast("PacksPage")
				end
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:halign(0):zoom(0.3):diffuse(getMainColor('positive')):settext("Previous")
		end,
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:x(175):halign(0.5):zoom(0.3):diffuse(getMainColor('positive'))
		end,
		SetCommand=function(self)
			self:settextf("Showing %i-%i of %i", math.min(((currentpage-1)*perpage)+1, #packlist), math.min(currentpage*perpage, #packlist), #packlist)
		end,
		PacksPageMessageCommand=function(self) self:queuecommand("Set") end
	}
}



t[#t+1] = packs
t[#t+1] = pageButtons
return t