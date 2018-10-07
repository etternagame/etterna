local File, Width, Height = ...
assert(File)
assert(Width)
assert(Height)

local FullFile = THEME:GetPathB("", "_frame files 3x3/" .. File)
local Frame = LoadActor(FullFile)
return Def.ActorFrame {
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(0):pause():horizalign(right):vertalign(bottom):x(-Width / 2):y(-Height / 2)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(1):pause():zoomtowidth(Width):vertalign(bottom):zoomtowidth(Width):y(-Height / 2)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(2):pause():horizalign(left):vertalign(bottom):x(Width / 2):y(-Height / 2)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(3):pause():horizalign(right):x(-Width / 2):zoomtoheight(Height)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(4):pause():zoomtowidth(Width):zoomtoheight(Height)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(5):pause():horizalign(left):x(Width / 2):zoomtoheight(Height)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(6):pause():horizalign(right):vertalign(top):x(-Width / 2):y(Height / 2)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(7):pause():zoomtowidth(Width):vertalign(top):zoomtowidth(Width):y(Height / 2)
			end
		},
	Frame ..
		{
			InitCommand = function(self)
				self:setstate(8):pause():horizalign(left):vertalign(top):x(Width / 2):y(Height / 2)
			end
		}
}
