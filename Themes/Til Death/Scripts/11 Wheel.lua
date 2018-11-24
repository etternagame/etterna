local Wheel = {}

Wheel.mt = {
  move = function(whee, num)
    rebuildFrames(whee, whee.index + num)
    --TODO: Animations?
  end,
  rebuildFrames = function(whee, newIndex)
    whee.items = whee.itemsGetter()
    whee.index = newIndex or whee.startIndex
    for k, v in pairs(whee.items) do
      -- TODO: Handle circular buffering logic?
      whee.itemUpdater(v, whee.items[k])
    end
  end
}

Wheel.defaultParams = {
  itemsGetter = function()
    -- Should return an array table of elements for the wheel
    -- This is a function so it can be delayed, and rebuilt
    --  with different items using this function
    return SONGMAN:GetAllSongs()
  end,
  count = 20,
  itemBuilder = function()
    -- Should return an actor def
    --TODO
  end,
  frameUpdater = function(frame, item) -- Update an frame created with frameBuilder with an item
    --TODO
  end,
  x = 0,
  y = 0,
  highlightBuilder = function()
  end,
  buildOnInit = true, -- Build wheel in InitCommand (Will be empty until rebuilt otherwise)
  frameTransformer = function(frame, offsetFromCenter, index, total) -- Handle frame positioning
    --TODO
  end,
  startIndex = 1
}
function Wheel:new(params)
  local whee = Def.ActorFrame {}
  setmetatable(whee, Wheel.mt)
  whee.itemsGetter = params.itemsGetter
  whee.count = params.count
  whee.startIndex = params.startIndex
  whee.frameUpdater = params.frameUpdater
  whee.buildOnInit = params.buildOnInit
  whee.frameTransformer = params.frameTransformer
  whee.index = whee.startIndex
  whee.x = params.x
  whee.y = params.y
  whee.OnCommand = function()
    whee:x(whee.x):y(whee.y)
    if params.buildOnInit then
      whee:rebuildFrames()
    end
  end
  whee.frames = {}
  for i = 1, (params.count) do
    local frame =
      params.frameBuilder() ..
      {
        InitCommand = function(self)
          whee.frames[i] = self
          local offset = math.floor(i - whee.count / 2 + 0.5)
          whee.frameTransformer(self, offset, i, whee.count)
        end
      }
    whee[#whee + 1] = frame
  end
  whee[#whee + 1] =
    params.highlightBuilder() ..
    {
      InitCommand = function(self)
        whee.highlight = self
      end
    }
  return whee
end

local function LegacyParams()
  local params = {}
  local function SelectMusicWheelMetric(key)
    return GetMetric("ScreenSelectMusic", "MusicWheel" .. key)
  end
  params.x = SelectMusicWheelMetric("X")
  params.y = SelectMusicWheelMetric("Y")
  local function MusicWheelMetric(key)
    return GetMetric("MusicWheel", key)
  end
  params.frameTransformer = MusicWheelMetric("ItemTransformFunction")
  params.count = MusicWheelMetric("NumWheelItems")
  local function MusicWheelItemMetric(key)
    return GetMetric("MusicWheelItem", key)
  end
  local songNameX = MusicWheelItemMetric("SongNameX")
  local songNameY = MusicWheelItemMetric("SongNameY")
  local function wheelItemPartXY(type)
    return {
      x = MusicWheelItemMetric(type .. "X"),
      y = MusicWheelItemMetric(type .. "Y")
    }
  end
  local function wheelItemPart(type)
    local part = wheelItemPartXY(type)
    part.on = MusicWheelItemMetric(type .. "OnCommand")
    return part
  end
  local wheelItemParts = {
    "SectionExpanded",
    "SectionCollapsed",
    "SectionCount",
    "Sort",
    "WheelNotifyIcon",
    "Roulette",
    "SongName",
    "Portal",
    "Random",
    "Custom",
    "Mode"
  }
  params.parts = {}
  for i = 1, #wheelItemParts do
    local part = wheelItemParts[i]
    params.parts[part] = wheelItemPart(part)
  end
  local wheelItemTypes = {
    "Course",
    "Custom",
    "Mode",
    "Portal",
    "Random",
    "Roulette",
    "SectionExpanded",
    "SectionCollaped",
    "Song",
    "Sort"
  }
  local function loadGraphicFile(filename)
    --TODO
    return loadfile(THEME:GetPathG(filename, ""))
  end
  local function loadMusicWheelItemPartLegacyActor(name)
    return loadGraphicFile("MusicWheelItem " .. type)
  end
  local function loadMusicWheelItemTypeLegacyActor(type)
    local parts = {
      "Normal",
      "Color",
      "Over"
    }
    local item = Def.ActorFrame {}
    for i = 1, #parts do
      local part = parts[i]
      local actor = loadMusicWheelItemPartLegacyActor(type .. " " .. part .. "Part")
      item[#item + 1] = actor
    end
    return item
  end
  local function constF(x)
    return function()
      return x
    end
  end
  params.actorBuilders = {}
  for i = 1, #wheelItemTypes do
    local type = wheelItemTypes[i]
    params.actorBuilders[type] = constF(loadMusicWheelItemTypeLegacyActor(type))
  end
  params.actorBuilders.grades = constF(loadMusicWheelItemPartLegacyActor("grades"))
  params.actorBuilders.highlight = constF(loadGraphicFile("highlight"))
end

local function example()
  -- Pseudo code for a possible way of doing groups
  -- Using polymorphism of items so both strings and songs are in there
  local function allPacks()
    --TODO
  end
  local function songActor()
    --TODO
  end
  local function groupActor()
    --TODO
  end
  local function split(table, element)
    --TODO
  end
  local function concat(t1, t2, t3, t4, tn)
    --TODO
  end
  local function getAllSongsFromPack(packname)
    --TODO
  end
  local w
  w =
    Wheel:new {
    contentGetter = allPacks,
    frameBuilder = function()
      local x
      x =
        Def.ActorFrame {
        groupActor {
          InitCommand = function(self)
            x.g = self
          end
        },
        songActor {
          InitCommand = function(self)
            x.s = self
          end
        }
      }
      return x
    end,
    frameUpdater = function(frame, songOrPack)
      if songOrPack.GetAllSteps then -- song
        -- Update songActor and make group actor invis
        local song = frame.s
        local group = frame.g
        group:visible(false)
        song:visible(true)
      else -- pack
        --update group actor and make song actor invis
        local song = frame.s
        local group = frame.g
        group:visible(true)
        song:visible(false)
      end
    end,
    onSelection = function(frame, songOrPack)
      if songOrPack.GetAllSteps then -- song
        -- TODO: Start song
      else -- pack
        local group = songOrPack
        local groups = allPacks()
        local g1, g2 = split(groups, group)
        w.contentGetter = function()
          return concat(g1, getAllSongsFromPack(group), g2)
        end
        w:rebuildFrames()
      end
    end
  }
end

function Wheel:LegacyWheel()
  return Wheel.new(LegacyParams)
end
