-- additional utility functions that could be used in places but not overshadowing other util functions


-- return the width of the widest child
-- ASSUMING all immediate children are not ActorFrames
-- this can be made into a recursive function but some behavior indicates that may not be a good idea
function getLargestChildWidth(actorFrame)
    local largest = 0
    if actorFrame == nil then
        return largest
    end

    for name, child in pairs(actorFrame:GetChildren()) do
        local w = child:GetZoomedWidth()
        if w > largest then
            largest = w
        end
    end

    return largest
end

-- recursively print the names of all children of this actorframe
function nameAllChildren(actorFrame)
    local s = actorFrame:GetName()
    actorFrame:RunCommandsRecursively(
        function(self)
            s = s .. "\n" .. self:GetName()
        end
    )
    ms.ok(s)
end

-- find the height and width while maintaining aspect ratio
function getHWKeepAspectRatio(h, w, ratio)
    local he = h / math.sqrt(ratio * ratio + 1)
    local we = w / math.sqrt(1 / (ratio * ratio) + 1)

    return he, we
end

-- string split, return a list given a string and a separator between items
function strsplit(given, separator)
    if separator == nil then
        separator = "%s" -- whitespace
    end
    local t = {}
    for str in string.gmatch(given, "([^"..separator.."]+)") do
        table.insert(t, str)
    end
    return t
end