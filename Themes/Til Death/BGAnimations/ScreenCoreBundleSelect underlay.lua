local minidoots = {"Novice", "Beginner", "Intermediate", "Advanced", "Expert"}
local diffcolors = {"#66ccff","#099948","#ddaa00","#ff6666","#c97bff"}
local packsy
local packspacing = 54
local ind = 7

local function input(event)
	if event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("ScMouseLeftClick")
		end
	end
	return false
end

local o = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(SCREEN_WIDTH/2, 50):halign(0.5)
	end,
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end,
	CodeMessageCommand = function(self, params)
		if params.Name == 'Up' then
			ind = ind - 1
			if ind < 1 or ind > 5 then
				ind = 5
			end
			self:queuecommand("SelectionChanged")
		end
		if params.Name == 'Down' then
			ind = ind + 1
			if ind > 5 then
				ind = 1
			end
			self:queuecommand("SelectionChanged")
		end
		if params.Name == 'Select' then
			if ind < 6 and ind > 0 then
				DLMAN:DownloadCoreBundle(minidoots[ind]:lower())
				SCREENMAN:GetTopScreen():StartTransitioningScreen("SM_GoToNextScreen") 
			end
		end
	end,
	Def.Quad{
		InitCommand=function(self)
			self:y(200):zoomto(500,500):diffuse(getMainColor('frames')):diffusealpha(1)
		end,
	},
	LoadFont("Common Large") .. {
		InitCommand=function(self)
			self:zoom(0.5)
		end,
		OnCommand=function(self)
			self:settext("You have no songs!")
		end
	},
	LoadFont("Common normal") .. {
		InitCommand=function(self)
			self:y(24):zoom(0.5)
		end,
		OnCommand=function(self)
			self:settext("Select a skill range to begin downloading some")
		end
	},
	LoadFont("Common normal") .. {
		InitCommand=function(self)
			self:y(330):zoom(0.4)
		end,
		OnCommand=function(self)
			self:settext("Core bundles are diverse selections of packs that span a skill range. They are chosen based on quality\nand popularity and are intended to span a variety of music and chart types. They will always be \navailable for download in the Packs tab in case you misjudge your level or wish for an easy step up.")
		end
	}
}

local function UpdateHighlight(self)
	self:GetChild("Doot"):playcommand("Doot")
end

local function makedoots(i)
	local packinfo
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspacing*i)
			self:SetUpdateFunction(UpdateHighlight)
		end,
		
		Def.Quad{
			Name="Doot",
			InitCommand=function(self)
				self:y(-12):zoomto(400,48):valign(0):diffuse(color(diffcolors[i]))
			end,
			OnCommand=function(self)
				self:queuecommand("SelectionChanged")
			end,
			SelectionChangedCommand=function(self)
				if i == ind then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.5)
				end
			end,
			ScMouseLeftClickMessageCommand=function(self)
				if isOver(self) and ind == i then
					DLMAN:DownloadCoreBundle(minidoots[i]:lower())
					SCREENMAN:GetTopScreen():StartTransitioningScreen("SM_GoToNextScreen") 
				elseif isOver(self) then
					ind = i
					self:diffusealpha(1)
				end
			end,
			DootCommand=function(self)
				if isOver(self) and ind ~= i then
					self:diffusealpha(0.75)
				elseif ind == i then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.5)
				end
			end,
		},
		LoadFont("Common Large") .. {
			InitCommand=function(self)
				self:zoom(0.5)
			end,
			OnCommand=function(self)
				self:settext(minidoots[i])
			end
		},
		LoadFont("Common normal") .. {
			InitCommand=function(self)
				self:y(24):zoom(0.5)
			end,
			OnCommand=function(self)
				local bundle = DLMAN:GetCoreBundle(minidoots[i]:lower())
				self:settextf("(%dmb)", bundle["TotalSize"])
			end
		}
	}
	return t
end

for i=1,#minidoots do
	o[#o+1] = makedoots(i)
end







local function input2(event)
	if event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("ScMouseLeftClick")
		end
	end
	return false
end


local dzoom = 0.5
local pdh = 42 * dzoom
local ygap = 2
local packspaceY = pdh + ygap

local numpacks = 10
local ind = 0
local width = SCREEN_WIDTH * 0.6
local height = (numpacks+1) * packspaceY
local c1x = 34
local c2x = c1x + 14
local c4x = width - 10		-- right aligned
local c3x = c4x - 14 - (dzoom*8*14)

local function highlight(self)
	self:queuecommand("Highlight")
end
local packlist 
local t = Def.ActorFrame{
	OnCommand=function(self)
		self:SetUpdateFunction(high)
		SCREENMAN:GetTopScreen():AddInputCallback(input2)
		self:xy(20,100)
		self:queuecommand("Update")
	end,
	UpdateCommand=function(self)
		packlist = DLMAN:GetPackList()
	end,
	ScMouseLeftClickMessageCommand=function(self)
		ind = ind + 10
		self:queuecommand("Update")
	end,
	
	Def.Quad{InitCommand=function(self) self:zoomto(width,height):halign(0):valign(0):diffuse(color("#ffffff")):diffusealpha(0.4) end},
	
	-- headers
	LoadFont("Common normal") .. {	--size
		InitCommand=function(self)
			self:x(c1x):zoom(dzoom):halign(1):settext("Size")
		end,
		HighlightCommand=function(self)
			if isOver(self) then
				self:diffusealpha(0.4)
			else
				self:diffusealpha(1)
			end
		end
	},
	LoadFont("Common normal") .. {	--name
		InitCommand=function(self)
			self:x(c2x):zoom(dzoom):halign(0):settext("Name")
		end,
	},
}

local function makePackDisplay(i)
	local packinfo
	
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspaceY*i)
		end,
		UpdateCommand=function(self)
			packinfo = packlist[i + ind]
		end,
		
		Def.Quad{InitCommand=function(self) self:zoomto(width,pdh):halign(0):diffuse(color("#000000")):diffusealpha(0.3) end},
		
		
		LoadFont("Common normal") .. {	--size
			InitCommand=function(self)
				self:x(c1x):zoom(dzoom):halign(1)
			end,
			UpdateCommand=function(self)
				local psize = packinfo:GetSize()/1024/1024
				self:settextf("%i", psize):diffuse(byFileSize(psize))
			end
		},
		LoadFont("Common normal") .. {	--name
			InitCommand=function(self)
				self:x(c2x):zoom(dzoom):maxwidth(width/2/dzoom):halign(0)
			end,
			UpdateCommand=function(self)
				self:settext(packinfo:GetName()):diffuse(bySkillRange(packinfo:GetAvgDifficulty()))
			end
		},
		LoadFont("Common normal") .. {	--avg diff
			InitCommand=function(self)
				self:x(c3x):zoom(dzoom):halign(1)
			end,
			UpdateCommand=function(self)
				local avgdiff = packinfo:GetAvgDifficulty()
				self:settextf("%0.2f",avgdiff):diffuse(byMSD(avgdiff))
			end
		},
		LoadFont("Common normal") .. {	--dl button
			InitCommand=function(self)
				self:x(c4x):zoom(dzoom):halign(1):settext("Download")
			end,
		},
	}
	return o
end

for i=1,numpacks do
	t[#t+1] = makePackDisplay(i)
end














return t