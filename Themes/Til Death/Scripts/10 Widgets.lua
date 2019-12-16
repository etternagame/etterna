--[[
	There are 2 types of widgets:
		- Wrappers for normal Actors which aim to have a more declarative uage and provide a
		relatively simple way to access the C++ actor handles since they're automagically bound
		to widget.actor in initialization.
		- Higher level widgets like TextBox, Slider, ComboBox, etc
	Of course, all of these "widgets" just return some combination of actors with some behaviour.
	For every widget, you can do widget.actor to get the actual actor (As long as that
		actor has been initialized, meaning it's InitCommand was executed. Using it in OnCommand's
		is safe)
	Naming conventions:
	widget.actor = handle to C++ actor
	lowercase first letter = not a "method"
	uppercase first letter = a "method"
	Widg.defaults.lowercasename = default params for "Name" actor
	Other conventions:
	Widg.WidgetName.new {param1=0,y=2,etc} to create an actor (You still have to add it to the actor
		returned by the file)
	All color parameters to widgets should be able to take a color string like "ffffff" or color("ffffff")
		to reduce boilerplate (Calling color manually)
	All widgets have an x and y and onInit (function(self)) params

	Ex.:
		local t = Widg.Container { x = SCREEN_WIDTH/2,y=SCREEN_HEIGHT/2}
		local button1 = Widg.Button { x = -100 }
		local button2 = Widg.Button {
			x = 100,
			onClick = function(self)
				button1.
			end
		}
		t:add(button1)
		t:add(button2)
		return t

	TODOs: (Generals)
		Make naming scheme coherent
		Try to reduce function creation inside widget creation functions (Function creation is
			NYI(not compiled) in LuaJit 2.1)
			Probably use metatables where possible
		"Visual" containers (Automagically mouse-wheel-scrollable html-like tables/lists that
			also help abstract the item positioning math)
		Find out why ComboBox (The ActorFrameTexture it uses) looks bad (Probably C++)
		Consider implications of using metatables instead of fillNilTableFieldsFrom, specially mutability
			of stuff like having a table inside default params (Like params.font.scale)
--]]
Widg = {}
Widg.defaults = {}
Widg.mts = {} -- metatables
function Widg.fillNilTableFieldsFrom(table1, defaultTable)
	for key, value in pairs(defaultTable) do
		if table1[key] == nil then
			table1[key] = defaultTable[key]
		end
	end
end
local fillNilTableFieldsFrom = Widg.fillNilTableFieldsFrom
function Widg.checkColor(c)
	if type(c) == "string" then
		if string.sub(c, 1, 1) ~= "#" then
			c = "#" .. c
		end
		if string.len(c) < 9 then
			c = c .. string.rep("F", 9 - string.len(c))
		end
		return color(c)
	end
	return c
end
local checkColor = Widg.checkColor

