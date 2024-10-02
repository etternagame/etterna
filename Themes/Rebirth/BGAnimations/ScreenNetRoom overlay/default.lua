local t = Def.ActorFrame {}

local margin = 12/1920 * SCREEN_WIDTH
local roomgap = 18/1080 * SCREEN_HEIGHT
local roomwidth = 450/1920 * SCREEN_WIDTH
local roomheight = 75/1080 * SCREEN_HEIGHT
local topframeh = 46

local scrnnamesize = 1
local descsize = 1
local createroomsize = 1
local noroomssize = 1
local buttonHoverAlpha = 0.6

local translations = {
    Name = THEME:GetString("NetRoom", "Name"),
    Explain = THEME:GetString("NetRoom", "Explain"),
    CreateRoom = THEME:GetString("NetRoom", "CreateRoom"),
    NoRooms = THEME:GetString("NetRoom", "NoRooms"),
}

local function makeroomlist()
    local selectedindex = 1
    local creatingroom = false
    local roomcount = 0

    local function validateconditions()
        if roomcount == 0 then
            creatingroom = true
        end
        MESSAGEMAN:Broadcast("UpdateRoomChoices")
    end

    local function moveindex(isdown)
        local amt = 1
        if not isdown then amt = -1 end
        
        local newind = selectedindex + amt
        if newind > roomcount then newind = 1 end
        if newind < 1 then newind = roomcount end
        selectedindex = newind
        validateconditions()
    end

    local t = Def.ActorFrame {
        Name = "RoomList",
        InitCommand = function(self)
            self:xy(SCREEN_CENTER_X, margin)
        end,
        OnCommand = function(self)
            SCREENMAN:set_input_redirected(PLAYER_1, true)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type == "InputEventType_FirstPress" or event.type == "InputEventType_Repeat" then

                    local gbtn = event.button
                    local up = gbtn == "MenuUp" or gbtn == "Up"
                    local down = gbtn == "MenuDown" or gbtn == "Down"
                    local left = gbtn == "MenuLeft" or gbtn == "Left"
                    local right = gbtn == "MenuRight" or gbtn == "Right"
                    local enter = gbtn == "Start"

                    if enter then
                        if creatingroom then
                            SCREENMAN:GetTopScreen():GetRoomWheel():MakeNewRoom()
                        else
                            self:GetChild("Room"..selectedindex):playcommand("MouseDown")
                        end
                        return true
                    end

                    if left or right then
                        creatingroom = not creatingroom
                        validateconditions()
                        return true
                    end

                    if up or down then
                        if creatingroom then
                            creatingroom = false
                            selectedindex = 1
                        else
                            moveindex(down)
                        end
                        validateconditions()
                        return true
                    end

                end
            end)
            self:playcommand("RoomListUpdated")
        end,
        UpdateRoomChoicesMessageCommand = function(self)
            for i=1, roomcount do
                local ac = self:GetChild("Room"..i)
                local a = ac:GetDiffuseAlpha()

                if creatingroom then
                    ac:diffuse(color("1,1,1"))
                    ac:diffusealpha(a)
                else
                    if i == selectedindex then
                        ac:diffuse(color("0.5,0.5,0.5"))
                        ac:diffusealpha(a)
                    else
                        ac:diffuse(color("1,1,1"))
                        ac:diffusealpha(a)
                    end
                end
            end

            local bt = self:GetChild("CreateButton")
            local a = bt:GetDiffuseAlpha()
            if creatingroom then
                bt:diffuse(color("0.5,0.5,0.5"))
                bt:diffusealpha(a)
            else
                bt:diffuse(color("1,1,1"))
                bt:diffusealpha(a)
            end
        end,
        RoomListUpdatedMessageCommand = function(self)
            local tscn = SCREENMAN:GetTopScreen()
            if tscn == nil then return end
            tscn:InfoSetVisible(false)
            local whe = tscn:GetRoomWheel()
            local rms = whe:GetRooms()

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
                newroom:addy(topframeh)
                ind = ind + 1
            end
            roomcount = #rms

            self:GetChild("NoEntriesMessage"):visible(#rms == 0)
            validateconditions()
        end,
        LoadFont("Common Normal") .. {
            Name = "NoEntriesMessage",
            InitCommand = function(self)
                self:settext(translations["NoRooms"])
                self:zoom(noroomssize)
                self:valign(0)
                self:y(SCREEN_CENTER_Y/2)
                self:visible(false)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
        },

        Def.ActorFrame {
            Name = "CreateButton",
            InitCommand = function(self)
                self:xy(-SCREEN_CENTER_X + roomwidth/2 + margin, SCREEN_CENTER_Y-margin)
            end,

            UIElements.QuadButton(1, 1) .. {
                Name = "BG",
                InitCommand = function(self)
                    self:zoomto(roomwidth, roomheight)
                    registerActorToColorConfigElement(self, "main", "PrimaryBackground")
                end,
                MouseDownCommand = function(self)
                    local tscn = SCREENMAN:GetTopScreen()
                    tscn:GetRoomWheel():MakeNewRoom()
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:GetParent():diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:GetParent():diffusealpha(1)
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "Text",
                InitCommand = function(self)
                    self:zoom(createroomsize)
                    self:maxwidth(roomwidth / createroomsize)
                    self:settext(translations["CreateRoom"])
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
            }
        }
    }
    return t
end

t[#t+1] = Def.ActorFrame {
    InitCommand = function(self)
        self:xy(margin, margin)
    end,

    Def.Quad {
        InitCommand = function(self)
            self:xy(-margin, -margin)
            self:valign(0):halign(0)
            self:zoomto(SCREEN_WIDTH, topframeh)
            registerActorToColorConfigElement(self, "main", "SecondaryBackground")
        end,
    },
    LoadFont("Common Large") .. {
        Name = "ScreeName",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:settext(translations["Name"])
            self:zoom(scrnnamesize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
    },
}

t[#t+1] = Def.ActorFrame {
    InitCommand = function(self)
        self:xy(margin, SCREEN_HEIGHT - margin)
    end,

    Def.Quad {
        InitCommand = function(self)
            self:xy(-margin, margin)
            self:valign(1):halign(0)
            self:zoomto(SCREEN_WIDTH, 32)
            registerActorToColorConfigElement(self, "main", "SecondaryBackground")
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "ScreeDesc",
        InitCommand = function(self)
            self:valign(1):halign(0)
            self:settext(translations["Explain"])
            self:zoom(descsize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
    },
}

t[#t+1] = makeroomlist()

t[#t+1] = LoadActor("../_mouse.lua")

return t