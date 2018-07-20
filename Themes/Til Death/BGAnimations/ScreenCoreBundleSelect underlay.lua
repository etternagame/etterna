local minidoots = {"Novice", "Beginner", "Intermediate", "Advanced", "Expert"}
local diffcolors = {"#66ccff","#099948","#ddaa00","#ff6666","#c97bff"}
local packsy
local packspacing = 54
local ind = 7

local o = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(SCREEN_WIDTH/2, 50):halign(0.5)
	end,
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
				SCREENMAN:SystemMessage(minidoots[ind]:lower())
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

local function makedoots(i)
	local packinfo
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspacing*i)
		end,
		
		Def.Quad{
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
			end
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

return o