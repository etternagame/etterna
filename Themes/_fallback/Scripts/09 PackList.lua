PackList = {}

local getSizePropName = "GetSize"
local getAvgDiffPropName = "GetAvgDifficulty"
local getNamePropName = "GetName"

function PackList:GetPackTable()
    SCREENMAN:SystemMessage(tostring(#(self.packs)))
    return self.packs
end
local foldr = function(func, val, tbl)
    for i, v in pairs(tbl) do
        val = func(val, v)
    end
    return val
end
function PackList:GetTotalSumByProp(propName)
    return foldr(
        function(sum, x)
            return sum + x[propName](x)
        end,
        0,
        self.packs
    )
end
function PackList:GetTotalSize()
    return self:GetTotalSumByProp(getSizePropName)
end
function PackList:GetAvgDiff()
    return self:GetTotalSumByProp(getAvgDiffPropName)
end
function PackList:SetFromCoreBundle(bundleName)
    local bundle = DLMAN:GetCoreBundle(bundleName)
    self.packs = bundle
    return self
end
function PackList:SortByProp(propName)
    if self.lastsort == propName then
        self.asc = not self.asc
    end
    self.lastsort = propName
    local asc = self.asc
    table.sort(
        self.packs,
        asc and function(a, b)
                return a[propName](a) > b[propName](b)
            end or function(a, b)
                return a[propName](a) < b[propName](b)
            end
    )
    return self
end
function PackList:SortByName()
    return self:SortByProp(getNamePropName)
end
function PackList:SortByDiff()
    return self:SortByProp(getAvgDiffPropName)
end
function PackList:SortBySize()
    return self:SortByProp(getSizePropName)
end
local function filter(func, tbl)
    local newtbl = {}
    for i, v in pairs(tbl) do
        if func(v) then
            newtbl[i] = v
        end
    end
    return newtbl
end
function PackList:FilterAndSearch(name, avgMin, avgMax, sizeMin, sizeMax)
    self.packs =
        filter(
        function(x)
            local d = x[getAvgDiffPropName]:x()
            local n = x[getNamePropName]:x()
            local s = x[getSizePropName]:x()
            return n == name and ((d > avgMin and d < avgMax) or d <= 0) and ((s > sizeMin and d < sizeMax) or s <= 0)
        end,
        self.packs
    )
    return self
end
function PackList:SetFromAll()
    self.packs = DLMAN:GetAllPacks()
    return self
end
function PackList:new()
    local packlist = {}
    packlist.asc = true
    packlist.lastsort = nil
    packlist.packs = {}
    setmetatable(
        packlist,
        {
            __index = function(t, i)
                return t.packs[i] or PackList[i]
            end
        }
    )
    packlist:SetFromAll()
    return packlist
end