Widg.defaults.container = {
	x = 0,
	y = 0,
	onInit = false,
	content = false,
	visible = true,
	name = "Container"
}
-- Widget wrapper for ActorFrame
Widg.Container = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.container)
	local container
	container =
		Def.ActorFrame {
		Name = params.name,
		InitCommand = function(self)
			container.actor = self
			self:xy(params.x, params.y):visible(params.visible)
			if params.onInit then
				params.onInit(container)
			end
		end
	}
	container.add = function(container, item)
		container[#container + 1] = item
	end
	container.Add = container.add
	if params.content then
		if params.content.Class then -- is an actor
			container[#container + 1] = params.content
		else -- assume its a table
			container[#container + 1] = Def.ActorFrame(params.content)
		end
	end
	return container
end

Widg.defaults.label = {
	x = 0,
	y = 0,
	scale = 1.0,
	text = "Label",
	name = "Common Normal",
	width = false,
	color = color("#FFFFFF"),
	halign = 0.5,
	visible = true,
	valign = 0.5,
	multicolor = false, -- syntax: "|crrggbbaaTextWithColor1|crrggbbaaTextColor2"
	onInit = false
}
-- Widget wrapper for BitmapText
Widg.Label = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.label)
	params.color = checkColor(params.color)

	local initText = params.text
	local label
	label =
		(multicolor and LoadColorFont or LoadFont)(params.name) ..
		{
			Name = "Label",
			InitCommand = function(self)
				label.actor = self
				self:xy(params.x, params.y):zoom(params.scale):halign(params.halign):valign(params.valign):visible(params.visible)
				if type(params.width) == "number" then
					self:maxwidth(params.width / params.scale)
				end
				self:settext(initText):diffuse(params.color)
				if params.onInit then
					params.onInit(label)
				end
			end
		}
	label.settext = function(label, text)
		if label.actor then
			label.actor:settext(text)
		else
			initText = text
		end
	end
	label.GetText = function(label, text)
		return label.actor and label.actor:GetText() or initText
	end
	label.SetText = label.settext
	return label
end

-- Widget wrapper for Quad
Widg.defaults.rectangle = {
	x = 0,
	y = 0,
	width = 100,
	height = 100,
	color = color("#FFFFFF"),
	onClick = false,
	onRightClick = false, --TODO: Make the msgcommand for this execute
	onInit = false,
	alpha = 1.0,
	halign = 0.5,
	valign = 0.5,
	visible = true,
	clickPolling = 0.3
}
Widg.Rectangle = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.rectangle)
	params.color = checkColor(params.color)
	local lastClick = -1
	local q
	q =
		Def.Quad {
		InitCommand = function(self)
			self:halign(params.halign):valign(params.valign):xy(params.x + params.width / 2, params.y + params.height / 2):zoomto(
				params.width,
				params.height
			):diffusealpha(params.alpha)
			if params.onInit then
				params.onInit(q)
			end
			self:visible(params.visible)
			q.actor = self
		end,
		OnCommand = params.color and function(self)
				self:diffuse(params.color):diffusealpha(params.alpha)
			end or nil,
		LeftClickMessageCommand = params.onClick and function(self)
				if params.onClick and q:isOver() then
					lastClick = os.clock()
					params.onClick(q)
				end
			end or nil
	}
	q.isOver = function(self)
		return os.clock() - lastClick > params.clickPolling and isOver(self.actor)
	end
	return q
end

Widg.defaults.borders = {
	x = 0,
	y = 0,
	color = color("#FFFFFF"),
	width = 100,
	height = 100,
	borderWidth = 10,
	onInit = false,
	alpha = 1.0
}
-- 4 rectangles forming borders
-- Abstraction over the necessary math and logic
Widg.Borders = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.borders)
	params.color = checkColor(params.color)
	local left =
		Widg.Rectangle({width = params.borderWidth, height = params.height, color = params.color, alpha = params.alpha})
	local right =
		Widg.Rectangle(
		{
			x = params.width - params.borderWidth,
			width = params.borderWidth,
			height = params.height,
			color = params.color,
			alpha = params.alpha
		}
	)
	local top =
		Widg.Rectangle({width = params.width, height = params.borderWidth, color = params.color, alpha = params.alpha})
	local bottom =
		Widg.Rectangle(
		{
			y = params.height - params.borderWidth,
			width = params.width,
			height = params.borderWidth,
			color = params.color,
			alpha = params.alpha
		}
	)
	local borders
	borders =
		Def.ActorFrame {
		InitCommand = function(self)
			borders.actor = self
			self:xy(params.x, params.y)
			if params.onInit then
				params.onInit(borders)
			end
		end,
		left,
		top,
		right,
		bottom
	}
	borders.top = top
	borders.left = left
	borders.bottom = bottom
	borders.right = right
	return borders
end
local function highlight(self)
	self:queuecommand("Highlight")
end
Widg.highlightF = highlight

