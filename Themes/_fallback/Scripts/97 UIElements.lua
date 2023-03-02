--- UI Elements. Buttons. All the fun stuff.
-- @module 97_UIElements

-- originally created by ca25nada/Prim
-- minor modifications by poco0317
--[[ 
USAGE:
___________________________________________________
Setup:

Some form of continous loop is required on the Screen for calling the BUTTON:UpdateMouseState() function. This is usually done via an update function.

Input callback function is required that will call BUTTON:SetMouseDown() and the BUTTON:SetMouseUp() functions. 
Currently, both take the Event.DeviceInput.button parameters but is currently unused.

Whenever a given screen initially comes on, BUTTON:ResetButtonTable() should be called to clear any pre-existing references to any buttons.
Otherwise you will likely see error messages relating to accessing actors that no longer exist.

___________________________________________________
Button Creation:
For any actor that you want to designate as a button, call BUTTON:AddButton() from the actor.
(Usually from the OnCommand, you will likely run into issues when calling from InitCommand from uninitialized actors.)

The function takes the following parameters:

Actor - actor
The Actor object you want to designate as a button, 
actor should have a non-zero positive value when actor:GetWidth()/actor:GetHeight() is called, but otherwise can be anything.

Screen - screen
The Screen object that the actor belongs to. This is needed to handle multiple overlapping screens.
e.g: if the actor specified above was created in ScreenSelectMusic, then the screen object for ScreenSelectMusic should be specified here.
	usually you can get this value via SCREENMAN:GetTopScreen()

int	- depth (optional, default = 0)
Some more complex buttons will usually involve an ActorFrame with multiple elements. Since the ActorFrame will have no width/height values,
the ActorFrame itself cannot be used as a button. However, you can create a child Actor with a defined width/height that will act as the button boundary for the actorframe.
The depth value corresponds to the number of nodes it needs to travel before reaching the "root" Actor/ActorFrame of the button.
e.g.: If there's an actorframe and a button child, the button child will specify a depth value of 1. When it's just the button as a standalone, the depth value should be specified as 0.

In addition, the Z value of the button actor is used to detect which button should be on top in the case of overlapping buttons.
This can be set by calling Actor:z(zValue).
The button who called BUTTON:AddButton() last will be considered as being on Top for tiebreakers.
___________________________________________________
Commands/Messages Broadcast:
None

___________________________________________________
Commands/Messages Unicast:

MouseOverCommand
Is played once when the mouse first goes over the specified button. The conditions are checked every time the BUTTON:UpdateMouseState() function is called.
ChildMouseOverCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseOutCommand
Is played once when the mouse that was over an actor moves off the button. The conditions are checked every time the BUTTON:UpdateMouseState() function is called.
ChildMouseOutCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseDownCommand
Is played once when the left click is pressed down for the first time while being over the button. The conditions are checked every time the BUTTON:SetMouseDown() function is called.
ChildMouseDownCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseUpCommand
Is played once when the mouse is released while being over the button. The conditions are checked every time the BUTTON:SetMouseUp() function is called.
ChildMouseUpCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseClickCommand
Is played once when both MouseDown and MouseUp events occur on the same button. The conditions are checked every time the BUTTON:SetMouseUp() function is called.
ChildMouseClickCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseReleaseCommand
Is played once when a button was pressed, but released while the mouse was no longer over the button.  The conditions are checked every time the BUTTON:SetMouseUp() function is called.
ChildMouseReleaseCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseHoldCommand
Is played every time BUTTON:UpdateMouseState() is called while a button is considered to be held down (e.g. between MouseDown and MouseUp events).
The Following parameters are also passed:
	MouseX - X coordinates relative to the actor designated as the "root" as specified by the depth value.
	MouseY - Y coordinates relative to the actor designated as the "root" as specified by the depth value.

	ChildMouseHoldCommand is also played for the actor designated as the "root" as specified by the depth value.

MouseDragCommand
Is played every time BUTTON:UpdateMouseState() is called while a button is considered to be held down and movement is detected since the last time the function was called.
	(e.g. holding a button down and dragging it around).
The Following parameters are also passed:
	MouseX - X coordinates relative to the actor designated as the "root" as specified by the depth value.
	MouseY - Y coordinates relative to the actor designated as the "root" as specified by the depth value.

	ChildMouseDragCommand is also played for the actor designated as the "root" as specified by the depth value.

___________________________________________________
Limitations:

Button rollover detection only takes the following into account:

	X, Y coordinates of the Button Actors and its parent/direct ancestors.
	Rotations on the Z axis for the Button Actors and its parent/direct ancestors.
	Halign/Valign values of the Button Actor.
	Width/Height of the Button Actor.

Any other transformations applied on the actor or any of its parent/ancestors will cause the button rollover detection to fail.
	e.g. Skew, rotations on X/Y axis, zoom on parent actors.

--]]

