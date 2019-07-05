local finalSecond

local plotWidth, plotHeight = 800, 240
local plotX, plotY = SCREEN_WIDTH - 9 - plotWidth / 2, SCREEN_HEIGHT - plotHeight / 2 - 156
local dotDims, plotMargin = 2, 4
local maxOffset = 40
local baralpha = 0.2
local bgalpha = 1
local textzoom = 0.35
local song

local function fitX(x) -- Scale time values to fit within plot width.
	if finalSecond == 0 then
		return 0
	end
	return x / finalSecond * plotWidth - plotWidth / 2
end

local function fitY(y) -- Scale diff values to fit within plot height
	return -1 * y / maxOffset * plotHeight
end

local o =
	Def.ActorFrame {
	OnCommand = function(self)
		self:xy(plotX, plotY)
		MESSAGEMAN:Broadcast("JudgeDisplayChanged") -- prim really handled all this much more elegantly
	end

}
-- Background
o[#o + 1] =
	Def.Quad {
        CurrentStepsP1ChangedMessageCommand = function(self)
            song = GAMESTATE:GetCurrentSong()
            if song then
                self:visible(true)
		        self:zoomto(plotWidth + plotMargin, plotHeight + plotMargin):diffuse(color("0.05,0.05,0.05,0.05")):diffusealpha(
			    bgalpha
                )
            else
                self:visible(false)
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
o[#o + 1] =
	Def.ActorMultiVertex {
        CurrentStepsP1ChangedMessageCommand = function(self)
            song = GAMESTATE:GetCurrentSong()
            if song then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

                self:visible(true)
                finalSecond = GAMESTATE:GetCurrentSong():GetLastSecond() * 2
                local steeps = GAMESTATE:GetCurrentSteps(PLAYER_1)
                local bloot = {}
		        local verts = {}
                bloot = steeps:DootSpooks()
		        for i = 1, #bloot do
    			    local x = fitX(i)
			        local y = fitY(bloot[i])
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

return o