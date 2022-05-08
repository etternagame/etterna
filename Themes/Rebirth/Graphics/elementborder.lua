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

            -- avoid shadowing self in the below nested functions, so store self in some variable
            local shelf = self
            self:GetParent():SetUpdateFunction(function(self)
                -- find the largest actor child of the assigned parent we are making a border for
                -- assign this border to match its size basically
                local bigw = 0
                local bigh = 0
                local eleh = nil
                pf:RunCommandsRecursively(
                    function(self)
                        if self:GetName() ~= shelf:GetName() then
                            local w = self:GetZoomedWidth()
                            local h = self:GetZoomedHeight()
                            if w > bigw then bigw = w eleh = self end
                            if h > bigh then bigh = h eleh = self end
                        end
                    end
                )
                shelf:halign(eleh:GetHAlign())
                shelf:valign(eleh:GetVAlign())
                shelf:x(eleh:GetX())
                shelf:y(eleh:GetY())
                shelf:zoomto(bigw, bigh)
            end)
            self:diffusealpha(borderAlpha)

            -- allow this to function as a button
            -- even with the comment this makes no sense. basically it has to do with layering stuff
            -- buttons on buttons and buttons in front or behind certain elements breaks stuff
            -- a higher z value is more "in front"
            self:z(10)

            -- place the quad behind the whole actorframe we are bordering
            self:draworder(-99)
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
            self:queuecommand("SetUpFinished")
        end,
        MouseOverCommand = function(self)
            self:alphaDeterminingFunction()
        end,
        MouseOutCommand = function(self)
            self:alphaDeterminingFunction()
        end,
        MouseDragCommand = function(self, params)
            if params.event == "DeviceButton_right mouse button" or not self.canDrag then return end

            -- due to unknown math reasons, rotated elements enter an unstable state which causes them
            -- to fly off the screen when their angle of rotation exceeds 60 degrees in either direction
            -- so im disabling dragging any rotated elements
            -- if you know how to fix this go ahead
            if self:GetTrueRotationZ() ~= 0 then return end


            local screenscale = MovableValues.ScreenZoom
            local pp = self:GetParent():GetParent()
            local ppp = pp:GetParent()
            local trueX = pp:GetTrueX() / screenscale
            local trueY = pp:GetTrueY() / screenscale
            local zoomfactor = 1

            -- this is almost always true but
            -- the primary reason this exists is to offset the Player related things properly
            -- normally it should be 0, but instead it is 640 for example
            -- hindsight comment: gonna be real with u chief this is a huge hack but it works
            -- if it stops working RESTRUCTURE YOUR ELEMENTS
            -- this only works because 'pp' is the ActorFrame that represents the gameplay element
            -- if ppp has been zoomed (breaking everything), then pp is a child of something that isn't a screen,
            -- such as the NoteField being pp and the Player being ppp
            if ppp ~= nil then
                trueX = trueX - (ppp:GetTrueX() / screenscale)
                trueY = trueY - (ppp:GetTrueY() / screenscale)
                zoomfactor = ppp:GetZoom()
            end

            local newx = params.MouseX + trueX - ((self.initialClickX or 0) / screenscale)
            local newy = params.MouseY + trueY - ((self.initialClickY or 0) / screenscale)
            newx = newx / zoomfactor
            newy = newy / zoomfactor
            local differenceX = newx - pp:GetX()
            local differenceY = newy - pp:GetY()

            -- massive gigantic hack
            -- not really interested in figuring out why this is necessary
            if getPlayerOptions():UsingReverse() and pp:GetName() == "NoteField" then
                differenceY = differenceY * -1
            end

            pp:x(newx):y(newy)
            setSelectedCustomizeGameplayElementActorPosition(differenceX, differenceY)
        end,
        MouseDownCommand = function(self, params)
            if params.event == "DeviceButton_right mouse button" then return end
            local pp = self:GetParent():GetParent()
            local screenscale = MovableValues.ScreenZoom

            self.initialClickX = params.MouseX * screenscale
            self.initialClickY = params.MouseY * screenscale

            local name = pp:GetName()
            self.canDrag = setSelectedCustomizeGameplayElementActorByName(name)
        end,
    }
}