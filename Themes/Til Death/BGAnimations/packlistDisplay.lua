local tzoom = 0.5
local pdh = 42 * tzoom
local ygap = 2
local packspaceY = pdh + ygap

local numpacks = 20
local ind = 0
local offx = 5
local width = SCREEN_WIDTH * 0.6
local dwidth = width - offx * 2
local height = (numpacks+2) * packspaceY

local adjx = 14
local c1x = 10 
local c2x = c1x + (tzoom*5*adjx)
local c5x = dwidth							-- right aligned cols
local c4x = c5x - adjx - (tzoom*8*adjx) 	-- right aligned cols
local c3x = c4x - adjx - (tzoom*6*adjx) 	-- right aligned cols
local headeroff = packspaceY/2

local function highlight(self)
	self:queuecommand("Highlight")
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end

local packlist
local bunchapacks
local o = Def.ActorFrame{
	Name = "PacklistDisplay",
	InitCommand=function(self)
		self:xy(0,0)
	end,
	OnCommand=function(self)
		self:SetUpdateFunction(highlight)
		bunchapacks = DLMAN:GetThePackList()
		self:queuecommand("PackTableRefresh")
	end,
	PackTableRefreshCommand=function(self)
		--bunchapacks:SetFromCoreBundle("novice")
		packlist = bunchapacks:GetPackTable()
		self:queuecommand("Update")
	end,
	UpdateCommand=function(self)
		if ind < 0 then
			ind = 0
		elseif ind > #packlist - (#packlist % numpacks) then
			ind = #packlist - (#packlist % numpacks)
		end
	end,
	NextPageCommand=function(self)
		ind = ind + numpacks
		self:queuecommand("Update")
	end,
	PrevPageCommand=function(self)
		ind = ind - numpacks
		self:queuecommand("Update")
	end,
	NextPackCommand=function(self)
		ind = ind + numpacks
		self:queuecommand("Update")
	end,
	PrevPackCommand=function(self)
		ind = ind - numpacks
		self:queuecommand("Update")
	end,
	
	Def.Quad{InitCommand=function(self) self:zoomto(width,height):halign(0):valign(0):diffuse(color("#ffffff")):diffusealpha(0.4) end},
	
	-- headers
	LoadFont("Common normal") .. {	--name
		InitCommand=function(self)
			self:xy(c2x, headeroff):zoom(tzoom):halign(0):settext("Name")
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				bunchapacks:SortByName()
				self:GetParent():queuecommand("PackTableRefresh")
			end
		end,
	},
	LoadFont("Common normal") .. {	--avg
		InitCommand=function(self)
			self:xy(c3x- 5, headeroff):zoom(tzoom):halign(1):settext("Avg")
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				bunchapacks:SortByDiff()
				self:GetParent():queuecommand("PackTableRefresh")
			end
		end,
	},
	LoadFont("Common normal") .. {	--size
		InitCommand=function(self)
			self:xy(c4x, headeroff):zoom(tzoom):halign(1):settext("Size")
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				bunchapacks:SortBySize()
				self:GetParent():queuecommand("PackTableRefresh")
			end
		end,
	},
}

local function makePackDisplay(i)
	local packinfo
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspaceY*i + headeroff)
		end,
		UpdateCommand=function(self)
			packinfo = packlist[(i + ind)]
			if packinfo then
				self:queuecommand("Display")
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		
		Def.Quad{InitCommand=function(self) self:x(offx):zoomto(dwidth,pdh):halign(0):diffuse(color("#000000")):diffusealpha(0.3) end},
		
		LoadFont("Common normal") .. {	--index
			InitCommand=function(self)
				self:x(c1x):zoom(tzoom):halign(0)
			end,
			DisplayCommand=function(self)
				self:settextf("%i.", i + ind)
			end
		},
		LoadFont("Common normal") .. {	--name
			InitCommand=function(self)
				self:x(c2x):zoom(tzoom):maxwidth(width/2/tzoom):halign(0)
			end,
			DisplayCommand=function(self)
				self:settext(packinfo:GetName()):diffuse(bySkillRange(packinfo:GetAvgDifficulty()))
			end
		},
		LoadFont("Common normal") .. {	--avg diff
			InitCommand=function(self)
				self:x(c3x):zoom(tzoom):halign(1)
			end,
			DisplayCommand=function(self)
				local avgdiff = packinfo:GetAvgDifficulty()
				self:settextf("%0.2f",avgdiff):diffuse(byMSD(avgdiff))
			end
		},
		LoadFont("Common normal") .. {	--dl button
			InitCommand=function(self)
				self:x(c5x):zoom(tzoom):halign(1):settext("Download")
			end,
			HighlightCommand=function(self)
				highlightIfOver(self)
			end,
			MouseLeftClickCommand=function(self)
				if isOver(self) then
					packinfo:DownloadAndInstall()
				end
			end
		},
		LoadFont("Common normal") .. {	--size
			InitCommand=function(self)
				self:x(c4x):zoom(tzoom):halign(1)
			end,
			DisplayCommand=function(self)
				local psize = packinfo:GetSize()/1024/1024
				self:settextf("%iMB", psize):diffuse(byFileSize(psize))
			end
		},
	}
	return o
end

for i=1,numpacks do
	o[#o+1] = makePackDisplay(i)
end

return o