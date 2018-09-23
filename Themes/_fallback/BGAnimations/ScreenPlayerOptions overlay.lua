local t = Def.ActorFrame {}

local NSPreviewSize = 0.5
local NSPreviewX = 35
local NSPreviewY = 125
local NSPreviewXSpan = 35
local NSPreviewReceptorY = -30
local OptionRowHeight = 35
local NoteskinRow = 0

function NSkinPreviewWrapper(dir, ele)
	return Def.ActorFrame {
		InitCommand = function(self)
			self:zoom(NSPreviewSize)
		end,
		LoadNSkinPreview("Get", dir, ele, PLAYER_1)
	}
end
t[#t + 1] =
	Def.ActorFrame {
	OnCommand = function(self)
		self:xy(NSPreviewX, NSPreviewY)
		for i = 0, SCREENMAN:GetTopScreen():GetNumRows() - 1 do
			if SCREENMAN:GetTopScreen():GetOptionRow(i) and SCREENMAN:GetTopScreen():GetOptionRow(i):GetName() == "NoteSkins" then
				NoteskinRow = i
			end
		end
		self:SetUpdateFunction(
			function(self)
				local row = SCREENMAN:GetTopScreen():GetCurrentRowIndex(PLAYER_1)
				local pos = 0
				if row > 4 then
					pos =
						NSPreviewY + NoteskinRow * OptionRowHeight -
						(SCREENMAN:GetTopScreen():GetCurrentRowIndex(PLAYER_1) - 4) * OptionRowHeight
				else
					pos = NSPreviewY + NoteskinRow * OptionRowHeight
				end
				self:y(pos)
				self:visible(NoteskinRow - row > -5 and NoteskinRow - row < 7)
			end
		)
	end,
	Def.ActorFrame {
		NSkinPreviewWrapper("Down", "Tap Note")
	},
	Def.ActorFrame {
		InitCommand = function(self)
			self:y(NSPreviewReceptorY)
		end,
		NSkinPreviewWrapper("Down", "Receptor")
	}
}
if GetScreenAspectRatio() > 1.7 then
	t[#t][#(t[#t]) + 1] =
		Def.ActorFrame {
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(NSPreviewXSpan * 1)
			end,
			NSkinPreviewWrapper("Left", "Tap Note")
		},
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(NSPreviewXSpan * 1):y(NSPreviewReceptorY)
			end,
			NSkinPreviewWrapper("Left", "Receptor")
		},
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(NSPreviewXSpan * 2)
			end,
			NSkinPreviewWrapper("Up", "Tap Note")
		},
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(NSPreviewXSpan * 2):y(NSPreviewReceptorY)
			end,
			NSkinPreviewWrapper("Up", "Receptor")
		},
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(NSPreviewXSpan * 3)
			end,
			NSkinPreviewWrapper("Right", "Tap Note")
		},
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(NSPreviewXSpan * 3):y(NSPreviewReceptorY)
			end,
			NSkinPreviewWrapper("Right", "Receptor")
		}
	}
end
return t
