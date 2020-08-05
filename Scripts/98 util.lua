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