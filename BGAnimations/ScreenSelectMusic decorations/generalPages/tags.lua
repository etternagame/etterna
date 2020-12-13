local t = Def.ActorFrame {
    Name = "TagsPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.tagstabindex then
                self:z(2)
                self:smooth(0.2)
                self:diffusealpha(1)
            else
                self:z(-1)
                self:smooth(0.2)
                self:diffusealpha(0)
            end
        end
    end,
}

local ratios = {
    
}

local actuals = {
    
}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

return t