Widg.defaults.borderedrect = {
	x = 0,
	y = 0,
	color = "FFFFFF",
	border = {
		color = "000000",
		width = 2
	},
	width = 100,
	height = 100,
	onInit = false,
	alpha = 1.0,
	visible = true
}
-- Combination of Widg.Borders and Widg.Rectangle
Widg.BorderedRect = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.borderedrect)
	params.color = checkColor(params.color)
	params.border.color = checkColor(params.border.color)
	return Widg.Container {
		x = params.x,
		y = params.y,
		visible = params.visible,
		onInit = params.onInit,
		content = {
			Widg.Borders {
				width = params.width,
				height = params.height,
				alpha = params.alpha,
				color = params.border.color
			},
			Widg.Rectangle {
				width = params.width,
				height = params.height,
				color = params.color,
				borderWidth = params.border.width
			}
		}
	}
end

Widg.defaults.sprite = {
	x = 0,
	y = 0,
	color = false,
	onInit = false,
	texture = false,
	valign = 0.5,
	halign = 0.5,
	width = 0,
	height = 0,
	color = false
}
Widg.Sprite = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.sprite)
	params.color = checkColor(params.color)
	local sprite
	sprite =
		Def.Sprite {
		_Level = 1,
		Texture = path,
		InitCommand = function(self)
			sprite.actor = self
			self:xy(params.x, params.y):halign(params.halign):valign(params.valign)
			if params.color then
				self:diffuse(params.color)
			end
			if params.width > 0 and params.height > 0 then
				self:zoomto(params.width, params.height)
			end
			if params.onInit then
				params.onInit(sprite)
			end
		end
	}
	if params.texture then
		sprite.Texture = ResolveRelativePath(THEME:GetPathG("", params.texture), 3)
	end
	return sprite
