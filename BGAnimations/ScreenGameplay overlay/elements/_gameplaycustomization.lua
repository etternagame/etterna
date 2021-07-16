-- gameplay customization
-- assume that customization is active if this file is loaded



local snm = Var("LoadingScreen")


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

        Movable.pressed = false
        Movable.current = "None"
        Movable.DeviceButton_r.element = nf
        Movable.DeviceButton_t.element = noteColumns
        Movable.DeviceButton_r.condition = true
        Movable.DeviceButton_t.condition = true
        self:GetChild("LifeP1"):GetChild("Border"):SetFakeParent(lifebar)
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
    }

}




return t