-- border used for customize gameplay primarily but feel free to attempt to use it for something else and have it not work

local borderAlpha = 0.2
local buttonHoverAlpha = 0.6

return Def.ActorFrame {
    Name = "BorderContainer", -- not really necessary to have this in an actorframe unless the border is more complex
    UIElements.QuadButton(1) .. {
        Name = "Border",
        OnCommand = function(self)
            self:queuecommand("SetUp")
        end,
        SetUpCommand = function(self)
            local pf = self:GetParent():GetParent()
            ms.ok(pf:GetName())

            -- avoid shadowing self in the below nested functions, so store self in some variable
            local poop = self
            self:GetParent():SetUpdateFunction(function(self)
                -- find the largest actor child of the assigned parent we are making a border for
                -- assign this border to match its size basically
                local bigw = 0
                local bigh = 0
                local eleh = nil
                pf:RunCommandsRecursively(
                    function(self)
                        local w = self:GetZoomedWidth()
                        local h = self:GetZoomedHeight()
                        if w > bigw then bigw = w eleh = self end
                        if h > bigh then bigh = h eleh = self end
                    end
                )
                poop:halign(eleh:GetHAlign())
                poop:valign(eleh:GetVAlign())
                poop:x(eleh:GetX())
                poop:y(eleh:GetY())
                poop:zoomto(bigw, bigh)
                poop:visible(true)
            end)
            self:diffusealpha(borderAlpha)

            -- allow this to function as a button
            self:z(5)

            -- place the quad behind the whole actorframe we are bordering
            self:draworder(-9999)
            pf:SortByDrawOrder()

            self.alphaDeterminingFunction = function(self)
                if isOver(self) then
                    pf:diffusealpha(buttonHoverAlpha)
                    self:diffusealpha(borderAlpha * buttonHoverAlpha)
                else
                    pf:diffusealpha(1)
                    self:diffusealpha(borderAlpha)
                end
            end
        end,
        MouseOverCommand = function(self)
            self:alphaDeterminingFunction()
        end,
        MouseOutCommand = function(self)
            self:alphaDeterminingFunction()
        end,
        MouseDragCommand = function(self, params)
            local newx = params.MouseX
            local newy = params.MouseY
            self:GetParent():GetParent():addx(newx):addy(newy)
        end,
        MouseDownCommand = function(self, params)

        end,
    }
}