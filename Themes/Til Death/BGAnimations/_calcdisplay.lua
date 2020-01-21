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
local highest = 0
local lowest = 0
local baralpha = 0.2
local bgalpha = 0.9
local textzoom = 0.35
local enabled = false

local song
local steps
local finalSecond
local graph1Vec1, graph1Vec2
local graph2Vec1, graph2Vec2
local ssrs

local function fitX(x, lastX) -- Scale time values to fit within plot width.
	if lastX == 0 then
		return 0
	end
	return x / lastX * plotWidth - plotWidth / 2
end

local function fitY(y) -- Scale diff values to fit within plot height
	return -1 * y / maxOffset * plotHeight
end

local function scale(x, lower, upper, scaledMin, scaledMax) -- uhh
    local perc = (x - lower) / (upper - lower)
    return perc * (scaledMax - scaledMin) + scaledMin
end

local function fitY2(y) -- scale ssr to fit thing
    local num = scale(y, lowest, highest, 0, 1)
    local out = -1 * num * plotHeight + plotHeight/2
    return out
end

local function convertPercentToIndex(x)
    local output = x
    if output < 0 then output = 0 end
    if output > 1 then output = 1 end

    local ind = notShit.round(output * #ssrs[1])
    return ind
end

local function HighlightUpdaterThing(self)
    if not enabled then return end
    self:GetChild("G2BG"):queuecommand("Highlight")
end

-- transforms the position of the mouse from the cd graph to the calc info graph
local function transformPosition(pos, w, px)
    distanceAcrossOriginal = (pos - px) / w
    out = distanceAcrossOriginal * plotWidth - plotWidth/2
    return out
end


-- for SSR graph generator, modify these constants
local ssrLowerBoundWife = 0.90 -- left end of the graph
local ssrUpperBoundWife = 1.0 -- right end of the graph
local ssrResolution = 200 -- higher number = higher resolution graph (and lag)

local function produceThisManySSRs(steps, rate)
    local count = ssrResolution
    if count < 10 then count = 10 end
    local output = {}
    for j = 1,8 do output[j] = {0,0,0,0,0,0,0,0} end

    for i = 1, count do
        local values = steps:GetSSRs(rate, ssrLowerBoundWife + ((ssrUpperBoundWife - ssrLowerBoundWife) / count) * i)
        for j = 1,8 do
            output[j][i] = values[j]
        end
    end

    return output
end

local function getGraphForSteps(steps)
    local output = produceThisManySSRs(steps, getCurRateValue())

    highest = output[1][1]
    lowest = output[1][1]
    for ss,vals in ipairs(output) do
        for ind,val in ipairs(vals) do
            if val > highest then highest = val end
            if val < lowest then lowest = val end
        end
    end
    lowest = lowest - 1
    highest = highest + 1
    return output
end

-- edit this to change graph and number output
local function updateCoolStuff()
    song = GAMESTATE:GetCurrentSong()
    steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
    if song then
        finalSecond = GAMESTATE:GetCurrentSong():GetLastSecond() * 2
    end
    if steps then
        ssrs = getGraphForSteps(steps)
        graph1Vec1 = steps:DootSpooks()[1]
        graph1Vec2 = steps:DootSpooks()[2]
        graph2Vec1 = ssrs[1]
        graph2Vec2 = ssrs[2]
        graph2Vec3 = ssrs[3]
        graph2Vec4 = ssrs[4]
        graph2Vec5 = ssrs[5]
        graph2Vec6 = ssrs[6]
        graph2Vec7 = ssrs[7]
        graph2Vec8 = ssrs[8]
    else
        graph1Vec1 = {}
        graph1Vec2 = {}
        graph2Vec1 = {}
        graph2Vec2 = {}
        graph2Vec3 = {}
        graph2Vec4 = {}
        graph2Vec5 = {}
        graph2Vec6 = {}
        graph2Vec7 = {}
        graph2Vec8 = {}
    end
end

local o =
	Def.ActorFrame {
    Name = "notChordDensityGraph", -- it's not the chord density graph
    InitCommand = function(self)
        self:SetUpdateFunction(HighlightUpdaterThing)
    end,
	OnCommand = function(self)
        self:xy(plotX, plotY)
    end,
    OffCommand = function(self)
        self:playcommand("CalcInfoOff")
    end,
    CalcInfoOnMessageCommand = function(self)
        updateCoolStuff()
        self:visible(true)
        enabled = true
        SCREENMAN:GetTopScreen():GetMusicWheel():visible(false)
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end,
    CalcInfoOffMessageCommand = function(self)
        self:visible(false)
        enabled = false
        SCREENMAN:GetTopScreen():GetMusicWheel():visible(true)
    end,
    CurrentStepsP1ChangedMessageCommand = function(self)
        if not enabled then return end
        updateCoolStuff()
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("CurrentStepsP1Changed")
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
    Name = "G2BG",
    InitCommand = function(self)
        self:y(plotHeight + 5)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
    end,
    DoTheThingCommand = function(self)
        self:visible(song ~= nil)
    end,
    HighlightCommand = function(self)
		local bar = self:GetParent():GetChild("Seek2")
        local txt = self:GetParent():GetChild("Seektext2")
        local bg = self:GetParent():GetChild("Seektext2BG")
        if isOver(self) then
            local mx = INPUTFILTER:GetMouseX()
            local ypos = INPUTFILTER:GetMouseY() - self:GetParent():GetY()
            
            local w = self:GetZoomedWidth() * self:GetParent():GetTrueZoom()
            local leftEnd = self:GetTrueX() - (self:GetHAlign() * w)
            local rightEnd = self:GetTrueX() + w - (self:GetHAlign() * w)
            local perc = (mx - leftEnd) / (rightEnd - leftEnd)
            local goodXPos = -plotWidth/2 + perc * plotWidth

			bar:visible(true)
            txt:visible(true)
            bg:visible(true)
			bar:x(goodXPos)
			txt:x(goodXPos - 4)
            txt:y(ypos)
            bg:zoomto(txt:GetZoomedWidth() + 6, txt:GetZoomedHeight() + 6)
            bg:x(goodXPos)
            bg:y(ypos + 3)
            local index = convertPercentToIndex(perc)
            local ovrl = ssrs[1][index]
            local strm = ssrs[2][index]
            local js = ssrs[3][index]
            local hs = ssrs[4][index]
            local stam = ssrs[5][index]
            local jack = ssrs[6][index]
            local chjk = ssrs[7][index]
            local tech = ssrs[8][index]
            if ovrl == nil then
                txt:settext("")
            else
                txt:settextf("Percent: %5.4f\nOverall: %.2f\nStream: %.2f\nJumpstream: %.2f\nHandstream: %.2f\nStamina: %.2f\nJackspeed: %.2f\nChordjack: %.2f\nTechnical: %.2f", (ssrLowerBoundWife + (ssrUpperBoundWife-ssrLowerBoundWife)*perc)*100, ovrl, strm, js, hs, stam, jack, chjk, tech)
            end
		else
			bar:visible(false)
            txt:visible(false)
            bg:visible(false)
		end
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
        self:settext("")
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            self:settextf("Upper Bound: %.4f\nLower Bound: %.4f", highest-1, lowest+1)
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
                local x = fitX(i, finalSecond)
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
                local x = fitX(i, finalSecond)
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

--[[
-- bottom graph overall line
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

            for i = 1, #graph2Vec1 do
                local x = fitX(i, #graph2Vec1)
                local y = fitY2(graph2Vec1[i])
                local cullur = color("#aaaaaa")
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}]]

-- bottom graph stream line
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

            for i = 1, #graph2Vec2 do
                local x = fitX(i, #graph2Vec2)
                local y = fitY2(graph2Vec2[i])
                local cullur = color("#7d6b91")

                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph js line
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

            for i = 1, #graph2Vec3 do
                local x = fitX(i, #graph2Vec3)
                local y = fitY2(graph2Vec3[i])
                local cullur = color("#8481db")
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph hs line
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

            for i = 1, #graph2Vec4 do
                local x = fitX(i, #graph2Vec4)
                local y = fitY2(graph2Vec4[i])
                local cullur = color("#995fa3")
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph stamina line
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

            for i = 1, #graph2Vec5 do
                local x = fitX(i, #graph2Vec5)
                local y = fitY2(graph2Vec5[i])
                local cullur = color("#f2b5fa")
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph jackspeed line
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

            for i = 1, #graph2Vec6 do
                local x = fitX(i, #graph2Vec6)
                local y = fitY2(graph2Vec6[i])
                local cullur = color("#6c969d")
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph chordjack line
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

            for i = 1, #graph2Vec7 do
                local x = fitX(i, #graph2Vec7)
                local y = fitY2(graph2Vec7[i])
                local cullur = color("#a5f8d3")
                setOffsetVerts(verts, x, y, cullur)
            end
            
            self:SetVertices(verts)
            self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
        else
            self:visible(false)
        end
    end
}

-- bottom graph technical line
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

            for i = 1, #graph2Vec8 do
                local x = fitX(i, #graph2Vec8)
                local y = fitY2(graph2Vec8[i])
                local cullur = color("#b0cec2")
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

o[#o + 1] = Def.Quad {
    Name = "Seektext2BG",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("0,0,0,.4")):zoomto(20,20)
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "Seektext2",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.4)
    end
}

o[#o + 1] = Def.Quad {
    Name = "Seek2",
    InitCommand = function(self)
        self:y(plotHeight+5)
        self:zoomto(1, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end
}

return o