local update = false


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
local sizeoffet = capWideScale(20,0)
local buttondiffuse = 0
local whee
local listYOffset=100

local Yspacing = 20
local row2Yoffset = 12

local pl


local currentpage = 1
local numpages = 1
local perpage = 10

local packlist = DLMAN:GetPackList()
numpages = notShit.ceil(#packlist/perpage)

local filters = {"", "0", "0", "0", "0", "0", "0"}--1=name 2=lowerdiff 3=upperdiff 4=lowersize 5=uppersize

local curInput = ""
local inputting = 0 --1=name 2=lowerdiff 3=upperdiff 4=lowersize 5=uppersize 0=none
local function getFilter(index)
	return filters[index]
end
local numbershers = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}
local englishes = {"a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",";"}
local function DlInput(event)
	if event.type ~= "InputEventType_Release" and update and inputting ~= 0 then
		local changed = false
		if event.button == "Start" then
			curInput = ""
			inputting = 0
			packlist = DLMAN:GetFilteredAndSearchedPackList(tostring(filters[1]), tonumber(filters[2]), tonumber(filters[3]), tonumber(filters[4]*1024*1024), tonumber(filters[5]*1024*1024))
			numpages = notShit.ceil(#packlist/perpage)
			if currentpage > numpages or currentpage < 1 then
				currentpage = 1
			end
			MESSAGEMAN:Broadcast("UpdatePacks")
			MESSAGEMAN:Broadcast("DlInputEnded")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			return true
		elseif event.button == "Back" then
			curInput = ""
			if inputting == 2 or inputting == 3 or inputting == 4 or inputting == 5 then
				if curInput == "" or not tonumber(curInput) then 
					curInput = "0"
				end
			end
			filters[inputting] = curInput
			inputting = 0
			packlist = DLMAN:GetFilteredAndSearchedPackList(tostring(filters[1]), tonumber(filters[2]), tonumber(filters[3]), tonumber(filters[4]*1024*1024), tonumber(filters[5]*1024*1024))
			MESSAGEMAN:Broadcast("UpdatePacks")
			MESSAGEMAN:Broadcast("DlInputEnded")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			curInput = curInput:sub(1, -2)
			changed = true
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			curInput = ""
			changed = true
		else
			if inputting == 2 or inputting == 3 or inputting == 4 or inputting == 5 then
				if tonumber(event.char) ~= nil then
					curInput = curInput..event.char
					changed = true
				end
			else
				if event.char and event.char:match("[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%'%\"%>%<%?%/%~%|%w]") and event.char ~= "" then
					curInput = curInput..event.char
					changed = true
				end
			end
		end
		if changed then
			if inputting == 2 or inputting == 3 or inputting == 4 or inputting == 5 then
				if curInput == "" or not tonumber(curInput) then 
					curInput = "0"
				end
			end
			filters[inputting] = curInput
			packlist = DLMAN:GetFilteredAndSearchedPackList(tostring(filters[1]), tonumber(filters[2]), tonumber(filters[3]), tonumber(filters[4]*1024*1024), tonumber(filters[5]*1024*1024))
			numpages = notShit.ceil(#packlist/perpage)
			if currentpage > numpages or currentpage < 1 then
				currentpage = 1
			end
			MESSAGEMAN:Broadcast("UpdatePacks")
			return true
		end
	end
end

local t = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0)
	end,
	OnCommand=function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(DlInput)
		self:bouncebegin(0.2):xy(0,0):diffusealpha(1)
	end,
	MouseRightClickMessageCommand=function(self)
		if update == true then
			inputting = 0
			curInput = 0
			MESSAGEMAN:Broadcast("DlInputEnded")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			MESSAGEMAN:Broadcast("UpdatePacks")
		end
	end,
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 8 and update then
			MESSAGEMAN:Broadcast("MouseRightClick")
		elseif getTabIndex() == 8 then
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
			self:x(250):y(row2Yoffset)
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
			end
		}
	}
	return o
end