end
Widg.defaults.button = {
	x = 0,
	y = 0,
	width = 50,
	height = 20,
	bgColor = color("#aaaaaa"),
	border = {
		color = color("#888888"),
		width = 2
	},
	highlight = {
		color = color("#cccccc"),
		alpha = false
	},
	onClick = false,
	onInit = false,
	onHighlight = false,
	onUnhighlight = false,
	alpha = 1.0,
	text = "Button",
	font = {
		scale = 0.5,
		name = "Common Large",
		color = color("#000000"),
		padding = {
			x = 10,
			y = 10
		}
	},
	halign = 0.5,
	valign = 0.5,
	texture = false,
	highlightCondition = isOver, -- function(self) -> bool
	enabled = true
}
--[[
	Structure: {
		bg,
		label,
		borders, -- optional
		sprite, -- optional
		highlightSprite -- optional
	}
	Methods: {
		GetText(),
		SetText(text),
		Diffuse(color),
		Enable(),
		Disable()
	}
]]
Widg.Button = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.button)
	if params.highlight then
		params.highlight.color = checkColor(params.highlight.color)
	end
	params.bgColor = checkColor(params.bgColor)
	if params.font then
		params.font.color = checkColor(params.font.color)
	end
	if params.border then
		params.border.color = checkColor(params.border.color)
	end

	local button
	button =
		Widg.Container {
		x = params.x,
		y = params.y,
		onInit = function(self)
			if params.onInit then
				params.onInit(button)
			end
			if params.highlight then
				self.actor:SetUpdateFunction(highlight)
			end
			self.params = params
			if not button.enabled then
				button:Disable()
			end
		end
	}
	button.enabled = params.enabled

	button.sprite =
		params.texture and
		Widg.Sprite {
			color = params.bgColor,
			texture = "buttons/" .. params.texture,
			width = params.width,
			height = params.height,
			halign = params.halign - 0.5,
			valign = params.valign - 0.5
		} or
		Def.ActorFrame {}
	button.highlightSprite =
		params.highlight and params.highlight.texture and
		Widg.Sprite {
			color = params.highlight.color,
			texture = "buttons/" .. params.highlight.texture,
			width = params.width,
			height = params.height,
			halign = params.halign - 0.5,
			valign = params.valign - 0.5
		} or
		Def.ActorFrame {}

	button.bg =
		Widg.Rectangle {
		width = params.width,
		height = params.height,
		color = params.bgColor,
		visible = not (not params.texture),
		alpha = params.texture and 0 or params.alpha,
		onClick = params.onClick and function(s)
				if button.enabled then
					params.onClick(button)
				end
			end or false,
		onRightClick = params.onRightClick,
		halign = params.halign,
		valign = params.valign,
		visible = not params.texture
	}
	do
		local mainActor = params.texture and button.sprite.actor
		button.bg.HighlightCommand = params.highlight and function(self)
				mainActor = mainActor or self
				local isOver = params.highlightCondition(self)
				if params.highlight.texture then
					(button.highlightSprite.actor):visible(isOver)
				end
				if isOver then
					if params.highlight.color then
						mainActor:diffuse(params.highlight.color)
					end
					mainActor:diffusealpha(params.highlight.alpha or params.alpha or 1)
					if params.onHighlight then
						params.onHighlight(button)
					end
				else
					if params.bgColor then
						mainActor:diffuse(params.bgColor)
					end
					mainActor:diffusealpha(params.alpha)
					if params.onUnhighlight then
						params.onUnhighlight(button)
					end
				end
			end or nil
	end

	button.borders =
		(params.texture or not params.border) and Def.ActorFrame {} or
		Widg.Borders {
			y = params.height * (0.5 - params.valign),
			x = params.width * (0.5 - params.halign),
			color = params.border.color,
			width = params.width,
			height = params.height,
			borderWidth = params.border.width,
			alpha = params.texture and 0 or params.alpha
		}

	button.label =
		Widg.Label {
		x = params.width * (1 - params.halign),
		y = params.height * (1 - params.valign),
		scale = params.font.scale,
		halign = params.font.halign,
		color = params.font.color,
		text = params.text,
		name = params.font.name,
		width = params.width - params.font.padding.x
	}

	button.settext = function(button, text)
		return (button.label):settext(text)
	end
	button.GetText = function(button)
		return (button.label):GetText()
	end
	button.diffuse = function(button, color)
		params.bgColor = checkColor(color)
		return (button.bg.actor):diffuse(color)
	end
	button.Diffuse = button.diffuse
	button.SetText = button.settext
	button.Enable = function(button)
		button.enabled = true
		(button.actor):SetUpdateFunction(highlight)
		return (button.actor):visible(button.enabled)
	end
	button.Disable = function(button)
		button.enabled = false
		(button.actor):SetUpdateFunction(nil)
		return (button.actor):visible(button.enabled)
	end

	button:add(button.bg)
	button:add(button.borders)
	button:add(button.sprite)
	button:add(button.highlightSprite)
	button:add(button.label)
	return button
end

Widg.defaults.scrollable = {
	width = 100,
	height = 100,
	content = false,
	textureName = false,
	x = 0,
	y = 0,
	halign = 0,
	valign = 0,
	onInit = false
}
Widg.scrollableCount = 0
--[[
	This aims to make actors in content "invisible" by the pixel outside of the defined rectangle
		while also easily providing the ability to scroll them using a scrollbar or mouse wheel
]]
Widg.Scrollable = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.scrollable)
	local textureName = params.textureName or "ScrollableWidget" .. tostring(Widg.scrollableCount)
	Widg.scrollableCount = Widg.scrollableCount + 1
	local content = params.content or Def.ActorFrame {}
	local sprite =
		Def.Sprite {
		Texture = textureName,
		InitCommand = function(self)
			self:halign(0):valign(0):SetTextureFiltering(true)
		end
	}
	local AFT =
		Def.ActorFrameTexture {
		InitCommand = function(self)
			self:SetTextureName(textureName)
			if params.width and params.width > 0 then
				self:SetWidth(params.width)
			end
			if params.height and params.height > 0 then
				self:SetHeight(params.height)
			end
			self:EnableAlphaBuffer(true)
			self:EnableDepthBuffer(true)
			self:EnableFloat(true)
			self:SetTextureFiltering(true)
			self:Create()
		end,
		Def.ActorFrame {
			Name = "Draw",
			content
		}
	}
	local scrollable
	scrollable =
		Def.ActorFrame {
		InitCommand = function(self)
			scrollable.actor = self
			self:xy(params.x + params.halign * params.width, params.y + params.valign * params.height)
			self.AFT = AFT
			self.sprite = sprite
			self.content = content
			self:SetTextureFiltering(true)
			if params.onInit then
				params.onInit(scrollable, content, AFT, sprite)
			end
		end,
		AFT,
		sprite
	}
	scrollable.content = content
	scrollable.aft = AFT
	scrollable.sprite = sprite
	return scrollable
