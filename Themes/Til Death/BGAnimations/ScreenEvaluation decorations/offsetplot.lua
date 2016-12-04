local plotWidth, plotHeight = 400,120
local plotX, plotY = SCREEN_WIDTH - 9 - plotWidth/2, SCREEN_HEIGHT - 56 - plotHeight/2
local dotDims, plotMargin = 2, 4
local maxOffset = 180
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
		return Def.Quad{InitCommand=cmd(xy,fitX(nr),fitY(184);zoomto,dotDims,dotDims;diffuse,offsetToJudgeColor(dv/1000);valign,0)}
	end
	return Def.Quad{InitCommand=cmd(xy,fitX(nr),fitY(dv);zoomto,dotDims,dotDims;diffuse,offsetToJudgeColor(dv/1000))}
end

local o = Def.ActorFrame{InitCommand=cmd(xy,plotX,plotY)}
-- Center Bar
o[#o+1] = Def.Quad{InitCommand=cmd(zoomto,plotWidth+plotMargin,1;diffuse,color("0.3,0.3,0.3,0.3");diffusealpha,0.8)}
-- Background
o[#o+1] = Def.Quad{InitCommand=cmd(zoomto,plotWidth+plotMargin,plotHeight+plotMargin;diffuse,color("0.05,0.05,0.05,0.05");diffusealpha,0.8)}
-- Plot Dots
for i=1,#devianceTable do
	o[#o+1] = plotOffset(td:GetElapsedTimeFromNoteRow(nrt[i]), dvt[i])
end
-- Early/Late markers
o[#o+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,-plotWidth/2,-plotHeight/2+2;settext,"Late (+180ms)";zoom,0.35;halign,0;valign,0)}
o[#o+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,-plotWidth/2,plotHeight/2-2;settext,"Early (-180ms)";zoom,0.35;halign,0;valign,1)}
return o

