PackList = {}

local getSizePropName = "GetSize"
local getAvgDiffPropName = "GetAvgDifficulty"
local getNamePropName = "GetName"

function PackList:GetPackTable()
    return self.packs
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
function PackList:FilterAndSearch(name, avgMin, avgMax, sizeMin, sizeMax)
    self.packs = 
        filter(
        function(x)
            local d = x[getAvgDiffPropName](x)
            local n = x[getNamePropName](x)
            local s = x[getSizePropName](x)
            local valid = string.find(string.lower(n), string.lower(name))
            if d > 0 then
                if avgMin > 0 then
                    valid = valid and d > avgMin
                end
                if avgMax > 0 then
                    valid = valid and d < avgMax
                end
            end
            if s > 0 then
                if sizeMin > 0 then
                    valid = valid and s > sizeMin
                end
                if sizeMax > 0 then
                    valid = valid and s < sizeMax
                end
            end
            return valid
        end,
        self.allPacks
    )
    return self
end
function PackList:SetFromAll()
    local allPacks = DLMAN:GetAllPacks()
    self.allPacks = allPacks
    self.packs = DeepCopy(allPacks)
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