end

do
	local function basicHandle(params)
		local h
		h =
			Widg.Rectangle {
			color = "00FF00",
			width = params.width / 5,
			height = params.height,
			valign = params.valign
		}
		h.onValueChange = function(val)
			h.actor:x(val * params.width / (params.max - params.min) - params.width * params.halign * 0.5)
		end
		return h
	end
	local function basicBar(params)
		return Widg.Rectangle {
			color = "FF0000",
			width = params.width,
			height = params.height,
			halign = params.halign,
			valign = params.valign
		}
	end
	Widg.defaults.sliderBase = {
		x = 0,
		y = 0,
		visible = true,
		enabled = true,
		width = 100,
		height = 30,
		onClick = false,
		color = color("#FFFFFFFF"),
		onValueChangeEnd = false, -- Called when the mouse click is released
		onValueChange = false, -- Recieves the value between min and max
		handle = basicHandle, -- function(params) that returns an actor for the handle
		--Note that the return value must have an onValueChange(val) field to update it's position
		bar = basicBar, -- function(params) that returns an actor for the bar
		onInit = false,
		defaultValue = 10, -- starting value
		max = 100,
		min = 0,
		step = 1,
		halign = 1,
		valign = 1,
		vertical = false, -- todo
		isRange = false --  todo
		-- If range, value = {start=number, end=number}
	}
	local function getRatioforAxis(mpos, pos, len, align)
		-- for some reason this magic math works
		-- it gives a number between 0 and 1
		-- its the percentage of the rectangle that
		-- is to the left of the mouse
		return (mpos - (pos + len * (1 - align))) / len + 0.5
	end
	-- this returns the current value for the slider
	local function getValue(mouse, params, slider)
		local length = (params.max - params.min)
		local ratio =
			params.vertical and getRatioforAxis(mouse.y, slider:GetTrueY(), params.height, params.valign) or
			getRatioforAxis(mouse.x, slider:GetTrueX(), params.width, params.halign)
		return math.round((ratio * length + params.min) / params.step) * params.step
	end
	--[[
		Base Slider. Meant to be very flexible and customizable.
		Structure: {
			bar,
			handle
		}
	]]
	Widg.SliderBase = function(params)
		fillNilTableFieldsFrom(params, Widg.defaults.sliderBase)
		params.color = checkColor(params.color)
		local bar = params.bar(params)
		local handle = params.handle(params)
		local updateFunction
		local container
		container =
			Widg.Container {
			visible = params.visible,
			x = params.x,
			y = params.y,
			onInit = function(container)
				handle.onValueChange(params.defaultValue)
				container.actor:SetUpdateFunction(updateFunction)
				if params.onInit then
					params.onInit(container)
				end
			end
		}
		container.bar = bar
		container.handle = handle
		container.enabled = params.enabled
		if params.range and type(params.defaultValue) ~= "table" then
			params.defaultValue = {params.defaultValue, params.defaultValue}
		end
		container.value = params.defaultValue
		container:add(bar)
		container:add(handle)
		local clicked = false
		local rectangle =
			Widg.Rectangle {
			width = params.width,
			height = params.height,
			halign = params.halign,
			valign = params.valign,
			onClick = function(rectangle)
				if container.enabled then
					clicked = true
				end
			end,
			-- isOver requires visible
			alpha = 0,
			visible = true
		}
		container:add(rectangle)
		-- "Temporary" set value (Not the end of the movement)
		container._SetValue = function(container, value)
			container.value = value
			if params.onValueChange then
				params.onValueChange(value)
			end
			if handle.onValueChange then
				handle.onValueChange(value)
			end
			if bar.onValueChange then
				bar.onValueChange(value)
			end
		end
		-- End of change set value
		container.SetValue = function(container, value)
			container:_SetValue(value)
			if params.onValueChangeEnd then
				params.onValueChangeEnd(value)
			end
			if bar.onValueChange then
				bar.onValueChange(value)
			end
			if bar.onValueChangeEnd then
				bar.onValueChangeEnd(value)
			end
		end
		updateFunction = function(containerActor)
			if clicked then
				if isOver(rectangle.actor) and INPUTFILTER:IsBeingPressed("Mouse 0", "Mouse") then
					local mouse = getMousePosition()
					container:_SetValue(getValue(mouse, params, containerActor))
				else
					clicked = false
					container:SetValue(container.value)
				end
			end
		end
		return container
	end
