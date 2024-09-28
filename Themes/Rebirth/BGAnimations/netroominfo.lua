local namesize = 1
local descsize = 0.75
local passsize = 0.5

local namey = 0
local descy = 25
local passy = 50

local translations = {
    Passworded = THEME:GetString("NetRoom", "Passworded"),
}

local t = Def.ActorFrame {
    Name = "Room",
    SetPosCommand = function(self, params)
        local i = params.index
        local yp = i * (params.h + params.gap)
        local xp = 0
        self:xy(xp, yp)
    end,
    UpdateRoomCommand = function(self, params)
        self.roominfo = params
    end,

    UIElements.QuadButton(1, 1) .. {
        Name = "BG",
        InitCommand = function(self)
            registerActorToColorConfigElement(self, "main", "PrimaryBackground")
        end,
        SetPosCommand = function(self, params)
            self:zoomto(params.w + params.gap/2, params.h + params.gap/2)
        end,
        MouseDownCommand = function(self)
            local tscn = SCREENMAN:GetTopScreen()
            if tscn == nil then return end
            local whe = tscn:GetRoomWheel()
            local ri = self:GetParent().roominfo
            whe:StopSearch()
            whe:Move(1)
            whe:Search(ri.name, ri.description, true, ri.passworded, true)
            tscn:SelectCurrent()
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Name",
        InitCommand = function(self)
            self:zoom(namesize)
        end,
        SetPosCommand = function(self, params)
            self:y(-params.h / 3)
            self:maxwidth(params.w / namesize)
        end,
        UpdateRoomCommand = function(self, params)
            self:settext(params.name)
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Desc",
        InitCommand = function(self)
            self:zoom(descsize)
        end,
        SetPosCommand = function(self, params)
            self:y(0)
            self:maxwidth(params.w / namesize)
        end,
        UpdateRoomCommand = function(self, params)
            if params.description ~= nil then
                self:visible(true)
                self:settext(params.description)
            else
                self:visible(false)
            end
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Passworded",
        InitCommand = function(self)
            self:zoom(passsize)
            self:settext(translations["Passworded"])
            self:visible(false)
        end,
        SetPosCommand = function(self, params)
            self:y(params.h / 3)
            self:maxwidth(params.w / namesize)
        end,
        UpdateRoomCommand = function(self, params)
            if params.passworded then
                self:visible(true)
            else
                self:visible(false)
            end
        end,
    }
}

return t