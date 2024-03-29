--- Noteskin previewing utility
-- @usage LoadNSkinPreview("Get", "Down", "Tap Note");
-- @module 09_SimNSkinPreview

local function NSkinPreviewChange(Container, Player)
	return function(event)
		for i, n in pairs(NOTESKIN:GetNoteSkinNames()) do
			Container:GetChild("N" .. i):playcommand("Deactivate")
		end
		for i = 1, SCREENMAN:GetTopScreen():GetNumRows() - 1 do
			if SCREENMAN:GetTopScreen():GetOptionRow(i):GetName() == "NoteSkins" then
				local nSkin =
					Container:GetChild("N" .. SCREENMAN:GetTopScreen():GetOptionRow(i):GetChoiceInRowWithFocus(Player) + 1)
				nSkin:playcommand("Activate")
			end
		end
	end
end

local function SimChild(Container, Type)
	local notable, Count
	local rType = {}
	local Child = {Container}
	repeat
		notable = true
		Count = Child
		Child = {}
		for no in pairs(Count) do
			if lua.CheckType("ActorFrame", Count[no]) then
				for _, cld in pairs(Count[no]:GetChildren()) do
					if lua.CheckType("ActorFrame", cld) then
						notable = false
						Child[#Child + 1] = cld
					elseif lua.CheckType(Type, cld) then
						rType[#rType + 1] = cld
					end
				end
			end
		end
	until notable == true
	return rType
end

function GivenGameToFullNSkinElements(game)
	-- for this, restrict to only 4 keys to show them off
	-- and to save on visible space
	local theTable = {
		dance = {
			"Left", "Down", "Up", "Right"
		},
		pump = {
			"DownLeft", "UpLeft", "Center", "UpRight", "DownRight" 
		},
		kb7 = {
			"Key1", "Key2", "Key3", "Key4", "Key5", "Key6", "Key7"
		},
		beat = {
			--"scratch", "Key1", "Key2", "Key3"
			"Key1", "Key2", "Key3", "Key4", "Key5", "Key6", "Key7", "scratch"
		},
		solo = {
			--"Left", "UpLeft", "UpRight", "Right"
			"Left", "UpLeft", "Down", "Up", "UpRight", "Right"
		},
		popn = {
			--"Left White", "Left Yellow", "Left Green", "Left Blue"
			"Left White", "Left Yellow", "Left Green", "Left Blue", "Red", "Right Blue", "Right Green", "Right Yellow", "Right White"
		},
	}
	return theTable[game]
end

function GivenGameToNSkinElements(game)
	-- for this, restrict to only 4 keys to show them off
	-- and to save on visible space
	local theTable = {
		dance = {
			"Left", "Down", "Up", "Right"
		},
		pump = {
			"DownLeft", "UpLeft", "Center", "UpRight", --"DownRight" 
		},
		kb7 = {
			"Key1", "Key2", "Key3", "Key4", --"Key5", "Key6", "Key7"
		},
		beat = {
			"scratch", "Key1", "Key2", "Key3"
			--"Key1", "Key2", "Key3", "Key4", "Key5", "Key6", "Key7", "scratch"
		},
		solo = {
			"Left", "UpLeft", "UpRight", "Right"
			--"Left", "UpLeft", "Down", "Up", "UpRight", "Right"
		},
		popn = {
			"Left White", "Left Yellow", "Left Green", "Left Blue" -- etc.....
			--"Left White", "Left Yellow", "Left Green", "Left Blue", "Red", "Right Blue", "Right Green", "Right Yellow", "Right White"
		},
	}
	return theTable[game]
end

function GameToNSkinElements()
	local game = PREFSMAN:GetPreference("CurrentGame")
	return GivenGameToNSkinElements(game)
end

--- Load a NoteSkin preview actor
-- @tparam string Noteskin Noteskin name. If "Get" then it does the currently selected noteskin, and updates itself when it changes. Defaults to "Get"
-- @tparam string Button Ex: "Up". Defaults to "Down"
-- @tparam string Element Defaults to "Tap Note"
-- @tparam boolean Whether or not to add the input callback which controls the visibility of this
-- @treturn ActorFrame containing the noteskin preview
function LoadNSkinPreview(Noteskin, Button, Element, UseInputCallback)
	local Player = PLAYER_1
	Noteskin = Noteskin or "Get"
	Button = Button or "Tap Note"
	Element = Element or "Down"
	if UseInputCallback == nil then UseInputCallback = true end
	if Noteskin == "Get" then
		local t =
			Def.ActorFrame {
			OnCommand = function(self)
				if UseInputCallback then
					SCREENMAN:GetTopScreen():AddInputCallback(NSkinPreviewChange(self, Player))
				end
			end
		}
		for i, n in pairs(NOTESKIN:GetNoteSkinNames()) do
			t[#t + 1] =
				Def.ActorFrame {
				Name = "N" .. i,
				InitCommand = function(self)
					if n ~= GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred"):NoteSkin() then
						self:visible(false)
					end
					if Element == "Tap Note" then
						local TexY = NOTESKIN:GetMetricFForNoteSkin("NoteDisplay", "TapNoteNoteColorTextureCoordSpacingY", n)
						local TexX = NOTESKIN:GetMetricFForNoteSkin("NoteDisplay", "TapNoteAdditionTextureCoordOffsetX", n)
						if TexY > 0 then
							for _, i in pairs(SimChild(self, "Sprite")) do
								local ani = i:GetNumStates()
								local maxframes = i:GetTexture():GetNumFrames()
								local Skip = 1
								local TexY2 = (TexY * 64)
								local StateF = {}
								if TexX == 0.5 then
									Skip = 2
									TexY2 = (TexY * 64) * 2
								end
								for timer = 1, TexY2, Skip do
									for frame = (ani * timer) - ani, (ani * timer) - 1 do
										if frame < maxframes then
											StateF[#StateF + 1] = {Frame = frame, Delay = 4 / ((TexY * 64) * ani)}
										end
									end
								end
								i:SetStateProperties(StateF)
							end
						end
					end
				end,
				DeactivateCommand = function(self)
					self:visible(false)
				end,
				ActivateCommand = function(self)
					self:visible(true)
					if self.activated then return end
					self.activated = true

					NOTESKIN:AddChildActorForNoteSkin(Button, Element, n, self)
				end,
			}
		end
		return t
	else
		return NOTESKIN:LoadActorForNoteSkin(Button, Element, NOTESKIN:DoesNoteSkinExist(Noteskin) and Noteskin or "default")
	end
end
