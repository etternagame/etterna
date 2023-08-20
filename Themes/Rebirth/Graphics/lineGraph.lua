local FUNC_NOOP = function(...) return 0 end

-- find the min and max of the values in a table with this format: { {x, y, ....}, {x, y, ....}, ....}
-- the given i is the index position of the value, 1 for x, etc
local function findminmax(t, i)
    if t == nil or #t == 0 then return 0 end
    local mi = t[1][i]
    local ma = t[1][i]
    for _,v in ipairs(t) do
        if v[i] < mi then mi = v[i] end
        if v[i] > ma then ma = v[i] end
    end
    return mi, ma
end

-- a scaling function which outputs a percentage based on a given scale
local function scale(x, lower, upper, scaledMin, scaledMax)
    local perc = (x - lower) / (upper - lower)
    return perc * (scaledMax - scaledMin) + scaledMin
end

local dotWidth = 0
local function addDotVerts(vt, x, y, c)
	vt[#vt + 1] = {{x - dotWidth, y + dotWidth, 0}, c}
end

local fitX = FUNC_NOOP
local fitY = FUNC_NOOP
local colorToUse = color("#FFFFFF")

return Def.ActorFrame {
    Name = "LineGraph",
    RegisterToGraphOwnerCommand = function(self, params)
        if self.registered then return end
        self.registered = true
        if self:GetParent().graphs == nil then self:GetParent().graphs = {} end
        local graphs = self:GetParent().graphs
        graphs[params.name] = self
    end,
    DisplayGraphCommand = function(self, params)
        if params == nil or params.data == nil or #params.data == 0 then
            self:playcommand("DrawNothing")
            return
        end

        local fullWidth = self:GetParent():GetChild("BG"):GetZoomedWidth()
        local xmin, xmax = params.xmin, params.xmax
        if xmin == nil or xmax == nil then
            local a,b = findminmax(params.data, 1)
            if xmin == nil then xmin = a end
            if xmax == nil then xmax = b end
        end
        -- THIS IS WRITTEN ASSUMING THE GRAPH IS halign(0)
        fitX = function(xval)
            return (xval / xmax) * fullWidth
        end

        local fullHeight = self:GetParent():GetChild("BG"):GetZoomedHeight()
        local ymin, ymax = params.ymin, params.ymax
        if ymin == nil or ymax == nil then
            local a,b = findminmax(params.data, 2)
            if ymin == nil then ymin = a end
            if ymax == nil then ymax = b end
        end
        -- THIS IS WRITTEN ASSUMING THE GRAPH IS valign(0.5) / default
        fitY = function(yval)
            return -1 * scale(yval, ymin, ymax, 0, 1) * fullHeight + fullHeight/2
        end

        colorToUse = params.color or color("#FFFFFF")
        self:playcommand("ActuallyDisplay", params)
    end,

    Def.ActorMultiVertex {
        Name = "Line",
        DrawNothingCommand = function(self)
            self:SetVertices({})
            self:SetDrawState({Mode = "DrawMode_Quads", First = 1, Num = 0})
            self:visible(false)
        end,
        ActuallyDisplayCommand = function(self, params)
            self:playcommand("DrawNothing")
            if params == nil or params.data == nil or #params.data == 0 then return end

            self:visible(true)

            local data = params.data
            local verts = {}

            if #data == 1 then data[#data+1] = data[1] end

            for i, pt in ipairs(data) do
                local x = pt[1]
                local y = pt[2]
                local xpos = fitX(x)
                local ypos = fitY(y)
                addDotVerts(verts, xpos, ypos, colorToUse)
            end

            self:SetVertices(verts)
            self:SetDrawState({Mode = "DrawMode_LineStrip", First = 1, Num = #verts})
        end,
    },
}

