
-- hurrrrr nps quadzapalooza -mina
local imcrazy = 500
local wodth = 300
local hidth = 40

local textonleft = true
local function textmover(self)
    if isOver(self:GetChild("npstext")) and textonleft then
        self:GetChild("npstext"):x(wodth-10):halign(1)
        textonleft = false
	elseif isOver(self:GetChild("npstext")) and not textonleft then
        self:GetChild("npstext"):x(10):halign(0)
        textonleft = true
	end
end

local t = Def.ActorFrame {
    Name = "ChordDensityGraph",
	InitCommand=function(self)
		self:SetUpdateFunction(textmover)
	end,
	NoteFieldVisibleMessageCommand = function(self)
		self:visible(true)
		self:queuecommand("PlayingSampleMusic")
	end,
	DeletePreviewNoteFieldMessageCommand = function(self)
		self:visible(false)
	end,
    PlayingSampleMusicMessageCommand = function(self)
        local steps =  GAMESTATE:GetCurrentSteps(PLAYER_1)
		if steps then
			local moot = steps:GetNPSVector()
			local joot = steps:GetCPSVector(2)
			local hoot = steps:GetCPSVector(3)
			local qoot = steps:GetCPSVector(4)
			local thingers = math.min(imcrazy,#moot)
			local wid = wodth/thingers
            local hodth = 0
			for i=1,#moot do 
				if moot[i] * 2 > hodth then
					hodth = moot[i] * 2
				end
            end
            
            self:GetChild("npsline"):y(-hidth * 0.8 * 0.65)
            self:GetChild("npstext"):settext(hodth/2 * 0.8 * 0.65 .. "nps"):y(-hidth * 0.7)

			hodth = hidth/hodth * 0.8
			for i=1,imcrazy do
				if i <= thingers then
					if moot[i] > 0 then
						self:GetChild(i):x(i * wid):zoomto(wid,moot[i]*2*hodth)
						self:GetChild(i):visible(true)
					else
						self:GetChild(i):visible(false)
					end

					if joot[i] > 0 then
						self:GetChild(i.."j"):x(i * wid):zoomto(wid,joot[i]*2*2*hodth)
						self:GetChild(i.."j"):visible(true)
					else
						self:GetChild(i.."j"):visible(false)
					end

					if hoot[i] > 0 then
						self:GetChild(i.."h"):x(i * wid):zoomto(wid,hoot[i]*2*3*hodth)
						self:GetChild(i.."h"):visible(true)
					else
						self:GetChild(i.."h"):visible(false)
					end

					if qoot[i] > 0 then
						self:GetChild(i.."q"):x(i * wid):zoomto(wid,qoot[i]*2*4*hodth)
						self:GetChild(i.."q"):visible(true)
					else
						self:GetChild(i.."q"):visible(false)
					end
				else
					self:GetChild(i):visible(false)
					self:GetChild(i.."j"):visible(false)
					self:GetChild(i.."h"):visible(false)
					self:GetChild(i.."q"):visible(false)
				end
			end
		end
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(wodth, hidth + 2):diffusealpha(1):valign(1):diffuse(color("1,1,1")):halign(0)
		end,
    }
}

local function makeaquad(i)
	local o = Def.Quad {
		Name = i,
		InitCommand = function(self)
			self:zoomto(20, 0):diffusealpha(0.75):valign(1):diffuse(color(".75,.75,.75")):halign(1)
		end,
	}
	return o
end

-- generalizing this messed with the child name access or i was tired and did something wrong but i dont care enough to mess with it -mina
local function makeaquadforjumpcounts(i)
	local o = Def.Quad {
		Name = i.."j",
		InitCommand = function(self)
			self:zoomto(20, 0):diffusealpha(0.85):valign(1):diffuse(color(".5,.5,.5")):halign(1)
		end,
	}
	return o
end

local function makeaquadforhandcounts(i)
	local o = Def.Quad {
		Name = i.."h",
		InitCommand = function(self)
			self:zoomto(20, 0):diffusealpha(0.95):valign(1):diffuse(color(".25,.25,.25")):halign(1)
		end,
	}
	return o
end

local function makeaquadforquadcounts(i)
	local o = Def.Quad {
		Name = i.."q",
		InitCommand = function(self)
			self:zoomto(20, 0):diffusealpha(1):valign(1):diffuse(color(".1,.1,.1")):halign(1)
		end,
	}
	return o
end

for i=1,imcrazy do
    t[#t + 1] = makeaquad(i)
    t[#t + 1] = makeaquadforjumpcounts(i)
    t[#t + 1] = makeaquadforhandcounts(i)
    t[#t + 1] = makeaquadforquadcounts(i)
end

-- down here for draw order
t[#t + 1] = Def.Quad {
    Name = "npsline",
    InitCommand = function(self)
        self:zoomto(wodth, 2):diffusealpha(1):valign(1):diffuse(color(".75,0,0,0.75")):halign(0)
    end,

}


t[#t + 1] = LoadFont("Common Normal") .. {
    Name = "npstext",
    InitCommand = function(self)
        self:halign(0)
        self:zoom(0.4)
        self:settext(""):diffuse(color("1,0,0"))
    end
}

return t