end

do
	local function basicSelection(choice, params)
		return Widg.Button {
			y = 0,
			text = choice,
			width = params.width,
			color = params.selectionColor,
			border = false,
			height = params.itemHeight,
			highlight = {color = params.hoverColor},
			alpha = 0.8
		}
	end
	local function basicItem(choice, params)
		return Widg.Button {
			text = choice,
			width = params.width,
			height = params.itemHeight,
			color = params.itemColor,
			border = false,
			alpha = 1,
			highlight = {color = params.hoverColor}
		}
	end
	Widg.defaults.comboBox = {
		x = 0,
		y = 0,
		itemHeight = 20,
		maxDisplayItems = 5,
		width = 50,
		onSelectionChanged = false, -- function(newchoice, oldchoice)
		onInit = false,
		item = basicItem, -- function(choice, params) : Creates the unselected actor for one choice
		selection = basicSelection, -- function(choice, params) : Creates the selected actor for one choice
		--Note that the return value must have a SetText(val) field to update it's position
		commands = {},
		selectionColor = Color.Black,
		hoverColor = Color.Blue,
		itemColor = Color.Black,
		choices = {"default"},
		choice = false,
		numitems = false,
		scrollable = false
	}
	Widg.ComboBox = function(params, updateActor)
		fillNilTableFieldsFrom(params, Widg.defaults.comboBox)
		params.selectionColor = checkColor(params.selectionColor)
		params.itemColor = checkColor(params.itemColor)
		params.hoverColor = checkColor(params.hoverColor)

		local combobox =
			Widg.Container {
			x = params.x - params.width / 2,
			y = params.y - params.itemHeight / 2,
			onInit = params.onInit,
			visible = params.visible
		}
		fillNilTableFieldsFrom(combobox, params.commands)

		if not params.numitems then
			params.numitems = #(params.choices)
		end
		combobox.choices = params.choices
		combobox.droppedDown = false
		combobox.selected = 1
		if params.choice then
			for i, v in ipairs(params.choices) do
				if v == params.choice then
					combobox.selected = i
				end
			end
		end
		combobox.items = Widg.Container {visible = false}
		combobox.selection = Widg.Container {}
		combobox.selection.graphic = params.selection(combobox.choices[combobox.selected], params)
		combobox.selection:add(combobox.selection.graphic)
		combobox.selection:add(
			Widg.Rectangle {
				visible = false,
				width = params.width,
				height = params.itemHeight,
				onClick = function(self)
					combobox.droppedDown = not combobox.droppedDown
					local items = combobox.items.actor
					items:visible(combobox.droppedDown)
				end
			}
		)

		for i = 1, params.numitems do
			(combobox.items):add(
				Widg.Container {
					y = i * params.itemHeight,
					content = {
						params.item(combobox.choices[i], params),
						Widg.Rectangle {
							width = params.width,
							height = params.itemHeight,
							visible = false,
							onClick = function(self)
								if combobox.droppedDown then
									local g = combobox.selection.graphic
									g:SetText(combobox.choices[i])
									if params.onSelectionChanged then
										params.onSelectionChanged(combobox.choices[i], combobox.choices[combobox.selected])
									end
									combobox.selected = i
									combobox.droppedDown = false
									local items = combobox.items.actor
									items:visible(combobox.droppedDown)
								end
							end
						}
					}
				}
			)
		end

		if params.scrollable then
			combobox:add(
				Widg.Scrollable {
					width = params.width,
					height = params.itemHeight * (params.numitems + 1),
					content = combobox.items
				}
			)
		else
			combobox:add(combobox.items)
		end
		combobox:add(combobox.selection)
		return combobox
	end
