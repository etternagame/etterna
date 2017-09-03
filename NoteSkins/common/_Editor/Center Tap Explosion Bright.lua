return Def.Sprite {
	Texture=NOTESKIN:GetPath( '_center', 'explosion' );
	W1Command=function(self)
		self:blend("BlendMode_Add"):finishtweening():diffusealpha(0.2):zoom(0.6):linear(0.1):diffusealpha(0):zoom(0.8)
	end;
	W2Command=function(self)
		self:blend("BlendMode_Add"):finishtweening():diffusealpha(0.2):zoom(0.6):linear(0.1):diffusealpha(0):zoom(0.8)
	end;
	W3Command=function(self)
		self:blend("BlendMode_Add"):finishtweening():diffusealpha(0.2):zoom(0.6):linear(0.1):diffusealpha(0):zoom(0.8)
	end;
	W4Command=function(self)
		self:blend("BlendMode_Add"):finishtweening():diffusealpha(0.2):zoom(0.6):linear(0.1):diffusealpha(0):zoom(0.8)
	end;
	W5Command=function(self)
		self:blend("BlendMode_Add"):finishtweening():diffusealpha(0.2):zoom(0.6):linear(0.1):diffusealpha(0):zoom(0.8)
	end;
};