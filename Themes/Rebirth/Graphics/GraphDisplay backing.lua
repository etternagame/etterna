-- represents the background for the entire life graph

return Def.Quad {
    Name = "Backing",
    InitCommand = function(self)
        self:diffusealpha(1)
        registerActorToColorConfigElement(self, "evaluation", "LifeGraphBackground")
    end
}
