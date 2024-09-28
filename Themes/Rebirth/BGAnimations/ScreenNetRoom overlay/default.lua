local t = Def.ActorFrame {}

local margin = 12/1920 * SCREEN_WIDTH
local roomgap = 18/1080 * SCREEN_HEIGHT
local roomwidth = 450/1920 * SCREEN_WIDTH
local roomheight = 75/1080 * SCREEN_HEIGHT

local scrnnamesize = 1
local descsize = 1

local translations = {
    Name = THEME:GetString("NetRoom", "Name"),
    Explain = THEME:GetString("NetRoom", "Explain"),
}

local function makeroomlist()
    local t = Def.ActorFrame {
        Name = "RoomList",
        InitCommand = function(self)
            self:xy(SCREEN_CENTER_X, margin)
            self.roomactors = {}
        end,
        OnCommand = function(self)
            self:playcommand("RoomListUpdated")
        end,
        RoomListUpdatedMessageCommand = function(self)
            local tscn = SCREENMAN:GetTopScreen()
            if tscn == nil then return end
            tscn:InfoSetVisible(false)
            local whe = tscn:GetRoomWheel()
            local rms = whe:GetRooms()
            ms.ok(rms)

            local childs = self:GetChildren()
            for n,v in pairs(childs) do
                if v:GetName():find("Room") ~= nil then
                    self:RemoveChild(v:GetName())
                end
            end
            local ind = 1
            for i,v in ipairs(rms) do
                local newroom = self:AddChildFromPath(THEME:GetPathB("", "netroominfo"))
                newroom:name("Room"..ind)
                newroom:playcommand("SetPos", {index=ind, w=roomwidth, h=roomheight, gap=roomgap})
                newroom:playcommand("UpdateRoom", v)
                ind = ind + 1
            end
            self:GetChild("BG"):zoomy(#rms * (roomheight + margin * 2))
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:valign(0):halign(0)
                registerActorToColorConfigElement(self, "main", "PrimaryBackground")
            end,
        }
    }
    return t
end

t[#t+1] = LoadFont("Common Large") .. {
    Name = "ScreeName",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:settext(translations["Name"])
        self:xy(margin, margin)
        self:zoom(scrnnamesize)
    end,
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "ScreeDesc",
    InitCommand = function(self)
        self:valign(1):halign(0)
        self:settext(translations["Explain"])
        self:xy(margin, SCREEN_HEIGHT - margin)
        self:zoom(descsize)
    end,
}

t[#t+1] = makeroomlist()

t[#t+1] = LoadActor("../_mouse.lua")

return t