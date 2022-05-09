-- represents the background for the entire combo graph

return Def.Quad {
    Name = "Backing",
    InitCommand = function(self)
        self:diffusealpha(0.9)
        registerActorToColorConfigElement(self, "evaluation", "ComboGraphBackground")
    end
}
