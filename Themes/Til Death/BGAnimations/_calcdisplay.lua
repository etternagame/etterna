-- for cdgraph fitting
--[[
local plotWidth, plotHeight = capWideScale(280, 300), 40
local plotX, plotY = plotWidth/2, 0
]]
-- for standard fitting
local oldWidth = capWideScale(280, 300)
local plotWidth, plotHeight = capWideScale(300,450), 160
local plotX, plotY = oldWidth+3 + plotWidth/2, -20 + plotHeight/2

local dotDims, plotMargin = 2, 4
local maxOffset = 40
local baralpha = 0.2
local bgalpha = 0.9
local textzoom = 0.35
local enabled = false

local song
local steps
local finalSecond
local graph1Vec1, graph1Vec2
local graph2Vec1, graph2Vec2

local function fitX(x) -- Scale time values to fit within plot width.
	if finalSecond == 0 then
		return 0
	end
	return x / finalSecond * plotWidth - plotWidth / 2
end

local function fitY(y) -- Scale diff values to fit within plot height
	return -1 * y / maxOffset * plotHeight
end

-- transforms the position of the mouse from the cd graph to the calc info graph
local function transformPosition(pos, w, px)
    distanceAcrossOriginal = (pos - px) / w
    out = distanceAcrossOriginal * plotWidth - plotWidth/2
    return out
end

-- edit this to change graph and number output
local function updateCoolStuff()
    song = GAMESTATE:GetCurrentSong()
    steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
    if song then
        finalSecond = GAMESTATE:GetCurrentSong():GetLastSecond() * 2
    end
    if steps then
        graph1Vec1 = steps:DootSpooks()[1]
        graph1Vec2 = steps:DootSpooks()[2]
        graph2Vec1 = graph1Vec1
        graph2Vec2 = graph1Vec2
    else
        graph1Vec1 = {}
        graph1Vec2 = {}
        graph2Vec1 = {}
        graph2Vec2 = {}
    end
end

local o =
	Def.ActorFrame {
    Name = "notChordDensityGraph", -- it's not the chord density graph
	OnCommand = function(self)
        self:xy(plotX, plotY)
    end,
    CalcInfoOnMessageCommand = function(self)
        updateCoolStuff()
        self:visible(true)
        enabled = true
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end,
    CalcInfoOffMessageCommand = function(self)
        self:visible(false)
        enabled = false
    end,
    CurrentStepsP1ChangedMessageCommand = function(self)
        updateCoolStuff()
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end
}
-- graph bg
o[#o + 1] = Def.Quad {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
    end,
    DoTheThingCommand = function(self)
        self:visible(song ~= nil)
    end
}

-- second bg
o[#o + 1] = Def.Quad {
    InitCommand = function(self)
        self:y(plotHeight + 5)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
    end,
    DoTheThingCommand = function(self)
        self:visible(song ~= nil)
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:xy(-plotWidth/4, plotHeight + 5 + plotHeight/2 + 35)
        self:zoom(0.55)
        self:settext("")
        self:maxwidth(plotWidth * 3/4 / 0.55)
        self:halign(0)
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            title = song:GetDisplayFullTitle()
            artist = song:GetDisplayArtist()
            self:settext(title .. "\n  ~" .. artist)
        end
    end
}

-- top graph average text
o[#o + 1] = LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:xy(-plotWidth/2 + 5, plotHeight/3):halign(0)
        self:zoom(0.5)
        self:settext("fff")
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            local ave1 = 0
            local ave2 = 0
            if #graph1Vec1 > 0 then ave1 = table.average(graph1Vec1) end
            if #graph1Vec2 > 0 then ave2 = table.average(graph1Vec2) end
            self:settextf("Left Average: %.4f\nRight Average: %.4f", ave1, ave2)
        end
    end
}

-- lower graph average text
o[#o + 1] = LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:xy(-plotWidth/2 + 5, plotHeight + plotHeight/3):halign(0)
        self:zoom(0.5)
        self:settext("fff")
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            local ave1 = 0
            local ave2 = 0
            if #graph2Vec1 > 0 then ave1 = table.average(graph2Vec1) end
            if #graph2Vec2 > 0 then ave2 = table.average(graph2Vec2) end
            self:settextf("Left Average: %.4f\nRight Average: %.4f", ave1, ave2)
        end
    end
}


