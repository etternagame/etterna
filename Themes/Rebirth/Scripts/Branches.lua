--- Rebirth Branches
-- @module Rebirth_Branches
Branch.AfterSelectStyle = function()
    return "ScreenSelectMusic"
end
Branch.ExitingSyncMachine = function()
    -- HACK: ALSO USING THIS FOR ScreenTestInput DONT PANIC

    -- when exiting ScreenGameplaySyncMachine we need to go back where we came from
    -- ideally, entry is only from 2 points, ScreenSelectMusic or ScreenInputOptions[Sub]
    -- so if it is provided, use the ScreenSelectMusic exit direction instead
    -- another note: know that for some brilliant reason gameplay calls this twice in a row so resetting isnt exactly the brightest idea
    --  big brain: reset it after running more than once only ... i guess
    --  im not going to put any thought into why this could be really really bad
    local o = ""
    if SCUFF ~= nil and SCUFF.screenAfterSyncMachine ~= nil and SCUFF.screenAfterSyncMachine_iter ~= nil then
        o = SCUFF.screenAfterSyncMachine
        SCUFF.screenAfterSyncMachine_iter = SCUFF.screenAfterSyncMachine_iter + 1
    else
        o = "ScreenOptionsInputSub"
    end
    -- reset it to a default value
    if SCUFF ~= nil and SCUFF.screenAfterSyncMachine_iter >= 2 then
        SCUFF.screenAfterSyncMachine = "ScreenOptionsInputSub"
        SCUFF.screenAfterSyncMachine_iter = 0
    end
    return o
end
Branch.ExitingHelpMenu = function()
    local o = "ScreenTitleMenu"
    if SCUFF ~= nil and SCUFF.helpmenuBackout ~= nil then
        o = SCUFF.helpmenuBackout
    end
    return o
end