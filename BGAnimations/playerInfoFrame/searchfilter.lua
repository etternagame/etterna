local t = Def.ActorFrame {Name = "SearchFile"}

local ratios = {
    Width = 782 / 1920,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything
    SliderColumnLeftGap = 196 / 1920, -- from left edge of text to left edge of sliders
    RightColumnLeftGap = 410 / 1920, -- from left edge of frame to left edge of text
    -- using the section height to give equidistant spacing between items with "less" work
    UpperSectionHeight = 440 / 1080, -- from bottom of upperlip to top of upper divider
    LowerSectionHeight = 485 / 1080, -- from bottom of upper divider to bottom of frame
}

local actuals = {
    Width = ratios.Width * SCREEN_WIDTH,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    SliderColumnLeftGap = ratios.SliderColumnLeftGap * SCREEN_WIDTH,
    RightColumnLeftGap = ratios.RightColumnLeftGap * SCREEN_WIDTH,
    UpperSectionHeight = ratios.UpperSectionHeight * SCREEN_HEIGHT,
    LowerSectionHeight = ratios.LowerSectionHeight * SCREEN_HEIGHT,
}

local function upperSection()

    -- the base text for each line
    local entryTextTable = {
        "Any Seach", -- this is spelled wrong on purpose
        "Title Search",
        "Subtitle Search",
        "Artist Search",
        "Author Search",
    }

    -- used to actually search for things in WheelDataManager
    local searchentry = {
        Title = "",
        Subtitle = "",
        Artist = "",
        Author = "",
    }

    -- current focused text entry field, 1 is "Any"
    -- based on entryTextTable
    local focusedField = 1

    -- text entry into a field causes the searchentry to be updated accordingly
    -- based on entryTextTable
    local entryFunction = {
        -- "Any Search"
        function(input)
            -- must parse the input for title, subtitle, artist, author
            -- same formatting as the old search
            local artistpos = input:find("artist=")
            local authorpos = input:find("author=")
            local titlepos = input:find("title=")
            local subtitlepos = input:find("subtitle=")

            -- because title is a substring of subtitle we have to check to see if the match is incorrect
            if titlepos ~= nil and subtitlepos ~= nil and titlepos + 3 == subtitlepos then
                titlepos = input:find("title=", titlepos + 1)
            end

            local foundartist = ""
            local foundauthor = ""
            local foundtitle = ""
            local foundsubtitle = ""

            if artistpos ~= nil or authorpos ~= nil or titlepos ~= nil or subtitlepos ~= nil then
                if artistpos ~= nil then
                    local strend = input:find("[;]", artistpos+1)
                    if strend == nil then strend = #input end
                    foundartist = input:sub(artistpos, strend)
                end
                if authorpos ~= nil then
                    local strend = input:find("[;]", authorpos+1)
                    if strend == nil then strend = #input end
                    foundauthor = input:sub(authorpos, strend)
                end
                if titlepos ~= nil then
                    local strend = input:find("[;]", titlepos+1)
                    if strend == nil then strend = #input end
                    foundtitle = input:sub(titlepos, strend)
                end
                if subtitlepos ~= nil then
                    local strend = input:find("[;]", subtitlepos+1)
                    if strend == nil then strend = #input end
                    foundsubtitle = input:sub(subtitlepos, strend)
                end
                searchentry.Title = foundtitle
                searchentry.Subtitle = foundsubtitle
                searchentry.Artist = foundartist
                searchentry.Author = foundauthor
            else
                searchentry.Title = input
                searchentry.Subtitle = ""
                searchentry.Artist = ""
                searchentry.Author = ""
            end
        end,
        -- "Title Search"
        function(input)
            searchentry.Title = input
        end,
        -- "Subtitle Search"
        function(input)
            searchentry.Subtitle = input
        end,
        -- "Artist Search"
        function(input)
            searchentry.Artist = input
        end,
        -- "Author Search"
        function(input)
            searchentry.Author = input
        end,
    }

    -- move focus of text entry to another line
    local function changeFocus(direction)
        focusedField = focusedField + direction
        if focusedField > #entryTextTable then focusedField = 1 end
        if focusedField < 1 then focusedField = #entryTextTable end

    end

    local function textEntryField(i)
        return Def.ActorFrame {
            Def.Quad {
                Name = "CursorUnderscore",
                InitCommand = function(self)
                end,
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                InitCommand = function(self)
                end,
                MouseOverCommand = function(self)
                end,
                MouseOutCommand = function(self)
                end,
            }
        }
    end

    local t = Def.ActorFrame {
        Name = "UpperSectionFrame",
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)

            end)
        end
    }

    for i = 1, #entryTextTable do
        t[#t+1] = textEntryField(i)
    end

    return t
end

local function lowerSection()

    local filterCategoryTable = {}

    local function filterSlider(i)

    end

    -- use this function to generate a new line for the right column
    -- this column has multiple purposes, so all this function will do is generate the base
    -- the base being: they are all BitmapText on a consistent column with consistent spacing
    local function filterMiscLine(i)

    end


    local t = Def.ActorFrame {
        Name = "LowerSectionFrame",
    }

    for i = 1, #filterCategoryTable do
        t[#t+1] = filterSlider(i)
    end

    return t
end

t[#t+1] = upperSection()
t[#t+1] = lowerSection()

return t