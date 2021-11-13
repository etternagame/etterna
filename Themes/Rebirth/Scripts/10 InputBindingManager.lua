-- this provides various lua-based utility and state related to key binding

INPUTBINDING = {
    -- these values are indexed at 0, not 1
    maxColumn = 4, -- the 5th column (yes there are actually many more columns to bind to)
    maxPlayer = 1, -- the right player/controller
    defaultColumn = 2, -- the column which is usually the default internally (in key config this is the 3rd column)
}

-- get all MenuButtons which can be mapped that are relevant to directions
-- 
function DirectionalMenuButtonsToMap()
    return {
        "MenuLeft",
        "MenuDown",
        "MenuUp",
        "MenuRight",
    }
end

-- gets all GameButtons or MenuButtons which can be mapped
function GetButtonsToMap(isMenu)
    local bt = {}
    if isMenu then
        -- does not include the buttons DirectionalMenuButtonsToMap
        bt = INPUTMAPPER:GetMenuButtonsToMap()
    else
        bt = INPUTMAPPER:GetGameButtonsToMap()
    end
    return bt
end

-- makes sure that if the arrow keys are not bound to gameplay buttons that they are bound to the menu buttons
function INPUTBINDING.MakeSureMenuIsNavigable(self)
    local gb = GetButtonsToMap(false)

    local mb = DirectionalMenuButtonsToMap()
    local keys = {"Key_left", "Key_down", "Key_up", "Key_right"}

    -- is this key bound to a game button
    -- if its bound to a menu button i do not care. rebind it anyways
    local function isBound(key)
        for i = 1, #gb do
            for bindingColumn = 0, self.maxColumn do
                if INPUTMAPPER:GetButtonMapping(gb[i], 0, bindingColumn) == key then
                    return true
                end
            end
        end
        return false
    end

    for i = 1, #mb do
        if not isBound(keys[i]) then
            INPUTMAPPER:SetInputMap(keys[i], mb[i], self.defaultColumn, 0)
        end
    end
end

-- literally removes every key binding for gameplay or menu
-- player 1 and player 2
function INPUTBINDING.RemoveAllKeyBindings(self, isMenu)
    print("INPUTBINDING RemoveAllKeyBindings - isMenu "..tostring(isMenu))
    local bt = GetButtonsToMap(isMenu)
    for i = 1, #bt do
        for player = 0, self.maxPlayer do
            for bindingColumn = 0, self.maxColumn do
                INPUTMAPPER:SetInputMap("", bt[i], bindingColumn, player)
            end
        end
    end
end

-- does the above except ignores the default column
-- does not set any new defaults, blanks stay blank
-- (default should be column 2, the 3rd column)
function INPUTBINDING.RemoveDoubleBindings(self, isMenu)
    print("INPUTBINDING RemoveDoubleBindings - isMenu "..tostring(isMenu))
    local bt = GetButtonsToMap(isMenu)
    local alreadymapped = {}
    for i = 1, #bt do
        for player = 0, self.maxPlayer do
            local b = INPUTMAPPER:GetButtonMapping(bt[i], player, self.defaultColumn)
            if b == nil then
                b = "nil"
            end
            alreadymapped[#alreadymapped+1] = b
        end
    end
    self:RemoveAllKeyBindings(isMenu)
    local ind = 1
    for i = 1, #bt do
        for player = 0, self.maxPlayer do
            local b = alreadymapped[ind]
            INPUTMAPPER:SetInputMap(b, bt[i], self.defaultColumn, player)
            ind = ind + 1
        end
    end
end

-- return whether or not this set of key bindings is missing the defaults
-- useful only for this theme where the default column is required to be used
function INPUTBINDING.IsAnyDefaultBindingMissing(self, isMenu)
    local bt = GetButtonsToMap(isMenu)
    for i = 1, #bt do
        for p = 0, self.maxPlayer do
            local buttonmapped = INPUTMAPPER:GetButtonMapping(bt[i], p, self.defaultColumn)
            if buttonmapped == nil then return false end -- bad
        end
    end
    return true -- okay
end