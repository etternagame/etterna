-- this is a singleton
-- manages the state of keyboard input contexts
-- declare all specific screen contexts desired here
-- provides toggles for contexts and global on off switches

CONTEXTMAN = {}

-- reset all info to defaults
function CONTEXTMAN.Reset(self)
    -- a screen name to a context
    self.ScreenToContext = {}

    -- if set to true, all contexts are deferred
    -- that is, we not "focused" on all contexts
    -- so things depending on the context being true would fail
    self.ContextIgnored = false
end

-- emplace a context (ActorFrame) into the context for the screen
-- the given actor
function CONTEXTMAN.RegisterContext(self, screen, actorName)
    if self.ScreenToContext[screen] == nil then
        self.ScreenToContext[screen] = {}
    end
    self.ScreenToContext[screen][actorName] = true
end

-- dont know why you would want to do the reverse of the above function
-- but here it is anyways
function CONTEXTMAN.UnRegisterContext(self, screen, actorName)
    if self.ScreenToContext[screen] ~= nil then
        self.ScreenToContext[screen][actorName] = nil
    end
end

-- return whether or not a given screen and actor is "in context"
-- that is, we are "focused" on it
function CONTEXTMAN.CheckContext(self, screen, actorName)
    if self.ContextIgnored then return false end

    if self.ScreenToContext[screen] ~= nil then
        return self.ScreenToContext[screen][actorName] ~= nil and self.ScreenToContext[screen][actorName] == true
    end
    return false
end

-- toggle a context; works if not registered
function CONTEXTMAN.ToggleContext(self, screen, actorName)
    -- taking advantage of existing functions
    -- this returns false if false OR nonexistent
    if self:CheckContext(screen, actorName) then
        self.ScreenToContext[screen][actorName] = false
    else
        -- this sets true if false OR nonexistent
        CONTEXTMAN:RegisterContext(screen, actorName)
    end
end