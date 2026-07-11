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

local function getColorForDensity(density, nColumns)
    -- Generically (generally? intelligently? i dont know) set a range
	-- The value var describes the level of density.
    -- Beginning at lowVal for 0, to highVal for nColumns.
    local interval = 1 / nColumns
	local value = 1 - density * interval
	return color(tostring(value)..","..tostring(value)..","..tostring(value))
end

local function getColorForMine()
	-- this is white because the diffuseshift does the coloring
	return color("1,1,1,1")
end

local function updateGraphMultiVertex(parent, realgraph, minegraph)
	local steps = GAMESTATE:GetCurrentSteps()
	if steps then
		local ncol = steps:GetNumColumns()
		local rate = math.max(MIN_MUSIC_RATE, getCurRateValue())
		local graphVectors = steps:GetCDGraphVectors(rate)
		if graphVectors == nil then
			-- reset everything if theres nothing to show
			realgraph:SetVertices({})
			realgraph:SetDrawState( {Mode = "DrawMode_Quads", First = 0, Num = 0} )
			minegraph:SetVertices({})
			minegraph:SetDrawState( {Mode = "DrawMode_Quads", First = 0, Num = 0} )
			return
		end
		
		local npsVector = graphVectors[1] -- refers to the cps vector for 1 (tap notes)
		local mineVector = steps:GetCDGraphVectors(rate, "TapNoteType_Mine")
		parent.npsVector = npsVector
		local numberOfColumns = #npsVector
		local columnWidth = wodth/numberOfColumns

		-- set height scale of graph relative to the max nps
		local hodth = 0
		for i=1,#npsVector do
			if npsVector[i] * 2 > hodth then
				hodth = npsVector[i] * 2
			end
		end
		local maxValueAllowed = hodth
		
		parent:GetChild("npsline"):y(-hidth * 0.7)
		parent:GetChild("npstext"):settext(hodth / 2 * 0.7 .. translated_info["nps"]):y(-hidth * 0.9)

		hodth = hidth/hodth
		maxValueAllowed = maxValueAllowed * hodth

		local verts = {} -- reset the vertices for the graph
		local mverts = {}
		local yOffset = 0 -- completely unnecessary, just a Y offset from the graph
		local lastIndex = 1
		for density = 1,ncol do
			for column = 1,numberOfColumns do
				local val = graphVectors[density][column]
				if val > 0 then
					local barColor = getColorForDensity(density, ncol)
					makeABar(verts, math.min(column * columnWidth, wodth), yOffset, columnWidth, val * 2 * hodth, barColor)
					if column > lastIndex then
						lastIndex = column
					end
				end
			end
		end
		for column = 1, numberOfColumns do
			local val = mineVector[column]
			if val > 0 then
				local barColor = getColorForMine()
				makeABar(mverts, math.min(column * columnWidth, wodth), yOffset, columnWidth, math.min(maxValueAllowed, val * 2 * hodth), barColor)
				if column > lastIndex then
					lastIndex = column
				end
			end
		end

		parent.finalNPSVectorIndex = lastIndex
		
		realgraph:SetVertices(verts)
		realgraph:SetDrawState( {Mode = "DrawMode_Quads", First = 1, Num = #verts} )
		minegraph:SetVertices(mverts)
		minegraph:SetDrawState( {Mode = "DrawMode_Quads", First = 1, Num = #mverts} )
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

t[#t+1] = Def.ActorMultiVertex {
	Name = "CDGraphDrawer",
	GraphUpdateCommand = function(self)
		if self:IsVisible() then
			self:GetParent():SetUpdateFunction(textmover)
			updateGraphMultiVertex(cdg, self, self:GetParent():GetChild("CDGraphDrawerMines"))
			self:GetParent():linear(0.3)
			self:GetParent():diffusealpha(1)
		else
			self:GetParent():SetUpdateFunction(nil)
		end
	end
}
t[#t+1] = Def.ActorMultiVertex {
	Name = "CDGraphDrawerMines",
	InitCommand = function(self)
		self:diffuseshift()
		self:effectclock("timer")
		self:effectperiod(2)
		self:effectcolor1(color("#FF000000"))
		self:effectcolor2(color("#FF0000FF"))
	end,
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
