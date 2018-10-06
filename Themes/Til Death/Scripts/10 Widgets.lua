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
Widg.defaults.label = {
	x = 0,
	y = 0,
	scale = 1.0,
	text = "Label",
	name = "Common Normal",
	width = false,
	color = color("#FFFFFF"),
	halign = 0.5,
	onInit = false
}
Widg.Label = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.label)
	params.color = checkColor(params.color)
	return LoadFont(params.name) ..
		{
			Name = "Label",
			InitCommand = function(self)
				self:xy(params.x, params.y):zoom(params.scale):halign(params.halign)
				if type(params.width) == "number" then
					self:maxwidth(params.width / params.scale)
				end
				if params.onInit then
					params.onInit(self)
				end
			end,
			BeginCommand = function(self)
				self:settext(params.text):diffuse(params.color)
			end
		}
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
		OnCommand = function(self)
			self:diffuse(params.color)
		end,
		LeftClickMessageCommand = params.onClick and function(self)
				if params.onClick and os.clock() - lastClick > params.clickPolling and isOver(self) then
					lastClick = os.clock()
					params.onClick(self)
				end
			end or nil
	}
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
	return Def.ActorFrame {
		InitCommand = function(self)
			self:xy(params.x, params.y)
			if params.onInit then
				params.onInit(self)
			end
		end,
		--4 border quads
		Widg.Rectangle({width = params.borderWidth, height = params.height, color = params.color, alpha = params.alpha}), --left
		Widg.Rectangle({width = params.width, height = params.borderWidth, color = params.color, alpha = params.alpha}), --top
		Widg.Rectangle(
			{
				x = params.width - params.borderWidth,
				width = params.borderWidth,
				height = params.height,
				color = params.color,
				alpha = params.alpha
			}
		), --right
		Widg.Rectangle(
			{
				y = params.height - params.borderWidth,
				width = params.width,
				height = params.borderWidth,
				color = params.color,
				alpha = params.alpha
			}
		) --bottom
	}
end
local function highlight(self)
	self:queuecommand("Highlight")
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
	local sprite =
		Def.Sprite {
		_Level = 1,
		Texture = path,
		InitCommand = function(self)
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
	halign = 1,
	valign = 1,
	texture = false
}
Widg.Button = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.button)
	params.highlight.color = checkColor(params.highlight.color)
	params.bgColor = checkColor(params.bgColor)
	params.font.color = checkColor(params.font.color)
	params.border.color = checkColor(params.border.color)
	local sprite = Def.ActorFrame {}
	local spriteActor = nil
	if params.texture then
		sprite =
			Widg.Sprite {
			x = params.x,
			color = params.bgColor,
			y = params.y,
			texture = "buttons/" .. params.texture,
			width = params.width,
			height = params.height,
			halign = params.halign - 0.5,
			valign = params.valign - 0.5,
			onInit = function(s)
				spriteActor = s
			end
		}
	end
	local rect =
		Widg.Rectangle {
		x = params.x,
		y = params.y,
		width = params.width,
		height = params.height,
		color = params.bgColor,
		alpha = params.texture and 0 or params.alpha,
		onClick = params.onClick and function(s)
				params.onClick(s)
			end or false,
		halign = params.halign,
		valign = params.valign,
		visible = not params.texture
	}
	rect.HighlightCommand = function(self)
		local mainActor = params.texture and spriteActor or self
		if isOver(self) then
			if params.highlight.color then
				mainActor:diffuse(params.highlight.color)
			end
			mainActor:diffusealpha(params.highlight.alpha or params.alpha or 1)
			if params.onHighlight then
				params.onHighlight(mainActor)
			end
		else
			mainActor:diffuse(params.bgColor):diffusealpha(params.alpha)
			if params.onUnhighlight then
				params.onUnhighlight(mainActor)
			end
		end
	end
	local borders =
		params.texture and Def.ActorFrame {} or
		Widg.Borders {
			y = params.y + params.height * (0.5 - params.valign),
			x = params.x + params.width * (0.5 - params.halign),
			color = params.border.color,
			width = params.width,
			height = params.height,
			borderWidth = params.border.width,
			alpha = params.texture and 0 or params.alpha
		}
	local label = false
	local initText = false
	local button =
		Def.ActorFrame {
		InitCommand = function(self)
			self:SetUpdateFunction(highlight)
			self.params = params
		end,
		rect,
		sprite,
		Widg.Label {
			x = params.x + params.width * (1 - params.halign),
			y = params.y + params.height * (1 - params.valign),
			scale = params.font.scale,
			halign = params.font.halign,
			text = params.text,
			width = params.width - params.font.padding.x,
			onInit = function(self)
				label = self
				if initText then
					self:settext(initText)
				end
			end
		},
		borders
	}
	button.settext = function(button, text)
		if label then
			label:settext(text)
		else
			initText = text
		end
	end
	return button