local dotWidth = 0
local function setOffsetVerts(vt, x, y, c)
	vt[#vt + 1] = {{x - dotWidth, y + dotWidth, 0}, c}
	vt[#vt + 1] = {{x + dotWidth, y + dotWidth, 0}, c}
	vt[#vt + 1] = {{x + dotWidth, y - dotWidth, 0}, c}
	vt[#vt + 1] = {{x - dotWidth, y - dotWidth, 0}, c}
end
-- top graph main line
o[#o + 1] = Def.ActorMultiVertex {
    DoTheThingCommand = function(self)
        if song and enabled then
            self:SetVertices({})
            self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

            self:visible(true)
            local verts = {}
            local highest = 0
            
            for i = 1, #graph1Vec1 do
                if graph1Vec1[i] > highest then
                    highest = graph1Vec1[i]
                end
            end
            maxOffset = highest * 1.2
            for i = 1, #graph1Vec1 do
                local x = fitX(i)
                local y = fitY(graph1Vec1[i])
                local cullur = offsetToJudgeColor(y, 1)
                y = y + plotHeight / 2
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- top graph alt line
o[#o + 1] = Def.ActorMultiVertex {
    DoTheThingCommand = function(self)
        if song and enabled then
            self:SetVertices({})
            self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

            self:visible(true)
            local verts = {}
            local highest = 0
            
            for i = 1, #graph1Vec2 do
                if graph1Vec2[i] > highest then
                    highest = graph1Vec2[i]
                end
            end
            maxOffset = highest * 1.2
            for i = 1, #graph1Vec2 do
                local x = fitX(i)
                local y = fitY(graph1Vec2[i])
                local cullur = offsetToJudgeColor(y/10, 1)
                y = y + plotHeight / 2
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}


-- bottom graph main line
o[#o + 1] = Def.ActorMultiVertex {
    InitCommand = function(self)
        self:y(plotHeight+5)
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            self:SetVertices({})
            self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

            self:visible(true)
            local verts = {}
            local highest = 0
            
            for i = 1, #graph2Vec1 do
                if graph2Vec1[i] > highest then
                    highest = graph2Vec1[i]
                end
            end
            maxOffset = highest * 1.2
            for i = 1, #graph2Vec1 do
                local x = fitX(i)
                local y = fitY(graph2Vec1[i])
                local cullur = offsetToJudgeColor(y, 1)
                y = y + plotHeight / 2
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph alt line
o[#o + 1] = Def.ActorMultiVertex {
    InitCommand = function(self)
        self:y(plotHeight+5)
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            self:SetVertices({})
            self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

            self:visible(true)
            local verts = {}
            local highest = 0
            
            for i = 1, #graph2Vec2 do
                if graph2Vec2[i] > highest then
                    highest = graph2Vec2[i]
                end
            end
            maxOffset = highest * 1.2
            for i = 1, #graph2Vec2 do
                local x = fitX(i)
                local y = fitY(graph2Vec2[i])
                local cullur = offsetToJudgeColor(y/10, 1)
                y = y + plotHeight / 2
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- a bunch of things for stuff and things
o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "Seektext1",
    InitCommand = function(self)
        self:y(8):valign(1):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
    end,
    UpdatePositionCommand = function(self, params)
        self:x(transformPosition(params.pos, params.w, params.px) - 5)
    end
}

o[#o + 1] = Def.Quad {
    Name = "Seek1",
    InitCommand = function(self)
        self:zoomto(2, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end,
    UpdatePositionCommand = function(self, params)
        self:x(transformPosition(params.pos, params.w, params.px))
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "Seektext2",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
    end,
    UpdatePositionCommand = function(self, params)
        self:x(transformPosition(params.pos, params.w, params.px) - 5)
    end
}

o[#o + 1] = Def.Quad {
    Name = "Seek2",
    InitCommand = function(self)
        self:y(plotHeight+5)
        self:zoomto(2, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end,
    UpdatePositionCommand = function(self, params)
        self:x(transformPosition(params.pos, params.w, params.px))
    end
}
return o