function Actor.PlayCommandsOnChildren(self, cmd, params)
    return self:RunCommandsOnChildren(function(self) self:playcommand(cmd, params) end)
end

function Actor.QueueCommandsOnChildren(self, cmd)
    return self:RunCommandsOnChildren(function(self) self:queuecommand(cmd) end)
end

-- Rotates coordinates x,y by an angle (degrees) from the origin.
function rotateFromOrigin(x, y, angle)
    local rad = math.rad(angle)
    return x * math.cos(rad) - y * math.sin(rad), x * math.sin(rad) + y * math.cos(rad)
end

function Actor.GetButtonRoot(self, depth)
	assert(depth >= 0, "Invalid Button Depth")
	
	local buttonRoot = self
	for i = 0, depth, 1 do
		buttonRoot = buttonRoot:GetParent()
	end

	return buttonRoot
end

-- Gets the X/Y coordinates relative to the parent of the actor's "root" node.
-- "root" node is specified by the depth value, which is the number of parent nodes needed to reach the "root"
function Actor.GetLocalMousePos(self, mouseX, mouseY, depth)
    if self == nil then
        return 0, 0
    end

	local buttonRoot = self:GetButtonRoot(depth)

    if buttonRoot == nil then
        return mouseX, mouseY
    else
        local rotationZ = buttonRoot:GetTrueRotationZ()
        local rootX = buttonRoot:GetTrueX()
        local rootY = buttonRoot:GetTrueY()
        return rotateFromOrigin(mouseX - rootX, mouseY - rootY, -rotationZ)
    end

end

-- recursively get the Z of an actor through itself and its parents
-- NOTE: this does not account for Xrotation or Yrotation.
function Actor.GetTrueZ(self)
	if self == nil then
		return 0
	end

	-- defer to fakeparent first
	local parent = self:GetFakeParent()
	if parent == nil then
		parent = self:GetParent()
	end

	if parent == nil then
		return self:GetZ()
	else
		return self:GetZ() + parent:GetTrueZ()
	end
end