end
Widg.defaults.container = {
	x = 0,
	y = 0,
	onInit = false,
	content = false,
	visible = true
}
Widg.Container = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.container)
	local container =
		Def.ActorFrame {
		InitCommand = function(self)
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
		if params.content.class then
			container[#container + 1] = params.content
		else
			container[#container + 1] = Def.ActorFrame(params.content)
		end
	end
	return container
end
Widg.defaults.scrollable = {
	width = 100,
	height = 100,
	content = false,
	textureName = false,
	x = 100,
	y = 100,
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
			self:halign(params.halign):valign(params.valign)
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
			self:Create()
			self:Draw()
		end,
		Def.ActorFrame {
			Name = "Draw",
			content
		}
	}
	local scrollable =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(params.x, params.y)
			self.AFT = AFT
			self.sprite = sprite
			self.content = content
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
	vertical = false,
	isRange = false,
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
		height = params.itemHeight
	}
end
local function basicItem(choice, params)
	return Widg.Button {
		text = choice,
		width = params.width,
		height = params.itemHeight,
		color = params.itemColor,
		highlight = {color = params.itemHoverColor}
	}
end
Widg.defaults.comboBox = {
	x = 0,
	y = 0,
	itemHeight = 20,
	maxDisplayItems = 5,
	width = 50,
	onSelectionChanged = false,
	onInit = false,
	item = basicItem,
	selection = basicSelection, -- must understand :settext
	commands = {},
	selectionColor = Color.Black,
	itemHoverColor = Color.Blue,
	itemColor = Color.Black,
	choices = {"default"}
}
Widg.ComboBox = function(params, updateActor)
	fillNilTableFieldsFrom(params, Widg.defaults.comboBox)
	params.selectionColor = checkColor(params.selectionColor)
	params.itemColor = checkColor(params.itemColor)
	params.itemHoverColor = checkColor(params.itemHoverColor)
	fillNilTableFieldsFrom(frame, params.commands)
	frame.droppedDown = params.droppedDown
	frame.choices = params.choices
	frame.selected = 1

	local choicesActor
	local choices =
		Widg.Container {
		onInit = function(self)
			choicesActor = self
		end
	}

	local selectionGraphic = params.selection(frame.choices[frame.selected])
	local selection =
		Widg.Container {
		content = {
			selectionGraphic,
			Widg.Rectangle {
				visible = false,
				width = params.width,
				height = params.itemHeight,
				onClick = function(self)
					frame.droppedDown = not frame.droppedDown
					choicesActor:visible(frame.droppedDown)
				end
			}
		}
	}

	for i, v in pairs(frame.choices) do
		choices:add(
			Widg.Container {
				y = i * params.selectionParams.height,
				content = {
					Widg.Rectangle {
						onClick = function(self)
							if frame.droppedDown then
								selectionGraphic:settext(frame.choices[i])
								if onSelectionChanged then
									onSelectionChanged(frame.choices[i], frame.choices[frame.selected])
								end
								frame.selected = i
							end
						end
					},
					params.item(frame.choices[i], params)
				}
			}
		)
	end

	--[[frame:add(Widg.Scrollable{
		--width = params.selectionParams.width,
		width = 64,
		height = 64,
		halign = 0,
		x = -30,
		y = 0,
		content = choices
	})]]
	local frame =
		Widg.Container {
		x = params.x,
		y = params.y
	}
	frame:add(selection)
	frame:add(choices)
	return frame
end