end

Widg.defaults.textbox = {
	-- All the Widg.button defaults and
	text = "",
	numeric = false,
	cursor = {
		blinkInterval = 0.15,
		color = color "ffffff"
	},
	-- filter(key) == true => accept input (Otherwise discard it)
	filter = false, -- If numeric is used then filter is set automatically
	cursorOffset = 0, -- <=0 (Ex. if it's -1 then cursor would be as|d (The |))
	onValueChangeEnd = false, -- Called when esc or enter is pressed
	onValueChange = false, -- Called on each change to the string
	clearOnEsc = false, -- Set to empty string when ending input with escape
	inputStartCondition = function()
		return not SCREENMAN:get_input_redirected(PLAYER_1)
	end
}
do
	-- delete => remove from in front of the cursor (Otherwise behind)
	local function firstChunk(s, offset, delete)
		return s:sub(1, offset - (delete and 1 or 2))
	end
	local function secondChunk(s, offset, delete)
		return s:sub((delete and 2 or 1) + s:len() + offset)
	end
	local function removeCharacter(s, offset, delete)
		return firstChunk(s, offset, delete) .. secondChunk(s, offset, delete)
	end
	local function addStrWithOffset(s, offset, new)
		return firstChunk(s, offset, true) .. new .. secondChunk(s, offset, false)
	end
	Widg.mts.TextBox = {}
	Widg.mts.TextBox.__index = Widg.mts.TextBox
	function Widg.mts.TextBox:SetCursor(offset)
		self.cursorOffset = offset
		local len = self.text:len()
		self.cursor.actor:x(self.params.width / 2 + (len / 2 + offset) * 6.5)
	end
	function Widg.mts.TextBox:SetText(str)
		local params = self.params
		if params.numeric then
			str = str == "" and "0" or ((str:len() > 1 and str:sub(1, 1) == "0") and (str:sub(2)) or str)
		end
		self.text = str
		self:ButtonSetText(str)
		self:SetCursor(self.cursorOffset)
		if params.onValueChange then
			params.onValueChange(self, str)
		end
	end
	function Widg.mts.TextBox:SetActive(active)
		self.active = active
		self.cursor.actor:visible(false)
		if active then
			self:SetCursor(0)
		elseif self.params.onValueChangeEnd then
			self.params.onValueChangeEnd()
		end
		SCREENMAN:set_input_redirected(PLAYER_1, active)
		self:SetText(self.text)
	end
	function Widg.mts.TextBox:EndInput()
		self:SetActive(false)
		if self.params.onValueChangeEnd then
			self.params.onValueChangeEnd(self, self.text)
		end
	end
	function Widg.mts.TextBox:StartInput()
		self:SetActive(true)
	end
	--[[
		Defined using a button widget with an input callback.
		Has a movable cursor
		TODO: Handle cursor positioning better
		TODO: Selections and copying (Needs C++ too iirc)
	]]
	Widg.TextBox = function(params)
		fillNilTableFieldsFrom(params, Widg.defaults.textbox)
		fillNilTableFieldsFrom(params, Widg.defaults.button)
		if params.numeric then
			params.filter = tonumber
			params.text = params.text == "" and "0" or params.text
		end
		local cursor =
			Widg.Label {
			text = "I",
			color = params.cursor.color,
			scale = params.font.scale,
			x = params.width / 2,
			visible = false,
			y = params.height / 2
		}
		local textbox
		local inputF = function(event)
			if not textbox.active then
				return false
			end
			-- We want first press and repeat only
			if event.type ~= "InputEventType_Release" then
				if event.button == "Start" then
					textbox:EndInput()
					return true
				elseif event.button == "Back" then
					if params.clearOnEsc then
						textbox:SetText("")
					end
					textbox:EndInput()
					return true
				elseif event.DeviceInput.button == "DeviceButton_backspace" then
					textbox:SetText(removeCharacter(textbox.text, textbox.cursorOffset))
				elseif event.DeviceInput.button == "DeviceButton_delete" then
					textbox:SetText(removeCharacter(textbox.text, textbox.cursorOffset, true))
					textbox:SetCursor(math.min(textbox.cursorOffset + 1, 0))
				else
					local key = event.DeviceInput.button:match("DeviceButton_(.+)")
					if key == "v" and INPUTFILTER:IsControlPressed() then
						textbox:SetText(addStrWithOffset(textbox.text, textbox.cursorOffset, HOOKS:GetClipboard()))
					elseif key:len() == 1 and ((not params.filter) or params.filter(key)) then
						-- xor
						if INPUTFILTER:IsBeingPressed("caps lock") ~= INPUTFILTER:IsShiftPressed() then
							key = key:upper()
						end
						textbox:SetText(addStrWithOffset(textbox.text, textbox.cursorOffset, key))
					elseif key == "left" then
						textbox:SetCursor(math.max(textbox.cursorOffset - 1, -textbox.text:len()))
					elseif key == "right" then
						textbox:SetCursor(math.min(textbox.cursorOffset + 1, 0))
					elseif key == "home" then
						textbox:SetCursor(-textbox.text:len())
					elseif key == "space" then
						textbox:SetText(addStrWithOffset(textbox.text, textbox.cursorOffset, " "))
					elseif key == "tab" then
						textbox:SetText(addStrWithOffset(textbox.text, textbox.cursorOffset, "\t"))
					end
				end
			end
		end
		textbox =
			Widg.Button(
			setmetatable(
				{
					text = params.text,
					highlightCondition = function(self)
						return textbox.active or isOver(self)
					end,
					onRightClick = function()
						textbox:EndInput()
					end
				},
				{__index = params}
			)
		)
		textbox.bg.LeftClickMessageCommand = function(self)
			if not textbox.active then
				if (not params.inputStartCondition) or params.inputStartCondition() and isOver(self) then
					textbox:StartInput()
				end
			elseif not isOver(self) then
				textbox:EndInput()
			end
		end
		textbox.params = params
		local blinkingCursor
		do
			local i = 0
			local lastClock = 0
			local b = true
			local interval = params.cursor.blinkInterval
			blinkingCursor = function(self)
				highlight(self)
				local newClock = os.clock()
				i = i - lastClock + newClock
				lastClock = newClock
				if i > interval then
					i = 0
					b = not b
					cursor.actor:settext("")
				end
			end
		end
		textbox.OnCommand = function(self)
			SCREENMAN:GetTopScreen():AddInputCallback(inputF)
			self:SetUpdateFunction(blinkingCursor)
		end
		textbox.text = params.text
		textbox.cursorOffset = params.cursorOffset
		textbox.ButtonSetText = textbox.SetText
		setmetatable(textbox, Widg.mts.TextBox)
		textbox.SetText = Widg.mts.TextBox.SetText -- Otherwise there's a collision with Button's
		textbox:add(cursor)
		textbox.cursor = cursor
		return textbox
	end
end
