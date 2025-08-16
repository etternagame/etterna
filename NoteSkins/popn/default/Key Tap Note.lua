-- i keep telling myself this is the funniest thing i've seen.
local button = Var "Button"

local t = Def.ActorFrame {
	Def.Sprite {
		-- because there's not really a point of putting snaps on it,
		-- if you actually want it to have snaps, use 0.74.0+ or
		-- contact me.
		Texture = "_bar",
		Frame0000 = 0,
		Delay0000 = 1,

		InitCommand = function(self)
			-- have to zoom these by a half because notefield is a fcuk.
			self:diffuse(color(colors[button])):zoomx(0.5)
		end
	},
}
return t