-- Singleton for button related events.
BUTTON = {
	ButtonTable = {}, -- Table containing all the registered buttons for the current screen.
	DepthTable = {}, -- Button "depth" (# of parent actors until it reaches the "root" of the button)
	CurTopButton = nil, -- Current top button that the mouse is hovering over.
	CurTopButtonDepth = 0,
	CurDownButton = {}, -- Current button that is being held down.
	CurDownButtonDepth = {},
	UpdateOnlyOnMouseMovement = false
}

-- List of DeviceInput enum strings to handle for button inputs.
-- Check DeviceButton under http://dguzek.github.io/Lua-For-SM5/API/Lua.xml#Enums for all possible buttons.
BUTTON.AcceptedDeviceInput = {
	["DeviceButton_left mouse button"] = true,
	["DeviceButton_right mouse button"] = true,
	["DeviceButton_middle mouse button"] = true,
}

BUTTON.ScrollWheelInput = {
	["DeviceButton_mousewheel up"] = "Up",
	["DeviceButton_mousewheel down"] = "Down",
}

-- Function for handling input callbacks
-- Call screen:AddInputCallback(BUTTON.InputCallback) on the OnCommand of the screen where you want mouse inputs.
function BUTTON.InputCallback(event)

	if BUTTON.AcceptedDeviceInput[event.DeviceInput.button] then
		if event.type == "InputEventType_FirstPress" then
			BUTTON:SetMouseDown(event.DeviceInput.button)
			MESSAGEMAN:Broadcast("MouseClickPress", {button = event.DeviceInput.button})
		end

		if event.type == "InputEventType_Release" then
			BUTTON:SetMouseUp(event.DeviceInput.button)
			MESSAGEMAN:Broadcast("MouseClickRelease", {button = event.DeviceInput.button})
		end
	elseif BUTTON.ScrollWheelInput[event.DeviceInput.button] ~= nil then
		if event.type == "InputEventType_FirstPress" then
			-- produces a broadcast message of this for each "snap" of the wheel
			-- MouseScroll -- params: direction = Up/Down
			MESSAGEMAN:Broadcast("MouseScroll", {direction = BUTTON.ScrollWheelInput[event.DeviceInput.button]})
		end
	end

end

-- Resets the list of buttons currently added to the given screen. Call when the screen is being initialized.
function BUTTON.ResetButtonTable(self, screenName)
    if screenName ~= nil then
		self.ButtonTable[screenName] = nil
		self.CurTopButton = nil
		self.CurDownButton = {}
    end
end

-- Add/Register actors to act as buttons. This is called whenever QuadButton() is called.
function BUTTON.AddButton(self, actor, screenName, depth)
	if screenName ~= nil then
		if depth == nil then
			depth = 0
		end

        if self.ButtonTable[screenName] == nil then 
			self.ButtonTable[screenName] = {}
		end
		
		if self.DepthTable[screenName] == nil then
			self.DepthTable[screenName] = {}
		end

		self.ButtonTable[screenName][#self.ButtonTable[screenName]+1] = actor
		self.DepthTable[screenName][#self.DepthTable[screenName]+1] = depth
    end
end

-- Updates the position. Sends a broadcast if the position has changed.
-- This is called constantly from _mouse.lua via an updatefunction.
function BUTTON.UpdateMouseState(self)

	local topScreen = SCREENMAN:GetTopScreen()

    if topScreen == nil then
        return
	end

	if self.ButtonTable[topScreen:GetName()] == nil then
		return
	end

	local oldX = self.MouseX
	local oldY = self.MouseY
	self.MouseX = INPUTFILTER:GetMouseX()
	self.MouseY = INPUTFILTER:GetMouseY()


	local curButton, curButtonDepth = self:GetTopButton(self.MouseX, self.MouseY)
	-- If the top actor in which the mouse was hovering over has changed.
	if curButton ~= self.CurTopButton then
		if curButton ~= nil then 
			self:OnMouseOver(curButton, curButtonDepth)
		end
		if self.CurTopButton ~= nil then
			self:OnMouseOut(self.CurTopButton, self.CurTopButtonDepth)
		end
	end
	self.CurTopButton = curButton
	self.CurTopButtonDepth = curButtonDepth
	
	for event,curDownButton in pairs(self.CurDownButton) do

		if curDownButton ~= nil then
			local localX, localY = curDownButton:GetLocalMousePos(self.MouseX, self.MouseY, self.CurDownButtonDepth[event])
			if oldX ~= self.MouseX or oldY ~= self.MouseY then
				self:OnMouseDrag(curDownButton, self.CurDownButtonDepth[event], {event = event, MouseX = localX, MouseY = localY})
			end
			self:OnMouseHold(curDownButton, self.CurDownButtonDepth[event], {event = event, MouseX = localX, MouseY = localY})
		end
	end
end

-- Record where the mousedown event occured.
function BUTTON.SetMouseDown(self, event)
	local localX, localY

	self.CurDownButton[event] = self.CurTopButton
	self.CurDownButtonDepth[event] = self.CurTopButtonDepth
	if self.CurDownButton[event] ~= nil then -- Only call onmousedown if a button is pressed.
		localX, localY = self.CurDownButton[event]:GetLocalMousePos(self.MouseX, self.MouseY, self.CurDownButtonDepth[event])
		self:OnMouseDown(self.CurDownButton[event], self.CurDownButtonDepth[event], {event = event, MouseX = localX, MouseY = localY})
	end
end

-- Record where the mouseup event occured.
function BUTTON.SetMouseUp(self, event)

	-- Make local copies as the values can change before the function ends.
	local curTopButton = self.CurTopButton
	local curTopButtonDepth = self.CurTopButtonDepth
	local curDownButton = self.CurDownButton[event]
	local curDownButtonDepth = self.CurDownButtonDepth[event]
	local localX, localY

	if curTopButton == nil then
		if curDownButton == nil then -- Clicked non-button, release at non-button
            return
            
		else -- Clicked button, release at non-button
			localX, localY = curDownButton:GetLocalMousePos(self.MouseX, self.MouseY, curDownButtonDepth)
			self:OnMouseRelease(curDownButton, curDownButtonDepth, {event = event, MouseX = localX, MouseY = localY})
		end

	else
		if curDownButton == nil then -- Clicked non-button, release at button
			localX, localY = curTopButton:GetLocalMousePos(self.MouseX, self.MouseY, curTopButtonDepth)
			self:OnMouseUp(curTopButton, curTopButtonDepth, {event = event, MouseX = localX, MouseY = localY})

		elseif curDownButton == curTopButton then -- Clicked button, released on same button
			localX, localY = curTopButton:GetLocalMousePos(self.MouseX, self.MouseY, curTopButtonDepth)
			self:OnMouseUp(curTopButton, curTopButtonDepth, {event = event, MouseX = localX, MouseY = localY})
			self:OnMouseClick(curTopButton, curTopButtonDepth, {event = event, MouseX = localX, MouseY = localY})

		else -- Clicked button, released at different button
			localX, localY = curTopButton:GetLocalMousePos(self.MouseX, self.MouseY, curTopButtonDepth)
			self:OnMouseUp(curTopButton, curTopButtonDepth, {event = event, MouseX = localX, MouseY = localY})
			self:OnMouseRelease(curDownButton, curDownButtonDepth, {event = event, MouseX = localX, MouseY = localY})
		end
	end
	

	self.CurDownButton[event] = nil
	self.CurDownButtonDepth[event] = nil

end

-- Return the button with the highest Z value that is clickable from coordinates (X,Y)
function BUTTON.GetTopButton(self, x, y)
    local topScreen = SCREENMAN:GetTopScreen()
    if topScreen == nil then
        return
    end

	local topZ = -99999
	local topButton = nil
	local topButtonDepth = 0

	if self.ButtonTable[topScreen:GetName()] == nil then
		return
	end

	if #self.ButtonTable[topScreen:GetName()] == 0 then
		return
	end

	for i,v in ipairs(self.ButtonTable[topScreen:GetName()]) do
		if v:IsOver(x, y) then 
			local z = v:GetTrueZ()
			if z >= topZ then
				topButton = v
				topZ = z
				topButtonDepth = self.DepthTable[topScreen:GetName()][i]
			end
		end
	end

	return topButton, topButtonDepth
end

-- Called while an actor is held down.
function BUTTON.OnMouseHold(self, actor, depth, param)
	actor:playcommand("MouseHold", param)
	actor:GetButtonRoot(depth):playcommand("ChildMouseHold", param)
end

-- Called when the mouse is moved while an actor is held down.
function BUTTON.OnMouseDrag(self, actor, depth, param)
	actor:playcommand("MouseDrag", param)
	actor:GetButtonRoot(depth):playcommand("ChildMouseDrag", param)
end

-- Called when mouse begins to hover over the actor.
function BUTTON.OnMouseOver(self, actor, depth)
	actor:playcommand("MouseOver")
	actor:GetButtonRoot(depth):playcommand("ChildMouseOver")
end

-- Called when the mouse is no longer hovering over the actor.
function BUTTON.OnMouseOut(self, actor, depth)
	actor:playcommand("MouseOut")
	actor:GetButtonRoot(depth):playcommand("ChildMouseOut")
end

-- Called when a mouse button is pressed while over the actor.
function BUTTON.OnMouseDown(self, actor, depth, param)
	actor:playcommand("MouseDown", param)
	actor:GetButtonRoot(depth):playcommand("ChildMouseDown", param)
end

-- Called when a mouse button is released while over the actor.
function BUTTON.OnMouseUp(self, actor, depth, param)
	actor:playcommand("MouseUp", param)
	actor:GetButtonRoot(depth):playcommand("ChildMouseUp", param)
end

-- Called when both mousedown and mouseup events occur on the same actor.
function BUTTON.OnMouseClick(self, actor, depth, param)
	actor:playcommand("MouseClick", param)
	actor:GetButtonRoot(depth):playcommand("ChildMouseClick", param)
end

-- Called when a button was pressed but a mouseup event occured while not on the button.
function BUTTON.OnMouseRelease(self, actor, depth, param)
	actor:playcommand("MouseRelease", param)
	actor:GetButtonRoot(depth):playcommand("ChildMouseRelease", param)
end


UIElements = {}

-- Adapted from  Simply-Love-SM5/Scripts/SL-Helpers.lua
function UIElements.Border(width, height, bw)
	return Def.ActorFrame {
		Def.Quad {
			Name = "MaskSource",
			InitCommand = function(self)
				self:zoomto(width, height):MaskSource(true)
			end
		},
		Def.Quad {
			Name = "MaskDest",
			InitCommand = function(self)
				self:zoomto(width + 2 * bw, height + 2 * bw):MaskDest()
			end
		},
		Def.Quad {
			Name = "ClearBuffer",
			InitCommand = function(self)
				self:diffusealpha(0):clearzbuffer(true)
			end
		},
	}
end

-- Basic clickable button implementation with quads
function UIElements.QuadButton(z, depth)

	local t = Def.Quad {
		InitCommand = function(self) 
			self:z(z)
		end,
		OnCommand = function(self)
			local screen = SCREENMAN:GetTopScreen()
			if screen ~= nil then
				BUTTON:AddButton(self, screen:GetName(), depth)
			end
		end,
		ReloadedScriptsMessageCommand = function(self)
			-- dont playcommand the OnCommand because that could cause cascading issues
			local screen = SCREENMAN:GetTopScreen()
			if screen ~= nil then
				BUTTON:AddButton(self, screen:GetName(), depth)
			end
		end,
		MouseOverCommand = function(self) end,
		MouseOutCommand = function(self) end,
		MouseUpCommand = function(self, params) end,
		MouseDownCommand = function(self, params) end,
		MouseClickCommand = function(self, params) end,
		MouseReleaseCommand = function(self, params) end,
		MouseDragCommand = function(self, params) end,
		MouseHoldCommand = function(self, params) end,
	}
	return t
end

-- clickable image
function UIElements.SpriteButton(z, depth, tex)

	local t = Def.Sprite {
		Texture = tex,
		InitCommand = function(self) 
			self:z(z)
		end,
		OnCommand = function(self)
			local screen = SCREENMAN:GetTopScreen()
			if screen ~= nil then
				BUTTON:AddButton(self, screen:GetName(), depth)
			end
		end,
		ReloadedScriptsMessageCommand = function(self)
			-- dont playcommand the OnCommand because that could cause cascading issues
			local screen = SCREENMAN:GetTopScreen()
			if screen ~= nil then
				BUTTON:AddButton(self, screen:GetName(), depth)
			end
		end,
		MouseOverCommand = function(self) end,
		MouseOutCommand = function(self) end,
		MouseUpCommand = function(self, params) end,
		MouseDownCommand = function(self, params) end,
		MouseClickCommand = function(self, params) end,
		MouseReleaseCommand = function(self, params) end,
		MouseDragCommand = function(self, params) end,
		MouseHoldCommand = function(self, params) end,
	}
	return t
end

---
--- the point of this is to have a square hover area for a text button
--- it is possible to just hover the text but that introduces some odd issues
--- and come on, really, it is harder to click text than it is a square button
function UIElements.TextButton(z, depth, font)
	local t = Def.ActorFrame {
		Def.Quad {
			Name = "BG",
			InitCommand = function(self)
				self:diffusealpha(0)
				self:z(z)
			end,
			OnCommand = function(self)
				local screen = SCREENMAN:GetTopScreen()
				if screen ~= nil then
					BUTTON:AddButton(self, screen:GetName(), depth)
				end
			end,
			ReloadedScriptsMessageCommand = function(self)
				-- dont playcommand the OnCommand because that could cause cascading issues
				local screen = SCREENMAN:GetTopScreen()
				if screen ~= nil then
					BUTTON:AddButton(self, screen:GetName(), depth)
				end
			end,
			MouseOverCommand = function(self) self:GetParent():playcommand("RolloverUpdate",{update = "in"}) end,
			MouseOutCommand = function(self) self:GetParent():playcommand("RolloverUpdate",{update = "out"}) end,
			MouseUpCommand = function(self,params) self:GetParent():playcommand("Click",{update = "OnMouseUp", event = params.event}) end,
			MouseDownCommand = function(self,params) self:GetParent():playcommand("Click",{update = "OnMouseDown", event = params.event}) end,
			MouseClickCommand = function(self,params) self:GetParent():playcommand("Click",{update = "OnMouseClicked", event = params.event}) end,
			MouseReleaseCommand = function(self,params) self:GetParent():playcommand("Click",{update = "OnMouseReleased", event = params.event}) end,
			MouseDragCommand = function(self, params) self:GetParent():playcommand("DragUpdate", params) end,
	
		},
		Def.BitmapText {
			Name = "Text",
			Font = font,
			InitCommand = function(self)
				self:z(z)
			end,
		}
	}

	return t
end

-- this is the bare minimum implementation of the above function
-- this can be used as a text button as well, but due to reasons, this is more inaccurate.
function UIElements.TextToolTip(z, depth, font)
	return Def.BitmapText {
		Font = font,
		InitCommand = function(self)
			self:z(z)
		end,
		OnCommand = function(self)
			local screen = SCREENMAN:GetTopScreen()
			if screen ~= nil then
				BUTTON:AddButton(self, screen:GetName(), depth)
			end
		end,
		ReloadedScriptsMessageCommand = function(self)
			-- dont playcommand the OnCommand because that could cause cascading issues
			local screen = SCREENMAN:GetTopScreen()
			if screen ~= nil then
				BUTTON:AddButton(self, screen:GetName(), depth)
			end
		end,
	}
end

-- Basic clickable button implementation with quads
function ButtonDemo(z)

	local t = Def.ActorFrame {
		RolloverUpdateCommand = function(self, params)
			self:PlayCommandsOnChildren("RolloverUpdate", params)
		end,
		ClickCommand = function(self, params)
			self:PlayCommandsOnChildren("Click", params)
		end,
		DragUpdateCommand = function(self, params)
			self:xy(params.MouseX, params.MouseY)
		end,
	}
	
	t[#t+1] = UIElements.Border(150, 50, 5) .. {
		InitCommand = function(self)
			self:visible(false):diffuse(color("#000000"))
		end,
		RolloverUpdateCommand = function(self, params)
			if params.update == "over" then
				self:visible(true)
				TOOLTIP:Show()
				TOOLTIP:SetText(string.format("X:%0.2f Y:%0.2f", self:GetTrueX(), self:GetTrueY()))
			end
		
			if params.update == "out" then
				self:visible(false)
				TOOLTIP:Hide()
			end
		end
	}

	t[#t+1] = UIElements.QuadButton(z, 1) .. {
		InitCommand= function(self) 
			self:z(z):zoomto(150,50):diffuse(color("#000000")):diffusealpha(0.5)
		end,
		MouseOverCommand = function(self) self:GetParent():playcommand("RolloverUpdate",{update = "in"}) end,
		MouseOutCommand = function(self) self:GetParent():playcommand("RolloverUpdate",{update = "out"}) end,
		MouseUpCommand = function(self,params) self:diffuse(color("#FF0000")):diffusealpha(0.5) self:GetParent():playcommand("Click",{update = "OnMouseUp", event = params.event}) end,
		MouseDownCommand = function(self,params) self:diffuse(color("#00FF00")):diffusealpha(0.5) self:GetParent():playcommand("Click",{update = "OnMouseDown", event = params.event}) end,
		MouseClickCommand = function(self,params) self:diffuse(color("#0000FF")):diffusealpha(0.5) self:GetParent():playcommand("Click",{update = "OnMouseClicked", event = params.event}) end,
		MouseReleaseCommand = function(self,params) self:diffuse(color("#FF00FF")):diffusealpha(0.5) self:GetParent():playcommand("Click",{update = "OnMouseReleased", event = params.event}) end,
		MouseDragCommand = function(self, params) self:GetParent():playcommand("DragUpdate", params) end,
	}

	t[#t+1] = LoadFont("Common Normal") .. {
		InitCommand= function(self) 
			self:y(0):zoom(0.6):settext("init")
		end,
		ClickCommand = function(self, params)
			self:settextf("X:%.0f Y:%.0f \nAngle:%.0f \n%s",self:GetTrueX(), self:GetTrueY(), self:GetTrueRotationZ() % 360, params.event)
		end,
	}

	return t
end


-- Checkboxes
function UIElements.CheckBox(z, checked)

	local zoom = 0.15
	local checked = checked

	local t = Def.ActorFrame{
		OnCommand = function(self)
			self:playcommand("Toggle")
		end,
		ChildMouseClickCommand = function(self)
			if checked then 
				checked = false
				self:playcommand("Uncheck")
				self:RunCommandsOnChildren(function(self) self:playcommand("Uncheck") end)
			else
				checked = true
				self:playcommand("Check")
				self:RunCommandsOnChildren(function(self) self:playcommand("Check") end)
			end
		end,
		CheckCommand = function(self)
		end,
		UncheckCommand = function(self)
		end
	}

	t[#t+1] = UIElements.QuadButton(z, 1) .. {
		Name = "Button",
		InitCommand = function(self)
			self:zoomto(zoom * 100, zoom * 100)
		end,
		MouseDownCommand = function(self)
		end,
		MouseUpCommand = function(self)
		end,
		MouseReleaseCommand = function(self)
		end
	}

	t[#t+1] = LoadActor(THEME:GetPathG("", "checkmark")) .. {
		Name = "CheckGraphic",
		InitCommand = function(self)
			self:zoom(zoom)
		end,
		CheckCommand = function(self)
			self:finishtweening()
			self:smooth(0.1)
			self:zoom(zoom)
			self:diffusealpha(1)
		end,
		UncheckCommand = function(self)
			self:finishtweening()
			self:smooth(0.1)
			self:zoom(0)
			self:diffusealpha(0)
		end
	}

	return t
end

function UIElements.Slider(z, width)
	local zoom = 0.15

	local t = Def.ActorFrame {
		SliderMovedCommand = function(self, params)
			self:GetChild("ValueText"):settext(params.Value)
		end,
	}

	t[#t+1] = UIElements.QuadButton(z + 1, 0) .. {
		Name = "SliderBar",
		InitCommand = function(self)
			self:zoomto(width, zoom * 100 / 2)
			self:halign(0)
		end,
		MouseOverCommand = function(self) end,
		MouseOutCommand = function(self) end,
		MouseUpCommand = function(self, params) end,
		MouseDownCommand = function(self, params)
			local mouseX = clamp(params.MouseX, 0, width)
			self:GetParent():GetChild("SliderHandle"):x(mouseX)
			self:GetParent():playcommand("SliderMoved", {Value = mouseX / width})
		end,
		MouseClickCommand = function(self, params) end,
		MouseReleaseCommand = function(self, params) end,
		MouseDragCommand = function(self, params)
			local mouseX = clamp(params.MouseX, 0, width)
			self:GetParent():GetChild("SliderHandle"):x(mouseX)
			self:GetParent():playcommand("SliderMoved", {Value = mouseX / width})
		end,
	}

	t[#t+1] = UIElements.QuadButton(z, 0) .. {
		Name = "SliderHandle",
		InitCommand = function(self)
			self:zoomto(zoom * 100, zoom * 100)
			self:diffuse(COLOR.TextMain)
			self:x(0)
		end,
		MouseOverCommand = function(self) end,
		MouseOutCommand = function(self) end,
		MouseUpCommand = function(self, params) end,
		MouseDownCommand = function(self, params) end,
		MouseClickCommand = function(self, params) end,
		MouseReleaseCommand = function(self, params) end,
		MouseDragCommand = function(self, params)
			local mouseX = clamp(params.MouseX, 0, width)
			self:x(mouseX)
			self:GetParent():playcommand("SliderMoved", {Value = mouseX / width})
		end,
	}

	t[#t+1] = LoadFont("Common Normal") .. {
		Name = "ValueText",
		InitCommand = function(self)
			self:x(width + 10):zoom(0.6):halign(0):diffuse(color("#000000"))
		end;
	}

	return t

end