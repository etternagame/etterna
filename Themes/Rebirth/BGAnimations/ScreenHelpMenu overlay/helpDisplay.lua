
local ratios = {
    InfoTopGap = 25 / 1080, -- top edge screen to top edge box
    InfoLeftGap = 60 / 1920, -- left edge screen to left edge box
    InfoWidth = 1799 / 1920, -- small box width
    InfoHeight = 198 / 1080, -- small box height
    InfoHorizontalBuffer = 40 / 1920, -- from side of box to side of text
    InfoVerticalBuffer = 28 / 1080, -- from top/bottom edge of box to top/bottom edge of text

    MainDisplayTopGap = 250 / 1080, -- top edge screen to top edge box
    MainDisplayLeftGap = 60 / 1920, -- left edge screen to left edge box
    MainDisplayWidth = 1799 / 1920, -- big box width
    MainDisplayHeight = 800 / 1080, -- big box height

    ScrollerWidth = 15 / 1920, -- width of the scroll bar and its area (height dependent on items)
    ListWidth = 405 / 1920, -- from right edge of scroll bar to left edge of separation gap
    SeparationGapWidth = 82 / 1920, -- width of the separation area between selection list and the info area

    TopBuffer = 45 / 1080, -- buffer from the top of any section to any item within the section
    TopBuffer2 = 119 / 1080, -- from top edge of section to the subtitle text
    TopBuffer3 = 200 / 1080, -- from top edge of section to the description text
    EdgeBuffer = 25 / 1920, -- buffer from the edge of any section to any item within the section

    IconExitWidth = 47 / 1920,
    IconExitHeight = 36 / 1080,
}

local actuals = {
    InfoTopGap = ratios.InfoTopGap * SCREEN_HEIGHT,
    InfoLeftGap = ratios.InfoLeftGap * SCREEN_WIDTH,
    InfoWidth = ratios.InfoWidth * SCREEN_WIDTH,
    InfoHeight = ratios.InfoHeight * SCREEN_HEIGHT,
    InfoHorizontalBuffer = ratios.InfoHorizontalBuffer * SCREEN_WIDTH,
    InfoVerticalBuffer = ratios.InfoVerticalBuffer * SCREEN_HEIGHT,
    MainDisplayTopGap = ratios.MainDisplayTopGap * SCREEN_HEIGHT,
    MainDisplayLeftGap = ratios.MainDisplayLeftGap * SCREEN_WIDTH,
    MainDisplayWidth = ratios.MainDisplayWidth * SCREEN_WIDTH,
    MainDisplayHeight = ratios.MainDisplayHeight * SCREEN_HEIGHT,
    ScrollerWidth = ratios.ScrollerWidth * SCREEN_WIDTH,
    ListWidth = ratios.ListWidth * SCREEN_WIDTH,
    SeparationGapWidth = ratios.SeparationGapWidth * SCREEN_WIDTH,
    TopBuffer = ratios.TopBuffer * SCREEN_HEIGHT,
    TopBuffer2 = ratios.TopBuffer2 * SCREEN_HEIGHT,
    TopBuffer3 = ratios.TopBuffer3 * SCREEN_HEIGHT,
    EdgeBuffer = ratios.EdgeBuffer * SCREEN_WIDTH,
    IconExitWidth = ratios.IconExitWidth * SCREEN_WIDTH,
    IconExitHeight = ratios.IconExitHeight * SCREEN_HEIGHT,
}

local infoTextSize = 0.65
local listTextSize = 0.4
local titleTextSize = 0.95
local subtitleTextSize = 0.55
local descTextSize = 0.4

local textZoomFudge = 5
local buttonHoverAlpha = 0.3
local cursorAlpha = 0.3
local cursorAnimationSeconds = 0.1
local animationSeconds = 0.1

-- special handling to make sure our beautiful icon doesnt get tarnished
local logosourceHeight = 133
local logosourceWidth = 102
local logoratio = math.min(1920 / SCREEN_WIDTH, 1080 / SCREEN_HEIGHT)
local logoH, logoW = getHWKeepAspectRatio(logosourceHeight, logosourceWidth, logosourceWidth / logosourceWidth)

