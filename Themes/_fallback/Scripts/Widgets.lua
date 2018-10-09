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
		SCREENMAN:SystemMessage(c)
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
				if width then
					self:maxwidth(params.width)
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
	visible = true
}
Widg.Rectangle = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.rectangle)
	params.color = checkColor(params.color)
	return Def.Quad {
		InitCommand = function(self)
			self:xy(params.x + params.width / 2, params.y + params.height / 2):zoomto(params.width, params.height):diffusealpha(
				params.alpha
			):halign(params.halign):valign(params.valign)
			if params.onInit then
				params.onInit(self)
			end
			self:visible(params.visible)
		end,
		OnCommand = function(self)
			self:diffuse(params.color)
		end,
		LeftClickMessageCommand = params.onClick and function(self)
				if params.onClick and isOver(self) then
					params.onClick(self)
				end
			end or nil
	}
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
	local button = Def.ActorFrame {
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
				if initText then self:settext(initText) end
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
	content = false
}
Widg.Container = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.container)
	local container =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(params.x, params.y)
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

Widg.defaults.sliderBase = {
	x = 0,
	y = 0,
	width = 100,
	height = 30,
	onClick = false,
	onValueChangeEnd = false,
	onValueChange = false,
	handle = {},
	bar = {},
	onInit = false,
	defaultValue = 10,
	max = 100,
	min = 0,
	step = 1,
	halign = 0.5,
	valign = 0.5,
	vertical = false,
	isRange = false,
	bindToTable = {} -- Since tables are passed by reference, update t.value with the slider value.
	-- If range, value = {start=number, end=number}
}
local function getValueforAxis(mpos, pos, len, align, valuecount)
	return (mpos - pos + len / 2 + len * align) / (len / valuecount)
end
local function getValue(mouse, params)
	local length = (params.max - params.min) / params.step
	local value =
		params.vertical and getvalueForAxis(mouse.y, params.y, params.height, params.valign, length) or
		getvalueForAxis(mouse.x, params.x, params.width, params.halign, length)
	return value * params.step + params.min
end
Widg.SliderBase = function(params)
	fillNilTableFieldsFrom(params, Widg.defaults.sliderBase)
	local updateFunction
	local container =
		Widg.Container {
		x = params.x,
		y = params.y,
		onInit = function(container)
			container:SetUpdateFunction(updateFunction)
			params.onInit(container)
		end
	}
	if params.range and type(params.defaultValue) ~= "table" then
		params.defaultValue = {params.defaultValue, params.defaultValue}
	end
	local bar = params.bar(params)
	local handle = params.handle(params)
	local t = params.bindToTable
	t.value = defaultValue
	container.add(var)
	container.add(handle)
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
	container.add(rectangle)
	updateFunction = function(container)
		if clicked then
			if isOver(rectangle) and INPUTFILTER:IsBeingPressed("Mouse 0", "Mouse") then
				local mouse = getMousePosition()
				t.value = getValue(mouse, params)
				params.onValueChange(t.value)
				handle.onValueChange(t.value)
				bar.onValueChange(t.value)
			else
				clicked = false
				params.onValueChangeEnd(t.value)
				handle.onValueChangeEnd(t.value)
				bar.onValueChangeEnd(t.value)
			end
		end
	end
	return container, t
end

Widg.defaults.comboBox = {
	x = 0,
	y = 0,
	onSelectionChanged = false,
	onInit = false,

	selectionParams = {},
	headerParams = {},
	
	commands = {},

	headerColor = Color.Black,
	selectionColor = Color.Blue,
	defaultColor = Color.Black,

	droppedDown = false,
	choices = {"default"}
}
Widg.ComboBox = function(params, updateActor)
	fillNilTableFieldsFrom(params, Widg.defaults.comboBox)
	fillNilTableFieldsFrom(params.selectionParams, Widg.defaults.button)
	fillNilTableFieldsFrom(params.headerParams, Widg.defaults.button)
	params.selectionColor = checkColor(params.selectionColor)
	params.defaultColor = checkColor(params.defaultColor)
	
	local frame = Widg.Container{
		x = params.x,
		y = params.y
	}
	fillNilTableFieldsFrom(frame, params.commands)
	frame.droppedDown = params.droppedDown
	frame.choices = params.choices
	frame.selected = 1

	local headerParams = {
		y = 0,
		text = frame.choices[frame.selected],
		onClick = function(self)
			frame.droppedDown = not frame.droppedDown
		end
	}
	fillNilTableFieldsFrom(headerParams, params.headerParams)
	local header = Widg.Button(headerParams)
	
	local choices = Widg.Container{
		--x = 32,
		onInit = function(self)
			self:SetUpdateFunction(function()
				self:visible(frame.droppedDown)
			end)
		end
	}

	frame:add(header)
	frame:add(choices)
	--[[frame:add(Widg.Scrollable{
		--width = params.selectionParams.width,
		width = 64,
		height = 64,
		halign = 0,
		x = -30,
		y = 0,
		content = choices
	})]]

	for i,v in pairs(frame.choices) do
		local selectionParams = {
			y = i*params.selectionParams.height,
			text = frame.choices[i],
			bgColor = params.defaultColor,
			onClick = function(self)
				if frame.droppedDown then
					frame.selected = i
					header:settext(frame.choices[i])
					if onSelectionChanged then
						onSelectionChanged(self)
					end
				end
			end,
			onHighlight = function(self)
				if frame.selected == i then
					self:diffuse(params.selectionColor)
				end
			end,
			onUnhighlight = function(self)
				if frame.selected == i then
					self:diffuse(params.selectionColor)
				end
			end
		}
		fillNilTableFieldsFrom(selectionParams, params.selectionParams)
		choices:add(Widg.Button(selectionParams))
	end

	return frame
end