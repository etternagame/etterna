local File, Width = ...
assert(File)
assert(Width)

local FullFile = THEME:GetPathB("", "_frame files 3x1/" .. File)
local Frame = LoadActor(FullFile)

return Def.ActorFrame {
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(0):pause():horizalign(right):x(-Width / 2)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(1):pause():zoomtowidth(Width)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(2):pause():horizalign(left):x(Width / 2)
			end
		}
}