local function PackLabel(i)
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY  +12+ (i-1)*Yspacing)
		end,
		PacksPageMessageCommand=function(self)
			if not packlist[i + ((currentpage - 1) * perpage)] then
				self:visible(false)
			else
				self:visible(true)
			end
		end,
		UpdatePacksMessageCommand=function(self) self:queuecommand("PacksPage") end,
		DlInputEndedMessageCommand=function(self) self:queuecommand("PacksPage") end,
		DisplayAllMessageCommand = function(self) self:queuecommand("PacksPage") end,
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:zoom(fontScale)
				self:maxwidth(100)
				self:halign(1)
				self:y(row2Yoffset)
				self:x(sizeoffet)
				self:queuecommand("PacksPageMessage")
			end,
			PacksPageMessageCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settext(packlist[i + ((currentpage - 1) * perpage)] and tostring(notShit.floor((packlist[i + ((currentpage - 1) * perpage)]:GetSize()/1024)/1024)) or "")
			end,
			UpdatePacksMessageCommand=function(self) self:queuecommand("PacksPage") end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("PacksPage") end,
			DisplayAllMessageCommand = function(self) self:queuecommand("PacksPage") end
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:halign(0):zoom(fontScale):maxwidth(capWideScale(210,230)/fontScale)
				self:xy(sizeoffet+10,row2Yoffset)
				self:queuecommand("PacksPageMessage")
			end,
			PacksPageMessageCommand=function(self)
				self:diffuse(getMainColor("positive"))
				self:settext(packlist[i + ((currentpage - 1) * perpage)] and packlist[i + ((currentpage - 1) * perpage)]:GetName() or "")
			end,
			UpdatePacksMessageCommand=function(self) self:queuecommand("PacksPage") end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("PacksPage") end,
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
				self:diffuse(byMSD(rating))
			end,
			UpdatePacksMessageCommand=function(self) self:queuecommand("PacksPage") end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("PacksPage") end,
			DisplayAllMessageCommand = function(self) self:queuecommand("PacksPage") end
		},
	}
	t[#t+1] = DownloadButton(i)
	return t
end

local packs = Def.ActorFrame{
	OnCommand=function(self)
		self:y(listYOffset)
		numpages = notShit.ceil(#packlist/perpage)
	end
}

packs[#packs+1] = b

for i=1,perpage do
	packs[#packs+1] = PackLabel(i)
end

packs[#packs+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(rankingX + offsetX+320, rankingY + 8+offsetY)
		self:halign(0):zoom(fontScale)
		self:settextf("Avg")
		self:diffuse(getMainColor("positive"))
	end
}
packs[#packs+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(rankingX + offsetX + sizeoffet - 20, rankingY + 8+offsetY)
		self:halign(0):zoom(fontScale)
		self:settextf("Size(MB)")
		self:diffuse(getMainColor("positive"))
	end
}
packs[#packs+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(rankingX + offsetX+100, rankingY + 8+offsetY)
		self:halign(0):zoom(fontScale)
		self:settextf("Name")
		self:diffuse(getMainColor("positive"))
	end
}
local filters = Def.ActorFrame{
	OnCommand=function(self)
	end
}
local function numFilter(i,x,y)
	return Def.ActorFrame{
		Def.Quad{
			InitCommand=function(self)
				self:addx(x):addy(y):zoomto(18,18):halign(1)
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting = i
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == i then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(x):addy(y):halign(1):maxwidth(40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(i)
				self:settext(fval)
				if tonumber(fval) > 0 or inputting == i then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		}
	}
end

filters[#filters+1] = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY*2)
		end,
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:halign(0):zoom(fontScale)
			end,
			SetCommand=function(self)
				self:settext( "Avg Difficulty:")
			end	
		},
		Def.Quad{
			InitCommand=function(self)
				self:addx(110):zoomto(18,18):halign(1)
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting = 2
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == 2 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(110):halign(1):maxwidth(40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(2)
				self:settext(fval)
				if tonumber(fval) > 0 or inputting == 2 then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		Def.Quad{
			InitCommand=function(self)
				self:addx(140):zoomto(18,18):halign(1):diffuse(color("#666666"))
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting=3
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == 3 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(140):halign(1):maxwidth(40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(3)
				self:settext(fval)
				if tonumber(fval) > 0 or inputting == 3 then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(230):halign(0):zoom(fontScale)
			end,
			SetCommand=function(self)
				self:settext( "Size(MB):")
			end	
		},
		Def.Quad{
			InitCommand=function(self)
				self:addx(310):zoomto(18,18):halign(1)
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting = 4
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == 4 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(310):halign(1):maxwidth(40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(4)
				self:settext(fval)
				if tonumber(fval) > 0 or inputting == 4 then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		Def.Quad{
			InitCommand=function(self)
				self:addx(340):zoomto(18,18):halign(1):diffuse(color("#666666"))
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting=5
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == 5 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(340):halign(1):maxwidth(40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(5)
				self:settext(fval)
				if tonumber(fval) > 0 or inputting == 5 then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		Def.Quad{
			InitCommand=function(self)
				self:addy(25):addx(150):zoomto(110,15):halign(1):diffuse(color("#666666"))
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting=1
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == 1 then
					self:diffuse(color("#999999"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			bundletimeMessageCommand=function(self)
				self:visible(false)
			end,
			MouseRightClickMessageCommand=function(self)
				self:visible(true)
			end
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addy(25):halign(0):zoom(fontScale)
			end,
			SetCommand=function(self)
				self:settext( "Name:")
			end,
			bundletimeMessageCommand=function(self)
				self:settext( "Right click to cancel selection")
			end,
			MouseRightClickMessageCommand=function(self)
				self:settext( "Name:")
			end
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addy(25):addx(45):halign(0):maxwidth(400):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(1)
				self:settext(fval)
				if fval ~= "" or inputting == 1 then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdatePacksMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
			DlInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			bundletimeMessageCommand=function(self)
				self:visible(false)
			end,
			MouseRightClickMessageCommand=function(self)
				self:visible(true)
			end
		},
	}
t[#t+1] = filters
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
				numpages = notShit.ceil(#packlist/perpage)
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
					currentpage = numpages ~= 1 and numpages or 1
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
		PacksPageMessageCommand=function(self) self:queuecommand("Set") end,
		DlInputActiveMessageCommand=function(self) self:queuecommand("Set") end,
		DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
		UpdatePacksMessageCommand=function(self) self:queuecommand("Set") end
	}
}

local minidoots = {"Novice", "Beginner", "Intermediate", "Advanced", "Expert"}
local diffcolors = {"#66ccff","#099948","#ddaa00","#ff6666","#c97bff"}
local packspacing = 30
local bundlegumbley = 155
local iamspartacus = capWideScale(0,20)
local noiamspartacus = capWideScale(20,40)
local selectedbundle = false

local bundlename
local bundleindex

local function makedoots(i)
	local packinfo
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(iamspartacus+165+packspacing*i,bundlegumbley)
		end,
		Def.Quad{
			InitCommand=function(self)
				self:y(-36):zoomto(24,12):valign(0):diffuse(color(diffcolors[i]))
			end,
			MouseLeftClickMessageCommand=function(self)
				if update and isOver(self) and not selectedbundle then
					selectedbundle = true
					bundlename = minidoots[i]
					bundleindex = i
					currentpage = 1
					packlist = DLMAN:GetCoreBundle(bundlename:lower())
					MESSAGEMAN:Broadcast("bundletime")
					MESSAGEMAN:Broadcast("UpdatePacks")
				end
			end,
			bundletimeMessageCommand=function(self)
				self:visible(false)
			end,
			MouseRightClickMessageCommand=function(self)
				if update then
					self:visible(true)
				end
			end
		}
	}
	return t
end

local function makesuperdoots(i)
	local packinfo
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:xy(iamspartacus+165+packspacing*i,bundlegumbley)
		end,
		Def.Quad{
			InitCommand=function(self)
				self:y(-16):zoomto(24,12):valign(0):diffuse(color(diffcolors[i]))
			end,
			MouseLeftClickMessageCommand=function(self)
				if update and isOver(self) and not selectedbundle then
					selectedbundle = true
					bundlename = minidoots[i].."-Expanded"
					bundleindex = i
					currentpage = 1
					packlist = DLMAN:GetCoreBundle(bundlename:lower())
					MESSAGEMAN:Broadcast("bundletime")
					MESSAGEMAN:Broadcast("UpdatePacks")
				end
			end,
			bundletimeMessageCommand=function(self)
				self:visible(false)
			end,
			MouseRightClickMessageCommand=function(self)
				if update then
					self:visible(true)
				end
			end
		},
	}
	return t
end

for i=1,#minidoots do
	t[#t+1] = makedoots(i)
end

for i=1,#minidoots do
	t[#t+1] = makesuperdoots(i)
end

t[#t+1] = LoadFont("Common normal") .. {
	InitCommand=function(self)
		self:xy(noiamspartacus,bundlegumbley-36):zoom(0.5):halign(0):valign(0)
	end,
	SetCommand=function(self)
		self:settext("Core Bundles (Basic):")
	end,
	bundletimeMessageCommand=function(self)
		self:settext("Selected Bundle: "..bundlename):diffuse(color(diffcolors[bundleindex]))
	end,
	MouseRightClickMessageCommand=function(self)
		if update then
			self:settext("Core Bundles (Basic):"):diffuse(color("#ffffff"))
		end
	end
}

t[#t+1] = LoadFont("Common normal") .. {
	InitCommand=function(self)
		self:xy(noiamspartacus,bundlegumbley - 16):zoom(0.5):halign(0):valign(0)
	end,
	SetCommand=function(self)
		self:settext("Core Bundles (Expanded):")
	end,
	bundletimeMessageCommand=function(self)
		self:settextf("Total Size: %d(MB)", packlist["TotalSize"])
	end,
	MouseRightClickMessageCommand=function(self)
		if update then
			self:settext("Core Bundles (Expanded):")
		end
	end
}

t[#t+1] = LoadFont("Common normal") .. {
	InitCommand=function(self)
		self:xy(250+noiamspartacus,bundlegumbley - 36):zoom(0.5):halign(0):valign(0):visible(false)
	end,
	bundletimeMessageCommand=function(self)
		self:settextf("Avg: %5.2f", packlist["AveragePackDifficulty"])
		self:diffuse(byMSD(packlist["AveragePackDifficulty"]))
		self:visible(true)
	end,
	MouseRightClickMessageCommand=function(self)
		if update then
			self:visible(false)
		end
	end
}

t[#t+1] = LoadFont("Common normal") .. {
	InitCommand=function(self)
		self:xy(250+noiamspartacus,bundlegumbley - 16):zoom(0.5):halign(0):valign(0):visible(false)
	end,
	bundletimeMessageCommand=function(self)
		self:settext("Download All")
		self:visible(true)
	end,
	MouseRightClickMessageCommand=function(self)
		if update then
			self:visible(false)
		end
	end
}

t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(0,0):zoomto(80,20):valign(0):diffusealpha(0)
	end,
	bundletimeMessageCommand=function(self)
		self:linear(1):xy(290+noiamspartacus,bundlegumbley - 20)
	end,
	MouseLeftClickMessageCommand=function(self)
		if update and selectedbundle and isOver(self) then
			DLMAN:DownloadCoreBundle(bundlename:lower())
			MESSAGEMAN:Broadcast("MouseRightClick")
			packlist = DLMAN:GetPackList()
			MESSAGEMAN:Broadcast("UpdatePacks")
		end
	end,
	MouseRightClickMessageCommand=function(self)
		if update and selectedbundle then
			selectedbundle = false
			packlist = DLMAN:GetPackList()
			MESSAGEMAN:Broadcast("UpdatePacks")
			self:xy(0,0)
		end
	end,
}

t[#t+1] = packs
t[#t+1] = pageButtons
return t