local function helpMenu()

    -- describe each category
    -- this appears in the scroll list and large above the main screen
    -- the point of this table is solely to maintain the order of option categories that show up
    local categoryDefs = {
        "Common Terminology",
    }

    -- describe each option in each category
    -- each option within each category shows up in exactly that order. the categories do not use this order (refer to the above table instead)
    -- the definition is:
    --[[
    ["CategoryDef Entry"] = {
        [1] = {
            Name = "OptionName", -- this appears in the scroll list and large in the main area
            ShortDescription = "1 sentence", -- this appears as a subtext to the large text in the main area
            Description = "a paragraph", -- this appears as regular text in the remaining area
            Image = "path to an image", -- OPTIONAL -- if supplied, this takes up the right half of the main area
        },
        [2] = {}, ....
    }
    ]]
    local optionDefs = {
        ["Common Terminology"] = {
            {
                Name = "Roll (1234)",
                ShortDescription = "Vaguely a jumptrill",
                Description = "Roll is the common name given to this type of pattern which requires you to press all columns in succession before repeating any columns again. It comes in several forms.\n\nThe specific roll depicted here may be referred to as an ascending roll. If it went in the opposite direction (4321) it would be descending.\n\nThe direction of the roll does not affect its capacity to be jumptrilled. Only the speed of the roll does.",
                Image = THEME:GetPathG("", "Patterns/1234 roll"),
            },
            {
                Name = "Roll (1243)",
                ShortDescription = "Vaguely a jumptrill",
                Description = "Roll is the common name given to this type of pattern which requires you to press all columns in succession before repeating any columns again. It comes in several forms.\n\nThe specific roll depicted here may be referred to as a split roll. If the pattern began with the opposite hand (4312) it would be functionally equivalent.\n\nRegardless, this roll variant can be effectively jumptrilled if its speed is sufficient.",
                Image = THEME:GetPathG("", "Patterns/1243 roll"),
            },
            {
                Name = "Roll (1423)",
                ShortDescription = "Vaguely a split jumptrill",
                Description = "Roll is the common name given to this type of pattern which requires you to press all columns in succession before repeating any columns again. It comes in several forms.\n\nThe specific roll depicted here may be referred to as a split roll, split hand roll, or split trill. It may also begin in reverse order (3241, inside out) but still remains functionally equivalent.\n\nThis roll variant is more difficult to jumptrill than others. When hit quickly, it is physically similar to a split jumptrill.",
                Image = THEME:GetPathG("", "Patterns/1423 roll"),
            },
            {
                Name = "Gluts",
                ShortDescription = "Another word for 'in excess'",
                Description = "Gluts is a broad term which captures jumpgluts and handgluts. It's frequently debated what defines a glut or even if a handglut exists.\n\nThe most accepted definition of a glut is literal: jumpgluts are many continuous jumps (jumpjacks) which typically form minijacks as they change column pairs over the course of the glut run. Depicted in the image is a run of jumpjacks referred to as gluts because of the minijacks on column 1 and 4.\n\nGluts are a subset of chordjacks with more focus on jack speed than chords.",
                Image = THEME:GetPathG("", "Patterns/gluts"),
            },
            {
                Name = "Chordjack",
                ShortDescription = "Chords which form jacks",
                Description = "Chordjack is the blanket term for patterns made up entirely of n-chords which form jacks.\n\nThe chords contained in the overall pattern do not have to be the same. Jumps, hands, or quads are valid. Depicted to the right is a very generic medium density chordjack pattern containing only jumps and hands.",
                Image = THEME:GetPathG("", "Patterns/chordjacks"),
            },
            {
                Name = "Dense Chordjack",
                ShortDescription = "Don't break your keyboard",
                Description = "Dense chordjacks are a specialization of chordjacks which are biased toward hands and quads. At high enough density, this may be referred to as holedodge because when reading, there are more notes than empty spaces.\n\nDense chordjacks require a lot of stamina and tend to have embedded longjacks due to the pattern being almost entirely hands and quads.",
                Image = THEME:GetPathG("", "Patterns/dense chordjack"),
            },
            {
                Name = "Stream",
                ShortDescription = "Continuous single taps",
                Description = "Just like the name implies, a stream is a continuous stream of notes. More specifically, these continuous notes are mostly on separate columns, not forming jacks. There can be any kind of variation to the patterning as long as it doesn't deviate too much from the pure definition.\n\nMinijacks, chords, or other patterns that can be embedded may be found within a stream, but only serve to make it more difficult unless they dominate the overall pattern.\n\nTo the right is a stream which is slightly rolly.",
                Image = THEME:GetPathG("", "Patterns/streams"),
            },
            {
                Name = "Jumpstream",
                ShortDescription = "Stream with jumps",
                Description = "Jumpstream expands the definition of a stream by requiring jumps at a certain frequency within the pattern. Other than that, it is still a stream.\n\nDepicted to the right is a simple jumpstream pattern with an anchor on column 1. A more observant player may also recognize that this pattern in isolation can be jumptrilled.",
                Image = THEME:GetPathG("", "Patterns/jumpstream"),
            },
            {
                Name = "Handstream",
                ShortDescription = "Stream with hands",
                Description = "Handstream expands the definition of a stream by requiring hands at a certain frequency within the pattern. It is also not uncommon to find jumps embedded within a handstream. This may be referred to as dense handstream, although the most dense handstream is purely hands and single taps.\n\nAnchors are more common in a handstream since the charter only has 4 ways to fit a hand into 4 columns. In the depiction to the right, there is an anchor on column 4 and, depending who you ask, also column 3.",
                Image = THEME:GetPathG("", "Patterns/handstream"),
            },
            {
                Name = "Quadstream",
                ShortDescription = "Stream with quads",
                Description = "Quadstream takes the definition of stream so far that it begins to look like chordjacks.\n\nIt forms a minijack with a quad using a single tap (usually) and still flows like a stream as opposed to pure chordjacks.\n\nIncreasing the density of quads will lose this characteristic due to the limited column space.",
                Image = THEME:GetPathG("", "Patterns/quadstream"),
            },
            {
                Name = "Trill",
                ShortDescription = "Like musical theory",
                Description = "A trill is a sequence of two continuously alternating notes. In the context of this game, they can be either on one hand or split between both hands.\n\nWhen a trill is one handed, it is called a one hand trill. Otherwise, it is a two hand trill. Trills are not restricted to two adjacent columns, and can be on columns 1 and 3 for example.\n\nDepicted to the right is a simple two hand trill.",
                Image = THEME:GetPathG("", "Patterns/trill"),
            },
            {
                Name = "Jumptrill",
                ShortDescription = "Can be played blind",
                Description = "Jumptrill is a very simple expansion of a two hand trill. Jump on one hand and then jump on the other. Most often this pattern makes up the highest NPS section of a chart unless it is dense chordjacks.\n\nOne special thing about the ordinary jumptrill is that many other patterns can be broken down into a jumptrill, which allows cheese-oriented gameplay. We don't recommend doing this too frequently for the sake of your scores and habit forming.",
                Image = THEME:GetPathG("", "Patterns/jumptrill"),
            },
            {
                Name = "Split Jumptrill [13][24]",
                ShortDescription = "Jumptrill but dangerous",
                Description = "Split jumptrill is a shuffled jumptrill. It forms two one hand trills instead of one pure jumptrill.\n\nSplit jumptrills are very annoying due to many players' inability to hit them fluently. This specific variant of split jumptrills can be more difficult than the other variant of split jumptrills because a player tends to be more inclined to jumptrill or roll when simultaneously doing a same-direction rolling motion with both hands. Luckily they can be jumptrilled if hit just right.\n\nTo the right is a long split jumptrill.",
                Image = THEME:GetPathG("", "Patterns/13 split jt"),
            },
            {
                Name = "Split Jumptrill [14][23]",
                ShortDescription = "Jumptrill but dangerous",
                Description = "Split jumptrill is a shuffled jumptrill. It forms two one hand trills instead of one pure jumptrill.\n\nSplit jumptrills are very annoying due to many players' inability to hit them fluently. Compared to the other variant of split jumptrills, this one is usually easier because it can feel more natural. Anyways, they can be jumptrilled if hit just right.\n\nTo the right is a long split jumptrill.",
                Image = THEME:GetPathG("", "Patterns/14 split jt"),
            },
            {
                Name = "Minijacks",
                ShortDescription = "Instant combo breaker",
                Description = "Minijacks are pairs of jacks. They can be continuous like the image to the right or embedded in some other pattern like a stream.\nMinijacks can be difficult to hit accurately as they get closer together because of the nature of the hit window. Imagine hitting one note within 180ms. Now hit two notes in that same window. If the minijack is fast enough or the player is slow enough, it's guaranteed points lost.\nMinijacks can be embedded within broader jack oriented patterns as a jack burst. Most commonly they are in isolation or in stream transitions that jack instead of trill.",
                Image = THEME:GetPathG("", "Patterns/minijacks"),
            },
            {
                Name = "Longjack",
                ShortDescription = "Continuous taps",
                Description = "A jack, or jackhammer, is a set of continuous taps in the same column. A longjack is the same thing, but a little longer than usual.\n\nThe length of a jack that is considered a longjack is debated, but generally it is around 5-6 notes. The longjack can continue into infinity.\n\nLongjacks are the base pattern which make up continuous jumpjacks, handjacks, or quadjacks. This base pattern also makes up much of the structure for files oriented towards the vibro playstyle.",
                Image = THEME:GetPathG("", "Patterns/longjack"),
            },
            {
                Name = "Anchor",
                ShortDescription = "Basically embedded longjacks",
                Description = "An anchor is a common continuous set of columns being utilized relative to the other columns contained in a pattern. Most commonly, anchors are only one column at a time.\n\nAn anchor may be on a snap alternating from the rest of the pattern or not, or a bit of both. Anchors that last a while break down to be longjacks, which will cause an overall pattern to be more stamina draining.\n\nThe image to the right depicts an anchor on column 4.",
                Image = THEME:GetPathG("", "Patterns/anchor"),
            },
            {
                Name = "Minedodge",
                ShortDescription = "Spicy notes",
                Description = "Minedodge is the term for a type of chart or general pattern which contains notes that are intentionally placed near mines to increase difficulty.\n\nMinedodge does not actually change the MSD of a chart, because the calculator measures physical difficulty of the taps.\n\nThe difficulty of minedodge comes from the necessity of more precisely timing the press and release of notes and increased difficulty to read the notes depending on the noteskin used.",
                Image = THEME:GetPathG("", "Patterns/minedodge"),
            },
            {
                Name = "Hold",
                ShortDescription = "Don't let go",
                Description = "Holds are a note type which require the player to hold the button for the entire duration of the note. They may also be referred to as freezes or long notes.\n\nPatterns made up of more holds are sometimes referred to as a holdstream or, at the extreme end of the spectrum, full inverse (all empty space is a hold).\n\nIn this game, a hold can be released for a short period of time dependent on the judge difficulty. On judge 4, the time is 250ms. Holds can then be regrabbed. Holds do not have release timing, but release timing can be emulated with a lift or mine.",
                Image = THEME:GetPathG("", "Patterns/hold"),
            },
            {
                Name = "Roll / Rolld",
                ShortDescription = "Keep tapping",
                Description = "Roll note types, not to be confused with the roll pattern, are a hold type which require the player to continuously tap the button for the duration of the note. They may also be referred to as a rolld.\n\nRolls cannot be held and must be continuously tapped. The speed of the tap has a threshold the player receives no judgment for, but it gets smaller at higher judge difficulties. On judge 4, the player has up to 500ms between taps.",
                Image = THEME:GetPathG("", "Patterns/rolld"),
            },
            {
                Name = "Burst",
                ShortDescription = "Explosive speed",
                Description = "A burst is a pattern specialization which means exactly what it says. Relative to its pattern context, a burst is much quicker.\n\nBursts can come in any form which matches that definition, not only the scenario depicted to the right. Jacks and jumpstream can also burst. The point is that it is a quicker collection of notes, almost like a compressed pattern.\n\nOften, a burst is patterned in such a way that it isn't difficult to full combo. But that isn't always the case!",
                Image = THEME:GetPathG("", "Patterns/burst"),
            },
            {
                Name = "Polyrhythm",
                ShortDescription = "Brain melting patterns",
                Description = "Polyrhythms are a pattern specialization which indicates that multiple rhythms are being charted simultaneously, leading to alternating snaps being utilized. Sometimes the result of this is a very awkward, technically difficult to execute pattern.\n\nTo the right is a depiction of a simpler polyrhythm of 16ths and 12ths.",
                Image = THEME:GetPathG("", "Patterns/polyrhythms"),
            },
            {
                Name = "Graces",
                ShortDescription = "A little extra flare",
                Description = "Grace notes are slightly offset notes, exceptionally rarely forming minijacks, which represent a kind of grace or extra flare to an initial note.\n\nIn musical theory, these are defined as not so necessary, but typically when these are charted it means that a grace note was present in the music.\n\nFlams are made up of graces. Within this game, both usually mean the same. Graces usually break down to be a single chord, and can rarely contain chords themselves.",
                Image = THEME:GetPathG("", "Patterns/graces"),
            },
            {
                Name = "Runningman",
                ShortDescription = "Anchored stream",
                Description = "Classically, runningman is a term referring to a stream that is anchored. We expand on that definition by allowing chords to be mixed in very lightly. An anchored jumpstream can technically contain a runningman, but is more likely to just be referred to as anchored jumpstream.\n\nIt is required that the anchor in a runningman be offset from the rest of the pattern so that it doesn't form chords with the rest of the pattern.\n\nTo the right is a runningman anchored on column 1. The off-taps may be on any column, but it is important that not too many taps be on the same hand as the anchor.",
                Image = THEME:GetPathG("", "Patterns/runningman"),
            },
        }
    }

    -- generated table
    -- basically the data representation of the scroller thing
    -- categories are top level items
    -- options are slightly shifted over
    -- pagination and indexing is based on this table
    local items = {}
    for _, cat in ipairs(categoryDefs) do
        items[#items+1] = {
            isCategory = true,
            Name = cat,
        }
        for __, optionDef in ipairs(optionDefs[cat]) do
            items[#items+1] = {
                isCategory = false,
                Parent = cat,
                Name = optionDef.Name,
                Def = optionDef,
            }
        end
    end

    local itemsVisible = 20
    local cursorIndex = 1
    local page = 1
    local maxPage = math.ceil(#items / itemsVisible)
    local function getPageFromIndex(i)
        return math.ceil((i) / itemsVisible)
    end
    local function cursorHoversItem(i)
        if getPageFromIndex(cursorIndex) ~= page then return false end
        return ((cursorIndex-1) % itemsVisible == (i-1))
    end
    local function movePage(n)
        local newpage = page + n
        if newpage > maxPage then newpage = 1 end
        if newpage < 1 then newpage = maxPage end
        page = newpage
        MESSAGEMAN:Broadcast("UpdatePage")
    end
    local function moveCursor(n)
        local newpos = cursorIndex + n
        if newpos > #items then newpos = 1 end
        if newpos < 1 then newpos = #items end
        cursorIndex = newpos
        local newpage = getPageFromIndex(cursorIndex)
        if newpage ~= page then
            page = newpage
            MESSAGEMAN:Broadcast("UpdatePage")
        end
        MESSAGEMAN:Broadcast("UpdateCursor")
    end

    local function menuItem(i)
        local yIncrement = (actuals.MainDisplayHeight) / itemsVisible
        local index = i
        local item = items[index]
        return Def.ActorFrame {
            Name = "MenuItem_"..i,
            InitCommand = function(self)
                self:x(actuals.ScrollerWidth + actuals.EdgeBuffer/2)
                -- center y
                self:y(yIncrement * (i-1) + yIncrement / 2)
                self:playcommand("UpdateItem")
            end,
            SelectCurrentCommand = function(self)
                if cursorHoversItem(i) then
                    -- do something
                    MESSAGEMAN:Broadcast("SelectedItem", {def = item.Def, category = item.Parent})
                end
            end,
            UpdateItemCommand = function(self)
                index = (page-1) * itemsVisible + i
                item = items[index]
                if item ~= nil then
                    self:finishtweening()
                    self:diffusealpha(0)
                    self:smooth(animationSeconds)
                    self:diffusealpha(1)
                else
                    self:finishtweening()
                    self:smooth(animationSeconds)
                    self:diffusealpha(0)
                end
            end,
            UpdatePageMessageCommand = function(self)
                self:playcommand("UpdateItem")
            end,

            UIElements.QuadButton(1, 1) .. {
                Name = "ItemBG", -- also the "cursor" position
                InitCommand = function(self)
                    self:halign(0)
                    -- 97% full size to allow a gap for mouse hover logic reasons
                    self:zoomto(actuals.ListWidth - actuals.EdgeBuffer, yIncrement * 0.97)
                    self:diffusealpha(0)
                    self.alphaDeterminingFunction = function(self)
                        local alpha = 1
                        if isOver(self) then
                            alpha = buttonHoverAlpha
                            if cursorHoversItem(i) then
                                alpha = (buttonHoverAlpha + 1) / 2
                            end
                        else
                            alpha = 0
                            if cursorHoversItem(i) then
                                alpha = cursorAlpha
                            end
                        end

                        self:diffusealpha(alpha)
                    end
                end,
                CursorShowCommand = function(self)
                    self:smooth(cursorAnimationSeconds)
                    self:alphaDeterminingFunction()
                end,
                CursorHideCommand = function(self)
                    self:smooth(cursorAnimationSeconds)
                    self:alphaDeterminingFunction()
                end,
                MouseOverCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                MouseOutCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                UpdateCursorMessageCommand = function(self)
                    self:alphaDeterminingFunction()
                end,
                UpdateItemCommand = function(self)
                    self:alphaDeterminingFunction()
                end,
                MouseDownCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    cursorIndex = index
                    self:GetParent():playcommand("SelectCurrent")
                end,
                SelectedItemMessageCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "Text",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoom(listTextSize)
                    self:maxwidth((actuals.ListWidth - actuals.EdgeBuffer * 2) / listTextSize)
                end,
                UpdateItemCommand = function(self)
                    if item ~= nil then
                        if not item.isCategory then
                            self:x(actuals.EdgeBuffer)
                        else
                            self:x(0)
                        end
                        self:settext(item.Name)
                    end
                end,
            }
        }
    end

    local rightAreaWidth = actuals.MainDisplayWidth - (actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
    local t = Def.ActorFrame {
        Name = "MenuContainer",
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type == "InputEventType_Release" then return end

                local gameButton = event.button
                local key = event.DeviceInput.button
                local up = gameButton == "Up" or gameButton == "MenuUp"
                local down = gameButton == "Down" or gameButton == "MenuDown"
                local right = gameButton == "MenuRight" or gameButton == "Right"
                local left = gameButton == "MenuLeft" or gameButton == "Left"
                local enter = gameButton == "Start"
                local back = key == "DeviceButton_escape"

                if up or left then
                    moveCursor(-1)
                    self:playcommand("SelectCurrent")
                elseif down or right then
                    moveCursor(1)
                    self:playcommand("SelectCurrent")
                elseif enter then
                    self:playcommand("SelectCurrent")
                elseif back then
                    SCREENMAN:GetTopScreen():Cancel()
                end
            end)
            self:playcommand("UpdateCursor")
        end,
        Def.Quad {
            Name = "ScrollBar",
            InitCommand = function(self)
                self:zoomto(actuals.ScrollerWidth, actuals.MainDisplayHeight / maxPage)
                self:halign(0):valign(0)
                self:diffusealpha(0.6)
            end,
            UpdatePageMessageCommand = function(self)
                self:finishtweening()
                self:smooth(animationSeconds)
                self:y(actuals.MainDisplayHeight / maxPage * (page-1))
            end,
        },
        Def.Quad {
            Name = "MouseScrollArea",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:diffusealpha(0)
                self:zoomto(actuals.ScrollerWidth + actuals.ListWidth, actuals.MainDisplayHeight)
            end,
            MouseScrollMessageCommand = function(self, params)
                if isOver(self) then
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                end
            end,
        },
        Def.ActorFrame {
            Name = "SelectedItemContainer",
            InitCommand = function(self)
                self:x(actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
                -- make empty defaults load
                self:playcommand("UpdateSelectedItem")
            end,
            SelectedItemMessageCommand = function(self, params)
                self:playcommand("UpdateSelectedItem", params)
            end,

            LoadFont("Menu Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer)
                    self:zoom(titleTextSize)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.Name)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:maxwidth(((rightAreaWidth / 2) - actuals.EdgeBuffer) / titleTextSize)
                        else
                            self:maxwidth((rightAreaWidth - actuals.EdgeBuffer) / titleTextSize)
                        end
                    else
                        self:settext("")
                    end
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "ShortDescription",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer2)
                    self:skewx(-0.15)
                    self:zoom(subtitleTextSize)
                    self:maxheight((actuals.TopBuffer3 - actuals.TopBuffer2) / subtitleTextSize - textZoomFudge * 5)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.ShortDescription)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:wrapwidthpixels(((rightAreaWidth / 2) - actuals.EdgeBuffer) / subtitleTextSize)
                        else
                            self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer) / subtitleTextSize)
                        end
                    else
                        self:settext("")
                    end
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "Paragraph",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer3)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.Description)
                        self:zoom(descTextSize)
                        self:maxheight((actuals.MainDisplayHeight - actuals.TopBuffer3 - actuals.EdgeBuffer) / descTextSize)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:wrapwidthpixels(((rightAreaWidth / 2) - actuals.EdgeBuffer) / descTextSize)
                        else
                            self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer) / descTextSize)
                        end
                    else
                        self:zoom(subtitleTextSize)
                        self:settext("Select an item on the left to get info.\nScroll through the list for more categories.")
                        self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer) / subtitleTextSize)
                    end
                end,
            },
            UIElements.SpriteButton(1, 1, nil) .. {
                Name = "Image",
                InitCommand = function(self)
                    self:valign(0)
                    self:xy(rightAreaWidth / 4 * 3, actuals.TopBuffer)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        if def.Image ~= nil and def.Image ~= "" then
                            self:diffusealpha(1)
                            self:Load(def.Image)
                            local h = self:GetHeight()
                            local w = self:GetWidth()
                            local allowedHeight = actuals.MainDisplayHeight - (actuals.TopBuffer * 2)
                            local allowedWidth = rightAreaWidth - (actuals.EdgeBuffer + actuals.IconExitWidth)
                            if h >= allowedHeight and w >= allowedWidth then
                                if h * (allowedWidth / allowedHeight) >= w then
                                    self:zoom(allowedHeight / h)
                                else
                                    self:zoom(allowedWidth / w)
                                end
                            elseif h >= allowedHeight then
                                self:zoom(allowedHeight / h)
                            elseif w >= allowedWidth then
                                self:zoom(allowedWidth / w)
                            else
                                self:zoom(1)
                            end
                        else
                            self:diffusealpha(0)
                        end
                    else
                        self:diffusealpha(0)
                    end
                end,
            },
        }
    }

    for i = 1, itemsVisible do
        t[#t+1] = menuItem(i)
    end
    return t
end


local t = Def.ActorFrame {
    Name = "HelpDisplayFile",

    Def.ActorFrame {
        Name = "InfoBoxFrame",
        InitCommand = function(self)
            self:xy(actuals.InfoLeftGap, actuals.InfoTopGap)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.InfoWidth, actuals.InfoHeight)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Sprite {
            Name = "Logo",
            Texture = THEME:GetPathG("", "Logo"),
            InitCommand = function(self)
                self:xy(actuals.ScrollerWidth + (actuals.ListWidth / 2), actuals.InfoHeight / 2)
                self:zoomto(logoW, logoH)
            end
        },
        LoadColorFont("Menu Bold") .. {
            Name = "Text",
            InitCommand = function(self)
                local textw = actuals.InfoWidth - (actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
                local textx = actuals.InfoWidth - textw / 2
                self:xy(textx, actuals.InfoHeight/2)
                self:zoom(infoTextSize)
                self:maxheight((actuals.InfoHeight - (actuals.InfoVerticalBuffer*2)) / infoTextSize)
                self:wrapwidthpixels(textw / infoTextSize)
                self:settext("Help")
            end,
            SelectedItemMessageCommand = function(self, params)
                if params and params.category ~= nil then
                    self:settext(params.category)
                else
                    self:settext("Help")
                end
            end
        },
    },
    Def.ActorFrame {
        Name = "MainDisplayFrame",
        InitCommand = function(self)
            self:xy(actuals.MainDisplayLeftGap, actuals.MainDisplayTopGap)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.MainDisplayWidth, actuals.MainDisplayHeight)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Quad {
            Name = "Separator",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:x(actuals.ScrollerWidth + actuals.ListWidth)
                self:zoomto(actuals.SeparationGapWidth, actuals.MainDisplayHeight)
                self:diffuse(color("1,1,1"))
                self:diffusealpha(0.2)
            end,
        },
        helpMenu() .. {
            InitCommand = function(self)
                self:xy(0, 0)
            end,
        },
        UIElements.SpriteButton(1, 1, THEME:GetPathG("", "exit")) .. {
            Name = "Exit",
            InitCommand = function(self)
                self:valign(0):halign(1)
                self:xy(actuals.MainDisplayWidth - actuals.InfoVerticalBuffer/4, actuals.InfoVerticalBuffer/4)
                self:zoomto(actuals.IconExitWidth, actuals.IconExitHeight)
            end,
            MouseDownCommand = function(self, params)
                SCREENMAN:GetTopScreen():Cancel()
                TOOLTIP:Hide()
            end,
            MouseOverCommand = function(self, params)
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText("Exit")
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self, params)
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
        },
    },
    
}

return t
