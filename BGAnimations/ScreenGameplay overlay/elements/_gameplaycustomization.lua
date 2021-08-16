-- gameplay customization
-- assume that customization is active if this file is loaded

local snm = Var("LoadingScreen")

local ratios = {
    MenuHeight = 500 / 1080,
    MenuWidth = 600 / 1920,
    MenuDraggerHeight = 75 / 1080,
    EdgePadding = 10 / 1920,
}
local actuals = {
    MenuHeight = ratios.MenuHeight * SCREEN_HEIGHT,
    MenuWidth = ratios.MenuWidth * SCREEN_WIDTH,
    MenuDraggerHeight = ratios.MenuDraggerHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
}

local uiBGAlpha = 0.6
local textButtonHeightFudgeScalarMultiplier = 1.4
local buttonHoverAlpha = 0.6
local cursorAlpha = 0.5
local cursorAnimationSeconds = 0.05

local function spaceNotefieldCols(inc)
	if inc == nil then inc = 0 end
	local hCols = math.floor(#noteColumns/2)
	for i, col in ipairs(noteColumns) do
	    col:addx((i-hCols-1) * inc)
	end
end

local t = Def.ActorFrame {
    Name = "GameplayElementsCustomizer",
    InitCommand = function(self)
        -- in the off chance we end up in customization when in syncmachine, dont turn on autoplay
        if snm ~= "ScreenGameplaySyncMachine" then
            GAMESTATE:SetAutoplay(true)
        else
            GAMESTATE:SetAutoplay(false)
        end
    end,
    BeginCommand = function(self)
        local screen = SCREENMAN:GetTopScreen()

        local lifebar = screen:GetLifeMeter(PLAYER_1)
        local nf = screen:GetChild("PlayerP1"):GetChild("NoteField")
        local noteColumns = nf:get_column_actors()

        registerActorToCustomizeGameplayUI(lifebar)
        registerActorToCustomizeGameplayUI(nf)

        Movable.pressed = false
        Movable.current = "None"
        Movable.DeviceButton_r.element = nf
        Movable.DeviceButton_t.element = noteColumns
        Movable.DeviceButton_r.condition = true
        Movable.DeviceButton_t.condition = true
        --self:GetChild("LifeP1"):GetChild("Border"):SetFakeParent(lifebar)
        Movable.DeviceButton_j.element = lifebar
        Movable.DeviceButton_j.condition = true
        Movable.DeviceButton_k.element = lifebar
        Movable.DeviceButton_k.condition = true
        Movable.DeviceButton_l.element = lifebar
        Movable.DeviceButton_l.condition = true
        Movable.DeviceButton_n.condition = true
        Movable.DeviceButton_n.DeviceButton_up.arbitraryFunction = spaceNotefieldCols
        Movable.DeviceButton_n.DeviceButton_down.arbitraryFunction = spaceNotefieldCols
    end,

    Def.ActorFrame {
        Name = "LifeP1",
        MovableBorder(200, 5, 1, -35, 0)
    },
}

local function makeUI()
    local itemsPerPage = 8
    local itemListFrame = nil

    -- init moved to OnCommand due to timing reasons
    local elements = {}
    local page = 1
    local maxPage = 1

    local cursorPos = 1
    local selectedElement = nil

    local function movePage(n)
        if maxPage <= 1 then
            return
        end

        -- math to make pages loop both directions
        local nn = (page + n) % (maxPage + 1)
        if nn == 0 then
            nn = n > 0 and 1 or maxPage
        end
        page = nn

        local currentPageLowerBound = (page-1) * itemsPerPage + 1
        local currentPageUpperBound = page * itemsPerPage
        if cursorPos < currentPageLowerBound then
            cursorPos = currentPageLowerBound
            if itemListFrame ~= nil then
                itemListFrame:queuecommand("UpdateCursor")
            end
        elseif cursorPos > currentPageUpperBound then
            cursorPos = currentPageUpperBound
            if itemListFrame ~= nil then
                itemListFrame:queuecommand("UpdateCursor")
            end
        end

        if itemListFrame ~= nil then
            itemListFrame:playcommand("UpdateItemList")
        end
    end

    local function moveCursor(n)
        cursorPos = cursorPos + n
        if cursorPos > #elements then
            cursorPos = 1
            page = 1
            if maxPage ~= 1 and itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        elseif cursorPos < 1 then
            cursorPos = #elements
            page = maxPage
            if maxPage ~= 1 and itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        end

        local currentPageLowerBound = (page-1) * itemsPerPage + 1
        local currentPageUpperBound = page * itemsPerPage
        if cursorPos < currentPageLowerBound then
            -- lazy: move only 1 page but its possible to move many pages
            page = page-1
            if itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        elseif cursorPos > currentPageUpperBound then
            -- lazy: move only 1 page but its possible to move many pages
            page = page+1
            if itemListFrame ~= nil then
                itemListFrame:playcommand("UpdateItemList")
            end
        end

        if itemListFrame ~= nil then
            itemListFrame:playcommand("UpdateCursor")
        end
    end

    local function item(i)
        local index = i
        local element = elements[index]
        return UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "Item_"..i,
            InitCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                txt:halign(0)
                bg:halign(0)
                self:x(-actuals.MenuWidth + actuals.EdgePadding)
                local allowedSpace = actuals.MenuHeight - actuals.MenuDraggerHeight - (actuals.EdgePadding*2)
                local topItemY = -actuals.MenuHeight/2 + actuals.MenuDraggerHeight + actuals.EdgePadding
                self:y(topItemY + (allowedSpace / itemsPerPage) * (i-1) + (allowedSpace / itemsPerPage / 2))

                self.alphaDeterminingFunction = function(self)
                    local hovermultiplier = isOver(bg) and buttonHoverAlpha or 1
                    local visiblemultiplier = self:IsInvisible() and 0 or 1
                    self:diffusealpha(1 * hovermultiplier * visiblemultiplier)
                end
            end,
            SetItemCommand = function(self)
                local txt = self:GetChild("Text")
                local bg = self:GetChild("BG")
                index = (page - 1) * itemsPerPage + i
                element = elements[index]
                if element ~= nil then
                    self:diffusealpha(1)
                    txt:settext(element:GetName())
                    bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                else
                    self:diffusealpha(0)
                end
            end,
            UpdateCursorCommand = function(self)
                if index == cursorPos then
                    local cursor = self:GetParent():GetChild("Cursor")
                    cursor:finishtweening()
                    cursor:smooth(cursorAnimationSeconds)
                    cursor:xy(self:GetX(), self:GetY())
                    local bg = self:GetChild("BG")
                    cursor:zoomto(bg:GetZoomedWidth(), bg:GetZoomedHeight())
                    cursor:diffusealpha(cursorAlpha)
                end
            end,
            RolloverUpdateCommand = function(self, params)
                if self:IsInvisible() then return end
                self:alphaDeterminingFunction()
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    self:alphaDeterminingFunction()
                end
            end,
        }
    end

    local t = Def.ActorFrame {
        Name = "ItemListContainer",
        InitCommand = function(self)
            itemListFrame = self
        end,
        OnCommand = function(self)
            -- these are initialized here because most elements either need BeginCommand or InitCommand to run for them to be registered
            -- Order of execution: Init -> Begin -> On
            elements = getCustomizeGameplayElements()
            table.sort(
                elements,
                function(a, b)
                    return a:GetName():lower() < b:GetName():lower()
                end
            )
            maxPage = math.ceil(#elements / itemsPerPage)

            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type ~= "InputEventType_Release" then -- allow Repeat and FirstPress
                    local gameButton = event.button
                    local key = event.DeviceInput.button
                    local up = gameButton == "Up" or gameButton == "MenuUp"
                    local down = gameButton == "Down" or gameButton == "MenuDown"
                    local right = gameButton == "MenuRight" or gameButton == "Right"
                    local left = gameButton == "MenuLeft" or gameButton == "Left"
                    local enter = gameButton == "Start"
                    local ctrl = INPUTFILTER:IsBeingPressed("left ctrl") or INPUTFILTER:IsBeingPressed("right ctrl")
                    local shift = INPUTFILTER:IsShiftPressed()
                    local back = key == "DeviceButton_escape" or key == "DeviceButton_backspace"

                    if selectedElement ~= nil then
                        if up or down or left or right then
                            if shift then
                                -- fine movement
                            else
                                -- regular movement
                            end
                        elseif back then
                            if ctrl then
                                -- reset to default
                            else
                                -- undo changes and return
                            end
                        elseif enter then
                            -- save position and return
                        end
                    else
                        if up or left then
                            -- up
                            moveCursor(-1)
                        elseif down or right then
                            -- down
                            moveCursor(1)
                        elseif enter then
                            -- select category, select element
                        elseif back then
                            -- up one level
                        end
                    end
                end
            end)
            self:playcommand("UpdateItemList")
            self:playcommand("UpdateCursor")
            self:finishtweening()
        end,
        UpdateItemListCommand = function(self)
            self:playcommand("SetItem")
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(1)
                self:zoomto(actuals.MenuWidth, actuals.MenuHeight)
                registerActorToColorConfigElement(self, "main", "PrimaryBackground")
                self:diffusealpha(uiBGAlpha)
            end,
            MouseScrollMessageCommand = function(self, params)
                if isOver(self) then
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                    self:GetParent():playcommand("UpdateItemList")
                end
            end
        },
        UIElements.QuadButton(1) .. { 
            Name = "DraggableLip",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:y(-actuals.MenuHeight / 2)
                self:zoomto(actuals.MenuWidth, actuals.MenuDraggerHeight)
                registerActorToColorConfigElement(self, "main", "SecondaryBackground")
                self:diffusealpha(uiBGAlpha)
            end,
            MouseDragCommand = function(self, params)
                local newx = params.MouseX + actuals.MenuWidth/2
                local newy = params.MouseY + actuals.MenuHeight/2 - actuals.MenuDraggerHeight/2
                self:GetParent():addx(newx):addy(newy)
            end,
        },
    }

    for i = 1, itemsPerPage do
        t[#t+1] = item(i)
    end

    t[#t+1] = Def.Quad {
        Name = "Cursor",
        InitCommand = function(self)
            self:halign(0)
            registerActorToColorConfigElement(self, "main", "SeparationDivider")
            self:diffusealpha(cursorAlpha)
        end,
    }
    return t
end

t[#t+1] = Def.ActorFrame {
    Name = "UIContainer",
    InitCommand = function(self)
        self:xy(SCREEN_WIDTH, SCREEN_CENTER_Y)
    end,
    makeUI(),
}

return t