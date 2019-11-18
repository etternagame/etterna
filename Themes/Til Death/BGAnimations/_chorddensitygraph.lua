local cdg

local optionalParam = Var("width")

-- hurrrrr nps quadzapalooza -mina
local wodth = capWideScale(280, 300)
if optionalParam ~= nil then
	wodth = optionalParam
end
local hidth = 40
local txtoff = 10

local translated_info = {
	nps = THEME:GetString("ChordDensityGraph", "NPS")
}

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

local function makeABar(vertices, x, y, barWidth, barHeight, prettycolor)
	vertices[#vertices + 1] = {{x,y-barHeight,0},prettycolor}
	vertices[#vertices + 1] = {{x-barWidth,y-barHeight,0},prettycolor}
	vertices[#vertices + 1] = {{x-barWidth,y,0},prettycolor}
	vertices[#vertices + 1] = {{x,y,0},prettycolor}
end

local function getColorForDensity(density, ncol)
	if density == 1 then
		return color(".75,.75,.75") -- nps color
	elseif density == ncol then
		return color(".1,.1,.1")	-- biggest chord color
	elseif density == 2 then
		return color(".5,.5,.5") -- jumps color
	elseif density == 3 then
		return color(".25,.25,.25") -- hands color
	--else
		--local c =  lerp(density / (ncol +1 ), .1, .75) 	-- im sure we can programmatically handle n columns but im too lazy 
		--return color(c..","..c..","..c..","..c)			-- to make this look nice atm -mina
	end
end

local function updateGraphMultiVertex(parent, realgraph)
	local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
	if steps then
		local ncol = steps:GetNumColumns()
		local rate = math.max(1, getCurRateValue())
		local graphVectors = steps:GetCDGraphVectors(rate)
		if graphVectors == nil then
			-- reset everything if theres nothing to show
			realgraph:SetVertices({})
			realgraph:SetDrawState( {Mode = "DrawMode_Quads", First = 0, Num = 0} )
			return
		end
		
		local npsVector = graphVectors[1] -- refers to the cps vector for 1 (tap notes)
		local numberOfColumns = #npsVector
		local columnWidth = wodth/numberOfColumns * rate
		
		-- set height scale of graph relative to the max nps
		local hodth = 0
		for i=1,#npsVector do
			if npsVector[i] * 2 > hodth then
				hodth = npsVector[i] * 2
			end
		end
		
		parent:GetChild("npsline"):y(-hidth * 0.7)
		parent:GetChild("npstext"):settext(hodth / 2 * 0.7 .. translated_info["nps"]):y(-hidth * 0.9)
		hodth = hidth/hodth
		local verts = {} -- reset the vertices for the graph
		local yOffset = 0 -- completely unnecessary, just a Y offset from the graph
		for density = 1,ncol do
			for column = 1,numberOfColumns do
					if graphVectors[density][column] > 0 then
						local barColor = getColorForDensity(density, ncol)
						makeABar(verts, math.min(column * columnWidth, wodth), yOffset, columnWidth, graphVectors[density][column] * 2 * hodth, barColor)
					end
			end
		end
		
		realgraph:SetVertices(verts)
		realgraph:SetDrawState( {Mode = "DrawMode_Quads", First = 1, Num = #verts} )
	end
end

local t = Def.ActorFrame {
    Name = "ChordDensityGraph",
    InitCommand=function(self)
		cdg = self
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:diffusealpha(0)
	end,
	DelayedChartUpdateMessageCommand = function(self)
		self:playcommand("GraphUpdate")
	end,
	CurrentRateChangedMessageCommand = function(self)
		if self:IsVisible() then
			self:queuecommand("GraphUpdate")
		end
	end,
	ChartPreviewOnMessageCommand = function(self)
		self:queuecommand("GraphUpdate")
	end,
	PracticeModeReloadMessageCommand = function(self)
		self:queuecommand("GraphUpdate")
	end,
	Def.Quad {
        Name = "cdbg",
        InitCommand = function(self)
            self:zoomto(wodth, hidth + 2):valign(1):diffuse(color("1,1,1,1")):halign(0)
        end
    }
}

t[#t+1] =
	Def.ActorMultiVertex {
		Name = "CDGraphDrawer",
		GraphUpdateCommand = function(self)
			if self:IsVisible() then
				self:GetParent():SetUpdateFunction(textmover)
				updateGraphMultiVertex(cdg, self)
				self:GetParent():linear(0.3)
				self:GetParent():diffusealpha(1)
			else
				self:GetParent():SetUpdateFunction(nil)
			end
		end
	}

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
