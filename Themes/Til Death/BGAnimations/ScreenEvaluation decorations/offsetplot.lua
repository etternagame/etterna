local ts = PREFSMAN:GetPreference("TimingWindowScale") 
local plotWidth, plotHeight = 400,120
local plotX, plotY = SCREEN_WIDTH - 9 - plotWidth/2, SCREEN_HEIGHT - 56 - plotHeight/2
local dotDims, plotMargin = 2, 4
local maxOffset = 180*ts
local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1)
local dvt = pss:GetOffsetVector()
local nrt = pss:GetNoteRowVector()
local td = GAMESTATE:GetCurrentSteps(PLAYER_1):GetTimingData()
local finalSecond = GAMESTATE:GetCurrentSong(PLAYER_1):GetLastSecond()

local function fitX(x)	-- Scale time values to fit within plot width.
	return x/finalSecond*plotWidth - plotWidth/2
end

local function fitY(y)	-- Scale offset values to fit within plot height
	return -1*y/maxOffset*plotHeight/2
end

local function plotOffset(nr,dv)
	if dv == 1000 then 	-- 1000 denotes a miss for which we use a different marker
		return Def.Quad{InitCommand=cmd(xy,fitX(nr),fitY(ts*184);zoomto,dotDims,dotDims;diffuse,offsetToJudgeColor(dv/1000);valign,0)}
	end
	return Def.Quad{InitCommand=cmd(xy,fitX(nr),fitY(dv);zoomto,dotDims,dotDims;diffuse,offsetToJudgeColor(dv/1000))}
end

local o = Def.ActorFrame{InitCommand=cmd(xy,plotX,plotY)}
-- Center Bar
o[#o+1] = Def.Quad{InitCommand=cmd(zoomto,plotWidth+plotMargin,1;diffuse,byJudgment("TapNoteScore_W1"))}
local fantabars = {22.5, 45, 90, 135}
local bantafars = {"TapNoteScore_W2", "TapNoteScore_W3", "TapNoteScore_W4", "TapNoteScore_W5"}
for i=1, #fantabars do 
	o[#o+1] = Def.Quad{InitCommand=cmd(y, fitY(ts*fantabars[i]); zoomto,plotWidth+plotMargin,1;diffuse,byJudgment(bantafars[i]))}
	o[#o+1] = Def.Quad{InitCommand=cmd(y, fitY(-ts*fantabars[i]); zoomto,plotWidth+plotMargin,1;diffuse,byJudgment(bantafars[i]))}
end
-- Background
o[#o+1] = Def.Quad{InitCommand=cmd(zoomto,plotWidth+plotMargin,plotHeight+plotMargin;diffuse,color("0.05,0.05,0.05,0.05");diffusealpha,0.8)}
-- Convert noterows to timestamps
local wuab = {}
for i=1,#nrt do
	wuab[i] = td:GetElapsedTimeFromNoteRow(nrt[i])
end
-- Plot Dots
for i=1,#devianceTable do
	o[#o+1] = plotOffset(wuab[i], dvt[i])
end
-- Early/Late markers
o[#o+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,-plotWidth/2,-plotHeight/2+2;settextf,"Late (+%ims)", ts*180;zoom,0.35;halign,0;valign,0)}
o[#o+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,-plotWidth/2,plotHeight/2-2;settextf,"Early (-%ims)", ts*180;zoom,0.35;halign,0;valign,1)}




-- graph garbage

local wuaboffset = {}
for i=1,#wuab do
	if dvt[i] ~= 1000 then
		wuaboffset[i] = wuab[i] + dvt[i]/1000
	else
		wuaboffset[i] = wuab[i] + .180
	end
end

table.sort(wuab)
table.sort(wuaboffset)

local pointstotal = 0
local pointsmax = 0
local curperc = 0
local wifescore = pss:GetWifeScore()

local function fitXGraph(x)	-- Scale time values to fit within plot width.
	return x/finalSecond*plotWidth
end
local function fitYGraph(y)	-- Scale percent values to fit within plot height
	return -1 * plotHeight/2*(y-wifescore)/(1-wifescore)
end

local verts = {}
for i=1,#wuab do
	pointstotal = pointstotal + pss:WifeScoreOffset(wuaboffset[i] - wuab[i])
	pointsmax = pointsmax + 2
	curperc = pointstotal/pointsmax
	verts[#verts+1] = {{fitXGraph(wuaboffset[i]),fitYGraph(curperc),0},Color.White}
end

o[#o+1] = Def.ActorMultiVertex{
	InitCommand=function(self)
		self:addx(-plotWidth/2)
		self:SetDrawState{Mode="DrawMode_LineStrip"}
	end,
	BeginCommand=function(self)
		self:SetVertices(verts)
		self:SetDrawState{First = 1, #verts}
	end,
}

return o