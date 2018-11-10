
-- hurrrrr nps quadzapalooza -mina
local imcrazy = 500
local wodth = capWideScale(280, 300)
local hidth = 40
local txtoff = 10

local textonleft = true
local function textmover(self)
    if isOver(self:GetChild("npstext")) and textonleft then
        self:GetChild("npstext"):x(wodth-txtoff):halign(1)
        textonleft = false
	elseif isOver(self:GetChild("npstext")) and not textonleft then
        self:GetChild("npstext"):x(txtoff):halign(0)
        textonleft = true
	end
end

local function updateGraph(self)
	local steps =  GAMESTATE:GetCurrentSteps(PLAYER_1)
	if steps then
		local groot = steps:GetCDGraphVectors()
		if groot == nil then 
			for j=1,4 do 
				for i=1,imcrazy do
					self:GetChild(i..j):visible(false)
				end
			end
			return 
		end

		local moot = groot[1]
		local thingers = math.min(imcrazy,#moot)
		local wid = wodth/thingers
		local hodth = 0
		for i=1,#moot do 
			if moot[i] * 2 > hodth then
				hodth = moot[i] * 2
			end
		end
		
		self:GetChild("npsline"):y(-hidth * 0.7)
		self:GetChild("npstext"):settext(hodth/2 * 0.7 .. "nps"):y(-hidth * 0.9)
		hodth = hidth/hodth
		for j=1,4 do 
			for i=1,imcrazy do
				if i <= thingers then
					if groot[j][i] > 0 then
						self:GetChild(i..j):x(i * wid):zoomto(wid,groot[j][i]*2*hodth)
						self:GetChild(i..j):visible(true)
					else
						self:GetChild(i..j):visible(false)
					end
				else
					self:GetChild(i..j):visible(false)
				end
			end
		end
	end
end

local t = Def.ActorFrame {
    Name = "ChordDensityGraph",
    InitCommand=function(self)
		self:SetUpdateFunction(textmover)
	end,
	DelayedChartUpdateMessageCommand = function(self)
		self:queuecommand("GraphUpdate")
	end,
	GraphUpdateCommand = function(self)
		if self:GetVisible() then
			updateGraph(self)
		end
	end,
	Def.Quad {
        Name = "cdbg",
        InitCommand = function(self)
            self:zoomto(wodth, hidth + 2):valign(1):diffuse(color("1,1,1,1")):halign(0)
        end
    }
}

local function makeaquad(i,n, col)
	local o = Def.Quad {
		Name = i..n,
		InitCommand = function(self)
			self:zoomto(20, 0):valign(1):diffuse(color(col)):halign(1)
		end,
	}
	return o
end

for i=1,imcrazy do
    t[#t + 1] = makeaquad(i,1, ".75,.75,.75")   -- nps
    t[#t + 1] = makeaquad(i,2, ".5,.5,.5")      -- jumps
    t[#t + 1] = makeaquad(i,3, ".25,.25,.25")   -- hands
    t[#t + 1] = makeaquad(i,4, ".1,.1,.1")      -- quads
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
