-- this is the layer that sits behind the prompt that shows up in the key config screen

return Def.Quad {
	InitCommand = function(self)
		self:FullScreen():Center():diffuse(color("#000000"))
	end
}
