local t = Def.ActorFrame{	-- splashy thing when you first start a song
	Name = "Splashy",		
	DootCommand=function(self)
		self:RemoveAllChildren()
	end,
	
	Def.Quad{
		Name="DestroyMe",
		InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_CENTER_Y-40;zoomto,400,50;diffuse,getMainColor('highlight');fadeleft,0.4;faderight,0.4;diffusealpha,0),
		OnCommand=cmd(smooth,0.5;diffusealpha,0.7;sleep,1;smooth,0.3;smooth,0.4;diffusealpha,0)
	},
	
	LoadFont("Common Large") .. {
		Name="DestroyMe2",
		InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_CENTER_Y-50;zoom,0.5;diffusealpha,0;maxwidth,400/0.45),
		BeginCommand=cmd(settext,GAMESTATE:GetCurrentSong():GetDisplayMainTitle()),
		OnCommand=cmd(smooth,0.5;diffusealpha,1;sleep,1;smooth,0.3;smooth,0.4;diffusealpha,0)
	},
	
	LoadFont("Common Normal") .. {
		Name="DestroyMe3",
		InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_CENTER_Y-35;zoom,0.45;diffusealpha,0;maxwidth,400/0.45),
		BeginCommand=cmd(settext,GAMESTATE:GetCurrentSong():GetDisplaySubTitle()),
		OnCommand=cmd(smooth,0.5;diffusealpha,1;sleep,1;smooth,0.3;smooth,0.4;diffusealpha,0)
	},
	
	LoadFont("Common Normal") .. {
		Name="DestroyMe4",
		InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_CENTER_Y-25;zoom,0.45;;diffusealpha,0),
		BeginCommand=cmd(settext,GAMESTATE:GetCurrentSong():GetDisplayArtist()),
		OnCommand=cmd(smooth,0.5;diffusealpha,1;sleep,1;smooth,0.3;smooth,0.4;diffusealpha,0;queuecommand,"Doot"),
		DootCommand=function(self)
			self:GetParent():queuecommand("Doot")
		end
	}
}
return t