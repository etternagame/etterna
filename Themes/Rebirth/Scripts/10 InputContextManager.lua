--- Rebirth Input Context Manager
-- this is a singleton
-- manages the state of keyboard input contexts
-- declare all specific context groups here
-- provides toggles for contexts, etc
-- .. at the end of the day input contexts are still hard and this makes like 1 thing easier
-- @module Rebirth_10_InputContextManager
CONTEXTMAN = {}

--[[
-- how to use:

... OnCommand/BeginCommand
CONTEXTMAN:RegisterToContextSet(scrName, groupName)

... Any Time
CONTEXTMAN:CheckContextSet(scrName, groupName)

... Any Time
CONTEXTMAN:ToggleContextSet(scrName, groupName)

micromanaging functions available later if needed

]]

-- reset all info to defaults
function CONTEXTMAN.Reset(self)
    print("CONTEXTMAN Reset")
    -- a screen name to a context set
    -- each context set contains a list of strings (usually actor names but can be anything)
    -- each context has an 'enabled' field which tells if it is on or not
    self.ScreenToContext = {}

    -- if set to true, all contexts are deferred
    -- that is, we not "focused" on all contexts
    -- so things depending on the context being true would fail
    self.ContextIgnored = false
end
CONTEXTMAN:Reset()

-- emplace a context group into the context set for the screen
-- if group already exists, does not reset it
function CONTEXTMAN.RegisterContext(self, screen, group)
    if self.ScreenToContext[screen] == nil then
        self.ScreenToContext[screen] = {}
    end
    if self.ScreenToContext[screen][group] == nil then
        self.ScreenToContext[screen][group] = {enabled = true}
    end
end

-- dont know why you would want to do the reverse of the above function
-- but here it is anyways
function CONTEXTMAN.UnRegisterContext(self, screen, group)
    print("CONTEXTMAN UnRegister - "..screen.." - "..group)
    if self.ScreenToContext[screen] ~= nil then
        self.ScreenToContext[screen][group] = nil
    end
end

-- reset a context set
function CONTEXTMAN.ClearContextSet(self, screen, group)
    print("CONTEXTMAN ClearContextSet - "..screen.." - "..group)
    if self.ScreenToContext[screen] == nil then
        self.ScreenToContext[screen] = {}
    end
    self.ScreenToContext[screen][group] = {enabled = true}
end

-- alias for forgetting function names
function CONTEXTMAN.ResetContextSet(self, screen, group)
    self:ClearContextSet(screen, group)
end

-- add a name to a context set
function CONTEXTMAN.RegisterToContextSet(self, screen, group, name)
    self:RegisterContext(screen, group)
    self.ScreenToContext[screen][group][name] = true
end

-- return if a group exists and is enabled
function CONTEXTMAN.CheckContextSet(self, screen, group)
    if self.ContextIgnored then return false end

    if self.ScreenToContext[screen] ~= nil then
        if self.ScreenToContext[screen][group] ~= nil then
            return self.ScreenToContext[screen][group].enabled
        end
    end
    return false
end

-- set a single context group as enabled
function CONTEXTMAN.SetFocusedContextSet(self, screen, group)
    print("CONTEXTMAN Focus Switch - "..screen.." - "..group)
    self:RegisterContext(screen, group)
    for _, g in pairs(self.ScreenToContext[screen]) do
        g.enabled = false
    end
    self.ScreenToContext[screen][group].enabled = true
end

-- return whether or not a given screen and name is "in context"
-- that is, we are "focused" on it
function CONTEXTMAN.CheckContext(self, screen, group, name)
    if self.ContextIgnored then return false end
    if not self:CheckContextSet(screen, group) then return false end
    
    return self.ScreenToContext[screen][group][name] ~= nil and self.ScreenToContext[screen][group][name] == true
end

-- toggle a context set
-- if given a state boolean then forces the enabled state to that
function CONTEXTMAN.ToggleContextSet(self, screen, group, state)
    -- taking advantage of existing functions
    -- this returns false if false OR nonexistent
    if self.ScreenToContext[screen][group] == nil then
        -- uh.... huh...
        if state == nil then state = true end
        self.ScreenToContext[screen][group] = {enabled = state}
    else
        if state == nil then state = not self.ScreenToContext[screen][group].enabled end
        self.ScreenToContext[screen][group].enabled = state
    end
end