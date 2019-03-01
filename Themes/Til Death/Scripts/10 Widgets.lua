Widg = {}
Widg.defaults = {}
function fillNilTableFieldsFrom(table1, defaultTable)
	for key, value in pairs(defaultTable) do
		if table1[key] == nil then
			table1[key] = defaultTable[key]
		end
	end
end
function checkColor(c)
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

Widg.defaults.container = {
	x = 0,
	y = 0,
	onInit = false,
	content = false,
	visible = true,
	name = "Container"
}
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
				params.onInit(self)
			end
		end
	}
	container.add = function(container, item)
		container[#container + 1] = item
	end
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
	valign = 0.5,
	onInit = false
}
Widg.Label = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.label)
	params.color = checkColor(params.color)

	local initText = params.text
	local label
	label =
		LoadFont(params.name) ..
		{
			Name = "Label",
			InitCommand = function(self)
				label.actor = self
				self:xy(params.x, params.y):zoom(params.scale):halign(params.halign):valign(params.valign)
				if type(params.width) == "number" then
					self:maxwidth(params.width / params.scale)
				end
				self:settext(initText):diffuse(params.color)
				if params.onInit then
					params.onInit(self)
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
	return label
end

Widg.defaults.rectangle = {
	x = 0,
	y = 0,
	width = 100,
	height = 100,
	color = color("#FFFFFF"),
	onClick = false,
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
				params.onInit(self)
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
					params.onClick(self)
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
	local borders =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(params.x, params.y)
			if params.onInit then
				params.onInit(self)
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
				params.onInit(self)
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
	bgColor = color("#bb00bbFF"),
	border = {
		color = Color.Blue,
		width = 2
	},
	highlight = {
		color = color("#dd00ddFF"),
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
		color = Color.White,
		padding = {
			x = 10,
			y = 10
		}
	},
	halign = 0.5,
	valign = 0.5,
	texture = false,
	enabled = true
}

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
		onInit = function(self)
			if params.onInit then
				params.onInit(self)
			end
			if params.highlight then
				self:SetUpdateFunction(highlight)
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
			x = params.x,
			color = params.bgColor,
			y = params.y,
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
			x = params.x,
			color = params.highlight.color,
			y = params.y,
			texture = "buttons/" .. params.highlight.texture,
			width = params.width,
			height = params.height,
			halign = params.halign - 0.5,
			valign = params.valign - 0.5
		} or
		Def.ActorFrame {}

	button.bg =
		Widg.Rectangle {
		x = params.x,
		y = params.y,
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
		halign = params.halign,
		valign = params.valign,
		visible = not params.texture
	}
	button.bg.HighlightCommand = params.highlight and function(self)
			local mainActor = params.texture and button.sprite.actor or self
			local isOver = isOver(self)
			if params.highlight.texture then
				(button.highlightSprite.actor):visible(isOver)
			end
			if isOver then
				if params.highlight.color then
					mainActor:diffuse(params.highlight.color)
				end
				mainActor:diffusealpha(params.highlight.alpha or params.alpha or 1)
				if params.onHighlight then
					params.onHighlight(mainActor)
				end
			else
				if params.bgColor then
					mainActor:diffuse(params.bgColor)
				end
				mainActor:diffusealpha(params.alpha)
				if params.onUnhighlight then
					params.onUnhighlight(mainActor)
				end
			end
		end or nil

	button.borders =
		(params.texture or not params.border) and Def.ActorFrame {} or
		Widg.Borders {
			y = params.y + params.height * (0.5 - params.valign),
			x = params.x + params.width * (0.5 - params.halign),
			color = params.border.color,
			width = params.width,
			height = params.height,
			borderWidth = params.border.width,
			alpha = params.texture and 0 or params.alpha
		}

	button.label =
		Widg.Label {
		x = params.x + params.width * (1 - params.halign),
		y = params.y + params.height * (1 - params.valign),
		scale = params.font.scale,
		halign = params.font.halign,
		text = params.text,
		width = params.width - params.font.padding.x
	}

	button.settext = function(button, text)
		return (button.label):settext(text)
	end
	button.GetText = function(button)
		return (button.label):GetText()
	end
	button.diffuse = function(button, color)
		params.bgColor = color
		return (button.bg.actor):diffuse(color)
	end
	button.Enable = function(button)
		button.enabled = true
		return (button.actor):visible(button.enabled)
	end
	button.Disable = function(button)
		button.enabled = false
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
	local scrollable =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(params.x + params.halign * params.width, params.y + params.valign * params.height)
			self.AFT = AFT
			self.sprite = sprite
			self.content = content
			self:SetTextureFiltering(true)
			if params.onInit then
				params.onInit(self, content, AFT, sprite)
			end
		end,
		AFT,
		sprite
	}
	scrollable.content = content
	return scrollable
end

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
	width = 100,
	height = 30,
	onClick = false,
	color = color("#FFFFFFFF"),
	onValueChangeEnd = false, -- Called when the mouse click is released
	onValueChange = false, -- Recieves the value between min and max
	handle = basicHandle,
	bar = basicBar,
	onInit = false,
	defaultValue = 10, -- starting value
	max = 100,
	min = 0,
	step = 1,
	halign = 1,
	valign = 1,
	vertical = false, -- todo
	isRange = false, --  todo
	bindToTable = {} -- Since tables are passed by reference, update t.value with the slider value.
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
		params.vertical and getRatioforAxis(mouse.y, getTrueY(slider), params.height, params.valign) or
		getRatioforAxis(mouse.x, getTrueX(slider), params.width, params.halign)
	return math.round((ratio * length + params.min) / params.step) * params.step
end
Widg.SliderBase = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.sliderBase)
	params.color = checkColor(params.color)
	local bar = params.bar(params)
	local handle = params.handle(params)
	local updateFunction
	local container =
		Widg.Container {
		visible = params.visible,
		x = params.x,
		y = params.y,
		onInit = function(container)
			handle.onValueChange(params.defaultValue)
			container:SetUpdateFunction(updateFunction)
			if params.onInit then
				params.onInit(container)
			end
		end
	}
	if params.range and type(params.defaultValue) ~= "table" then
		params.defaultValue = {params.defaultValue, params.defaultValue}
	end
	local t = params.bindToTable
	t.value = defaultValue
	container.value = t.value
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
			clicked = true
		end,
		visible = false
	}
	container:add(rectangle)
	updateFunction = function(container)
		if clicked then
			if isOver(rectangle.actor) and INPUTFILTER:IsBeingPressed("Mouse 0", "Mouse") then
				local mouse = getMousePosition()
				t.value = getValue(mouse, params, container)
				container.value = t.value
				if params.onValueChange then
					params.onValueChange(t.value)
				end
				if handle.onValueChange then
					handle.onValueChange(t.value)
				end
				if bar.onValueChange then
					bar.onValueChange(t.value)
				end
			else
				clicked = false
				if params.onValueChangeEnd then
					params.onValueChangeEnd(t.value)
				end
				if bar.onValueChange then
					bar.onValueChange(t.value)
				end
				if bar.onValueChangeEnd then
					bar.onValueChangeEnd(t.value)
				end
			end
		end
	end
	return container
end

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
	onSelectionChanged = false, -- (newchoice, oldchoice)
	onInit = false,
	item = basicItem,
	selection = basicSelection, -- must understand :settext
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
								g:settext(combobox.choices[i])
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
