-- like a singleton
-- this manages all the data for the musicwheel
-- the purpose of it is to essentially cache wheel related things and sorts
-- we don't have to keep resorting the wheel every time we open a pack if this is correct
-- etc

WHEELDATA = {}
local firstTimeInit = false
-- quickly reset the WHEELDATA storage
function WHEELDATA.Reset(self)
    -- 1 is the "enum value" for Group sort, it should stay that way
    -- you can find the indices from the sortmodes table in this file
    self.CurrentSort = 1

    -- library of all Songs for this Game (all Styles)
    self.AllSongs = {}
    self.AllSongsByGroup = {}
    self.AllGroups = {}

    -- for the current sort, filtering and organization purposes
    self.AllFilteredSongs = {} -- equivalent of self.AllSongs but after the filter
    self.AllSongsByFolder = {} -- multipurpose; on Group sort, is identical to AllSongsByGroup
    self.AllFolders = {} -- this can be groups or "folders" like in the Title sort, etc
    self.StatsByFolder = {} -- stats for each folder
    self.TotalStats = {}

    -- filter info
    self.ActiveFilter = {
        metadata = {    -- search metadata to filter by; filters out all non matches
            Title = "",
            Subtitle = "",
            Artist = "",
            Author = "",
        },
        valid = nil, -- function expecting chart or song that returns true if it passes (this is left empty as free space for you, reader)
        requireTags = { -- require that a chart has tags
            mode = true,    -- true = AND, false = OR - requires either ALL or ANY of the tags are on the chart
            tags = {},
        },
        excludeTags = { -- remove all charts that have these tags
            mode = false,   -- true = AND, false = OR - hides charts that have either ALL or ANY of the tags
            tags = {},
        },
    }

    -- last generated list of WheelItems
    self.WheelItems = {}
end
WHEELDATA:Reset()

-- quickly reset just song search
function getEmptyActiveFilterMetadata()
    local metadata = {
        Title = "",
        Subtitle = "",
        Artist = "",
        Author = "",
    }
    return metadata
end

function WHEELDATA.ResetActiveFilterMetadata(self)
    self.ActiveFilter.metadata = getEmptyActiveFilterMetadata()
end

-- get wheelItems
function WHEELDATA.GetWheelItems(self)
    return self.WheelItems
end

-- set wheel items
-- meant to be used to keep a sense of global persistence to the wheelItems
-- use it after rebuilds
function WHEELDATA.SetWheelItems(self, t)
    self.WheelItems = t
end

-- check if the search is empty
function WHEELDATA.IsSearchFilterEmpty(self)
    return not (self.ActiveFilter.metadata.Title ~= "" or self.ActiveFilter.metadata.Subtitle ~= "" or self.ActiveFilter.metadata.Artist ~= "" or self.ActiveFilter.metadata.Author ~= "")
end

-- check if both tag filters are empty
function WHEELDATA.IsTagFilterEmpty(self)
    for _,__ in pairs(self.ActiveFilter.requireTags.tags) do
        return false
    end
    for _,__ in pairs(self.ActiveFilter.excludeTags.tags) do
        return false
    end
    return true
end

-- check if the filter is active
function WHEELDATA.IsFiltered(self)
    return self.ActiveFilter ~= nil and (self.ActiveFilter.valid ~= nil or not self:IsSearchFilterEmpty()) or FILTERMAN:AnyActiveFilter() or not self:IsTagFilterEmpty()
end

-- get the current search table
function WHEELDATA.GetSearch(self)
    return self.ActiveFilter.metadata
end

-- set the search table for filtering
-- does not actually sort the songs, trigger that with WHEELDATA:SortByCurrentSortmode()
function WHEELDATA.SetSearch(self, t)
    if t == nil then return end
    -- completely forgot what this is here for
    -- if s == nil or s:gsub("^%s*(.-)%s*$", "%1") == "" then
    if t.Title ~= nil then
        self.ActiveFilter.metadata.Title = t.Title:lower()
    else
        self.ActiveFilter.metadata.Title = ""
    end
    if t.Subtitle ~= nil then
        self.ActiveFilter.metadata.Subtitle = t.Subtitle:lower()
    else
        self.ActiveFilter.metadata.Subtitle = ""
    end
    if t.Artist ~= nil then
        self.ActiveFilter.metadata.Artist = t.Artist:lower()
    else
        self.ActiveFilter.metadata.Artist = ""
    end
    if t.Author ~= nil then
        self.ActiveFilter.metadata.Author = t.Author:lower()
    else
        self.ActiveFilter.metadata.Author = ""
    end
    -- end
end

-- set the list of tags to exclude
-- does not update the wheel
function WHEELDATA.SetExcludedTags(self, t)
    if t == nil then t = {} end
    self.ActiveFilter.excludeTags.tags = t
end

-- set the list of tags to require
-- does not update the wheel
function WHEELDATA.SetRequiredTags(self, t)
    if t == nil then t = {} end
    self.ActiveFilter.requireTags.tags = t
end

-- getter for list of tags to exclude
function WHEELDATA.GetExcludedTags(self)
    return self.ActiveFilter.excludeTags.tags
end

-- getter for list of tags to require
function WHEELDATA.GetRequiredTags(self)
    return self.ActiveFilter.requireTags.tags
end

-- set the excluded tag mode
-- true = AND, false = OR
function WHEELDATA.SetExcludedTagMode(self, m)
    if m == nil then
        m = not self.ActiveFilter.excludeTags.mode
    end
    self.ActiveFilter.excludeTags.mode = m
end

-- set the required tag mode
-- true = AND, false = OR
function WHEELDATA.SetRequiredTagMode(self, m)
    if m == nil then
        m = not self.ActiveFilter.requireTags.mode
    end
    self.ActiveFilter.requireTags.mode = m
end

-- getter for required tag mode
-- true = AND, false = OR
function WHEELDATA.GetRequiredTagMode(self)
    return self.ActiveFilter.requireTags.mode
end

-- getter for excluded tag mode
-- true = AND, false = OR
function WHEELDATA.GetExcludedTagMode(self)
    return self.ActiveFilter.excludeTags.mode
end

-- resetter for required tag filter
function WHEELDATA.ResetRequiredTags(self)
    self.ActiveFilter.requireTags = { -- require that a chart has tags
        mode = true,    -- true = AND, false = OR - requires either ALL or ANY of the tags are on the chart
        tags = {},
    }
end

-- resetter for excluded tag filter
function WHEELDATA.ResetExcludedTags(self)
    self.ActiveFilter.excludeTags = { -- remove all charts that have these tags
        mode = false,   -- true = AND, false = OR - hides charts that have either ALL or ANY of the tags
        tags = {},
    }
end

-- private function to handle checking to see if a chart passes the tag filters
-- to reduce code copypasta
-- expects a chart and the list of tags given by TAGMAN
-- returns false if the chart fails (which means the chart should not appear)
local function chartPassesTagFilters(c, tags)
    local g = c:GetChartKey()
    -- require tag filter
    if #WHEELDATA.ActiveFilter.requireTags.tags > 0 then
        if WHEELDATA.ActiveFilter.requireTags.mode then
            -- mode true = AND
            for _, tag in ipairs(WHEELDATA.ActiveFilter.requireTags.tags) do
                -- fail the chart if it does not have a tag
                if tags[tag][g] == nil or tags[tag][g] ~= 1 then
                    return false
                end
            end
        else
            local tagpass = false
            -- mode false = OR
            for _, tag in ipairs(WHEELDATA.ActiveFilter.requireTags.tags) do
                -- pass the chart if it has any tags
                if tags[tag][g] ~= nil and tags[tag][g] == 1 then
                    tagpass = true
                    break
                end
            end
            if not tagpass then return false end
        end
    end

    -- exclude tag filter
    if #WHEELDATA.ActiveFilter.excludeTags.tags > 0 then
        if WHEELDATA.ActiveFilter.excludeTags.mode then
            -- mode true = AND
            local passed = false
            for _, tag in ipairs(WHEELDATA.ActiveFilter.excludeTags.tags) do
                -- fail the chart if it has all tags
                -- so logically, pass the chart if it is missing a tag
                if tags[tag][g] == nil or tags[tag][g] ~= 1 then
                    passed = true
                    break
                end
            end
            if not passed then return false end
        else
            -- mode false = OR
            for _, tag in ipairs(WHEELDATA.ActiveFilter.excludeTags.tags) do
                -- fail the chart if it has any tags
                if tags[tag][g] ~= nil and tags[tag][g] == 1 then
                    return false
                end
            end
        end
    end
    return true
end

-- give a song, chartkey, or steps
-- works as AND for now, where if both search and valid are set then both have to pass
-- returns true if filter passed
function WHEELDATA.FilterCheck(self, g)
    if not self:IsFiltered() then return true end
    local passed = true
    local tags = TAGMAN:get_data().playerTags

    -- TODO: make the chart specific portions of this work for song search just in case
    if type(g) == "string" then
        -- working with a Chartkey
        local c = SONGMAN:GetStepsByChartKey(g)
        -- arbitrary function filter (passing Steps)
        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(c) then
                return false
            end
        end
        
        -- tag filters
        passed = passed and chartPassesTagFilters(c, tags)
    elseif g.GetAllSteps then
        -- working with a Song
        -- song search
        local title = g:GetDisplayMainTitle():lower()
        local author = g:GetOrTryAtLeastToGetSimfileAuthor():lower()
        local artist = g:GetDisplayArtist():lower()
        local subtitle = g:GetDisplaySubTitle():lower()
        if not self:IsSearchFilterEmpty() then
            local startIndex = 1
            local dontUsePatternMatching = true
            if self.ActiveFilter.metadata.Title ~= "" then
               if title:find(self.ActiveFilter.metadata.Title, startIndex, dontUsePatternMatching) == nil then return false end
            end
            if self.ActiveFilter.metadata.Subtitle ~= "" then
                if subtitle:find(self.ActiveFilter.metadata.Subtitle, startIndex, dontUsePatternMatching) == nil then return false end
            end
            if self.ActiveFilter.metadata.Author ~= "" then
                if author:find(self.ActiveFilter.metadata.Author, startIndex, dontUsePatternMatching) == nil then return false end
            end
            if self.ActiveFilter.metadata.Artist ~= "" then
                if artist:find(self.ActiveFilter.metadata.Artist, startIndex, dontUsePatternMatching) == nil then return false end
            end
        end

        -- arbitrary function filter (passing Song)
        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(g) then
                return false
            end
        end

        -- c++ FILTERMAN mixed in with tag filtering
        local charts = self:GetChartsMatchingFilter(g)
        if charts == nil or #charts == 0 then return false end

        -- tag filters -- if any chart passes, this song passes
        local tagFilterFails = 0
        for _, c in ipairs(charts) do
            if not chartPassesTagFilters(c, tags) then
                tagFilterFails = tagFilterFails + 1
            end
        end
        return tagFilterFails ~= #charts
    elseif g.GetChartKey then
        -- working with a Steps

        -- arbitrary function filter (passing Steps)
        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(g) then
                return false
            end
        end

        -- c++ FILTERMAN mixed in with tag filtering
        -- note: you would think that since C++ can check to see if a chart passes the filter we can direct call here
        -- well, we can't and the reason is that you can't do something like Steps:PassesFilter()
        -- instead, we have to let C++ take over and give us a list of a Song's charts that pass the filter
        -- ... of course I could implement this myself in C++ to make it a lot quicker but alas, today is not the day to do that
        local ck = g:GetChartKey()
        local s = SONGMAN:GetSongByChartKey(ck)
        if s ~= nil then
            local tmpbool = false
            local charts = self:GetChartsMatchingFilter(s)
            for _,c in ipairs(charts) do
                if c:GetChartKey() == ck then
                    tmpbool = true
                end
            end
            return tmpbool and passed
        end

        -- tag filters
        passed = passed and chartPassesTagFilters(g, tags)
    end

    return passed
end

-- return whether or not the given song matches all active search terms
-- note: the search terms should be set by WHEELDATA:SetSearch thus are already lowercase
local function isExactMetadataMatch(song)
    if WHEELDATA:IsSearchFilterEmpty() then return false end
    if not WHEELDATA:IsSearchFilterEmpty() then
        local title = song:GetDisplayMainTitle():lower()
        local author = song:GetOrTryAtLeastToGetSimfileAuthor():lower()
        local artist = song:GetDisplayArtist():lower()
        local subtitle = song:GetDisplaySubTitle():lower()
        if WHEELDATA.ActiveFilter.metadata.Title ~= "" then
           if title ~= WHEELDATA.ActiveFilter.metadata.Title then return false end
        end
        if WHEELDATA.ActiveFilter.metadata.Subtitle ~= "" then
            if subtitle ~= WHEELDATA.ActiveFilter.metadata.Subtitle then return false end
        end
        if WHEELDATA.ActiveFilter.metadata.Author ~= "" then
            if author ~= WHEELDATA.ActiveFilter.metadata.Author then return false end
        end
        if WHEELDATA.ActiveFilter.metadata.Artist ~= "" then
            if artist ~= WHEELDATA.ActiveFilter.metadata.Artist then return false end
        end
        return true
    end
    return false
end

-- based on the currently set filters and search terms, find any perfectly matching song
-- if none exists, return nil
-- if one exists, return the song
function WHEELDATA.FindExactSearchMatchSong(self)
    if not self:IsFiltered() then return nil end

    for _, song in ipairs(self.AllFilteredSongs) do
        if isExactMetadataMatch(song) then return song end
    end
    return nil
end

-- return the singular song that remains after a filter pass
-- this will only work if between all matches they all contain the same set of chartkeys
-- returns nil otherwise
function WHEELDATA.FindTheOnlySearchResult(self)
    if #self.AllFilteredSongs == 0 then return nil end
    -- collect the chart keys we want to try to match across all filtered songs
    local cks = {}
    for _, c in ipairs(self:GetChartsMatchingFilter(self.AllFilteredSongs[1])) do
        cks[#cks+1] = c:GetChartKey()
    end

    -- match those chart keys across every single filtered song
    for _, song in ipairs(self.AllFilteredSongs) do
        local cs = self:GetChartsMatchingFilter(song)
        local thesekeys = {}
        for __, c in ipairs(self:GetChartsMatchingFilter(song)) do
            thesekeys[#thesekeys+1] = c:GetChartKey()
        end
        
        -- verify that all the chart keys match in both directions.
        -- this allows for duplicate chartkeys on both files (why would you do this?)
        -- but if either set is a superset that is not exactly equivalent, we fail
        for __, k in ipairs(cks) do
            if findKeyOf(thesekeys, k) == nil then return nil end
        end
        for __, k in ipairs(thesekeys) do
            if findKeyOf(cks, k) == nil then return nil end
        end
    end
    return self.AllFilteredSongs[1]
end

-- obtain all songs loaded that pass the active filter
-- requires WHEELDATA.AllSongs to be filled already
function WHEELDATA.GetAllSongsPassingFilter(self)
    if not self:IsFiltered() then return self.AllSongs end

    local t = {}
    for _, song in ipairs(self.AllSongs) do
        if self:FilterCheck(song) then
            t[#t+1] = song
        end
    end
    return t
end

-- a kind of shadow to get the list of charts matching the lua filter and the c++ filter
-- does not consider song search, but does consider tag filtering
function WHEELDATA.GetChartsMatchingFilter(self, song)
    local charts = song:GetChartsMatchingFilter() -- FILTERMAN check
    local tags = TAGMAN:get_data().playerTags
    local t = {}
    for i, c in ipairs(charts) do
        local passed = true

        -- arbitrary validation function check
        if self.ActiveFilter.valid ~= nil and not self.ActiveFilter.valid(c) then
            passed = false
        end

        -- tag check
        if not chartPassesTagFilters(c, tags) then
            passed = false
        end

        if passed then
            t[#t+1] = c
        end
    end
    return t
end

-- quickly empty the sorted lists
function WHEELDATA.ResetSorts(self)
    self.AllFilteredSongs = {}
    self.AllFolders = {}
    self.AllSongsByFolder = {}
    self.StatsByFolder = {}
    self.TotalStats = {}
end

local sortmodes = {
    [0] = "Sort Mode Menu",
    "Group", -- group by pack, all "alphabetical order"
    "Title", -- group by title letter
    "Author", -- group by chartist
    "Favorite", -- show only Favorites
    "Cleartype", -- group by best cleartype on charts
    "Ungrouped", -- no grouping, alphabetical order
    "BPM", -- group by bpm ranges
    "Top Grade", -- group by best grade
    "Artist", -- group by song artist
    "Overall MSD", -- group by highest overall MSD
    "Stream MSD", -- group by highest stream MSD
    "Jumpstream MSD", -- group by highest jumpstream MSD
    "Handstream MSD", -- group by highest handstream MSD
    "Stamina MSD", -- group by highest stamina MSD
    "Jackspeed MSD", -- group by highest jack MSD
    "Chordjack MSD", -- group by highest chordjack MSD
    "Technical MSD", -- group by highest tech MSD
    "Length", -- group by length range
    "Pack Clear Percent", -- group by pack, order packs by percentage of grades, then sort by grade; songs ordered by grade
    "Last Score Date", -- group by month, order packs chronlogically, songs ordered alphabetically
    "PB Date (Percent)", -- same as above, but picks the highest percent
    "PB Date (Score Rating)", -- same as above, but picks the highest rating
    "Tournament", -- same as group alphabetical, but for packs with [x-x] title prefixes, sort by those prefixes
}
local function sortToString(val)
    return sortmodes[val]
end

-- getter for if you are in sortmode menu
-- useful for like 2 things on the wheel
function WHEELDATA.inSortModeMenu(self)
    return self.CurrentSort == 0
end

-- mimicing SongUtil::MakeSortString here to keep behavior consistent
function WHEELDATA.makeSortString(s)
    local st = s:upper()
    if st == "." then return "." end
    if #st > 0 then
        if st:find("[.]") == 1 then
            st = string.sub(st, 2)
        end
        if st:find("[#]") == 1 then
            return st
        end

        local fchar = string.byte(st)
        if (fchar < string.byte("A") or fchar > string.byte("Z")) and (fchar < string.byte("0") or fchar > string.byte("9")) then
            st = string.char(126) .. st
        end
    end
    return st
end

-- this function sits here for scoping reasons
-- essentially mimics the C++ behavior to get the first letter of a song
-- could be generalized later
local function getTitleSortFoldernameForSong(song)
    local title = song:GetTranslitMainTitle()
    title = WHEELDATA.makeSortString(title)
    if #title == 0 then return "" end
    local fchar = string.byte(title)
    if fchar >= string.byte("0") and fchar <= string.byte("9") then
        return "0-9"
    end
    if fchar < string.byte("A") or fchar > string.byte("Z") then
        return "?????"
    end
    return string.sub(title, 1, 1)
end

-- this function sits here for scoping reasons
-- essentially mimics the C++ behavior to get the first letter of an artist
local function getArtistSortFoldernameForSong(song)
    local artist = song:GetTranslitArtist()
    artist = WHEELDATA.makeSortString(artist)
    if #artist == 0 then return "" end
    local fchar = string.byte(artist)
    if fchar >= string.byte("0") and fchar <= string.byte("9") then
        return "0-9"
    end
    if fchar < string.byte("A") or fchar > string.byte("Z") then
        return "?????"
    end
    return string.sub(artist, 1, 1)
end

-- this function sits here for scoping reasons
-- gets the author of a file, for foldername purposes
local function getAuthorSortFoldernameForSong(song)
    local author = strtrim(song:GetOrTryAtLeastToGetSimfileAuthor():lower():gsub("^%l", string.upper))
    if author:upper() == "AUTHOR UNKNOWN" then author = "??????" end
    return author
end

-- this function sits here for scoping reasons
-- gets the best cleartype of a song, for all charts in the song, matching the active filter
local function getBestCleartypeForSong(song)
    local cleartype = 13 -- "No Play"
    for __, chart in ipairs(WHEELDATA:GetChartsMatchingFilter(song)) do
        local scorestack = SCOREMAN:GetScoresByKey(chart:GetChartKey())
        -- scorestack is nil if no scores on the chart
        if scorestack ~= nil then
            -- the scores are in lists for each rate
            -- find the highest
            for ___, l in pairs(scorestack) do
                local scoresatrate = l:GetScores()
                for ____, s in ipairs(scoresatrate) do
                    local ct = getClearTypeFromScore(s, 99)
                    if ct < cleartype then
                        cleartype = ct
                    end
                end
            end
        end
    end
    return getClearTypeText(cleartype)
end

-- this function sits here for scoping reasons
-- gets the bpm range for the file, for foldername purposes
local function getBPMSortFoldernameForSong(song)
    local bpmdivision = 20
    local bpms = song:GetDisplayBpms(true)
    local maxbpm = bpms[2]
    maxbpm = maxbpm + bpmdivision - (maxbpm % bpmdivision) - 1
    return string.format("%03d-%03d", maxbpm - (bpmdivision - 1), maxbpm)
end

-- this function sits here for scoping reasons
-- gets the length range for the file, for foldername purposes
local function getLengthSortFoldernameForSong(song)
    local lengthdivision = 60 -- in seconds
    local len_a = 0
    local charts = WHEELDATA:GetChartsMatchingFilter(song)
    for _, s in ipairs(charts) do
        local len = s:GetLengthSeconds()
        len_a = len > len_a and len or len_a
    end
    local maxlen = len_a
    maxlen = maxlen + (lengthdivision - (maxlen % lengthdivision) - 1)
    local minlen = maxlen - (lengthdivision - 1)
    return string.format("%s-%s", SecondsToMMSS(minlen), SecondsToMMSS(maxlen))
end

-- this function sits here for scoping reasons
-- gets the top grade for the song for foldername purposes
local function getTopGradeSortFoldernameForSong(song)
    local grade = song:GetHighestGrade()
    if grade == nil then
        return "???"
    else
        if grade:find("Invalid") ~= nil then
            return "???"
        end
        return THEME:GetString("Grade", grade:sub(#"Grade_T")), grade
    end
end

local function getHighestDiffForSongBySkillset(song, skillset)
    local charts = WHEELDATA:GetChartsMatchingFilter(song)
    local highest = 0
    for _, s in ipairs(charts) do
        local msd = s:GetMSD(1, skillset)
        if msd > highest then highest = msd end
    end
    return highest
end

-- gets the month of the most recent score
local function getLastMonthSortFoldernameForSong(song)
    local charts = WHEELDATA:GetChartsMatchingFilter(song)
    local recentDT = nil
    for _, chart in ipairs(charts) do
        local scorestack = SCOREMAN:GetScoresByKey(chart:GetChartKey())
        local useswarps = chart:GetTimingData():HasWarps()

        -- scorestack is nil if no scores on the chart
        -- skip if the chart has negbpms: these scores are always invalid for now and ruin lamps
        if scorestack ~= nil and not useswarps then
            -- the scores are in lists for each rate
            -- find the highest
            for ___, l in pairs(scorestack) do
                local scoresatrate = l:GetScores()
                for ____, s in ipairs(scoresatrate) do
                    local dt = getScoreDate(s)
                    if dt ~= "" then
                        if compareDates(recentDT, dt) then
                            recentDT = dt
                        end
                    end
                end
            end
        end
    end
    if recentDT == nil or recentDT == "" then
        return "N/A"
    else
        return extractYearAndMonthFromDateString(recentDT)
    end
end

-- gets the month of the highest percent score
local function getPBPercentMonthSortFoldernameForSong(song)
    local charts = WHEELDATA:GetChartsMatchingFilter(song)
    local pbscore = nil
    for _, chart in ipairs(charts) do
        local scorestack = SCOREMAN:GetScoresByKey(chart:GetChartKey())
        local useswarps = chart:GetTimingData():HasWarps()

        -- scorestack is nil if no scores on the chart
        -- skip if the chart has negbpms: these scores are always invalid for now and ruin lamps
        if scorestack ~= nil and not useswarps then
            -- the scores are in lists for each rate
            -- find the highest
            for ___, l in pairs(scorestack) do
                local scoresatrate = l:GetScores()
                for ____, s in ipairs(scoresatrate) do
                    local wp = s:GetWifeScore()
                    if pbscore == nil or wp > pbscore:GetWifeScore() then
                        pbscore = s
                    end
                end
            end
        end
    end
    if pbscore == nil then
        return "N/A"
    else
        return extractYearAndMonthFromDateString(getScoreDate(pbscore))
    end
end

-- gets the month of the highest rated score
local function getPBRatingMonthSortFoldernameForSong(song)
    local charts = WHEELDATA:GetChartsMatchingFilter(song)
    local pbscore = nil
    for _, chart in ipairs(charts) do
        local scorestack = SCOREMAN:GetScoresByKey(chart:GetChartKey())
        local useswarps = chart:GetTimingData():HasWarps()

        -- scorestack is nil if no scores on the chart
        -- skip if the chart has negbpms: these scores are always invalid for now and ruin lamps
        if scorestack ~= nil and not useswarps then
            -- the scores are in lists for each rate
            -- find the highest
            for ___, l in pairs(scorestack) do
                local scoresatrate = l:GetScores()
                for ____, s in ipairs(scoresatrate) do
                    local ssr = s:GetSkillsetSSR("Overall")
                    if pbscore == nil or ssr > pbscore:GetSkillsetSSR("Overall") then
                        pbscore = s
                    end
                end
            end
        end
    end
    if pbscore == nil then
        return "N/A"
    else
        return extractYearAndMonthFromDateString(getScoreDate(pbscore))
    end
end

-- functions responsible for actually sorting things according to the sortmodes table
-- each member is a table of functions
-- the first function modifies WHEELDATA by sorting its items
-- the second function describes the method used to find the folder that a song is in
-- the third function describes the method used to find the folder banner given a folder name
local sortmodeImplementations = {
    [0] = { -- The Sort Mode Menu -- all "group" items but pressing enter opens a sort mode
        function()
            WHEELDATA:ResetSorts()
            for _, v in ipairs(sortmodes) do
                WHEELDATA.AllSongsByFolder[v] = {}
                WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = "Sort by "..v
            end
        end,
        function(song)
            return nil
        end,
        function(packName)
            return ""
        end,
    },

    {   -- "Group" sort -- by pack, alphabetical within
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- for reasons determined by higher powers, literally mimic the behavior of AllSongsByGroup construction
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end
            -- sort the groups and then songlists in groups
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- "Title" sort -- by song title, alphabetical within
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = getTitleSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end
            -- sort groups and then songlists in groups
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getTitleSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- "Author" sort -- by chart artist(s), alphabetical within
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = getAuthorSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end
            -- sort groups and then songlists in groups
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getAuthorSortFoldernameForSong(song)
        end,
        function(packName)
            -- take the given folder and pull the cdtitle of the first song
            -- we don't cache this value so looping until we find a valid image isn't a good idea
            local s = WHEELDATA.AllSongsByFolder[packName]
            if s ~= nil then
                local p = s[1]:GetCDTitlePath()
                if p ~= nil then
                    return p
                end
            end
            return ""
        end,
    },

    {   -- Favorite sort -- alphabetical order, all favorited charts, 1 folder
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()
            local fname = "Favorites"

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                -- favorited songs only
                if song:IsFavorited() then
                    if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                        WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                    else
                        WHEELDATA.AllSongsByFolder[fname] = {song}
                        WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                    end
                    WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
                end
            end

            -- theres only 1 group, just sort the favorites alphabetically
            table.sort(
                WHEELDATA.AllSongsByFolder[fname],
                SongUtil.SongTitleComparator
            )
        end,
        function(song)
            return "Favorites"
        end,
        function(packName)
            local s = WHEELDATA.AllSongsByFolder["Favorites"]
            if s ~= nil then
                -- pick the middle cdtitle
                local p = s[clamp(math.floor(#s/2),1,#s)]:GetCDTitlePath()
                if p ~= nil then
                    return p
                end
            end
            return ""
        end,
    },

    {   -- Cleartype sort -- alphabetical order, by sortmode, folders are cleartypes
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = getBestCleartypeForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end
            -- sort groups and then songlists in groups
            table.sort(WHEELDATA.AllFolders,
                function(a,b)
                    -- folders are sorted by cleartype value instead of alphabetical
                    local aa = getClearTypeIndexFromValue(a)
                    local bb = getClearTypeIndexFromValue(b)
                    return aa < bb
                end
            )
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getBestCleartypeForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Ungrouped sort -- alphabetical order, 1 folder
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()
            local fname = "Ungrouped"

            -- build the song list in the group
            for _, song in ipairs(songs) do
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song 
            end

            -- sort the song list
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return "Ungrouped"
        end,
        function(packName)
            return ""
        end,
    },

    {   -- BPM sort -- alphabetical order within folders, folders are bpm ranges
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = getBPMSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    local af = a:gsub("-", "")
                    local bf = b:gsub("-", "")
                    return tonumber(af) < tonumber(bf)
                end
            )
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getBPMSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Top Grade sort -- alphabetical order within folders, folders are best grade on song
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- dynamically track what the localized grades mean
            -- to fix things for the sort function
            local gradezz = {}

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname, originalgrade = getTopGradeSortFoldernameForSong(song)
                gradezz[fname] = originalgrade

                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    if not gradezz[a] then return false end
                    if not gradezz[b] then return true end
                    return compareGrades(gradezz[a], gradezz[b])
                end
            )
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getTopGradeSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Artist sort -- alphabetical order within folders, folders are artist names
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = getArtistSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongArtistComparator
                )
            end
        end,
        function(song)
            return getArtistSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Overall MSD sort -- pack folders sorted internally by overall
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 1) < getHighestDiffForSongBySkillset(b, 1)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Stream MSD sort -- pack folders sorted internally by stream
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 2) < getHighestDiffForSongBySkillset(b, 2)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Jumpstream MSD sort -- pack folders sorted internally by js
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 3) < getHighestDiffForSongBySkillset(b, 3)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Handstream MSD sort -- pack folders sorted internally by hs
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 4) < getHighestDiffForSongBySkillset(b, 4)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Stamina MSD sort -- pack folders sorted internally by stamina
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 5) < getHighestDiffForSongBySkillset(b, 5)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Jackspeed MSD sort -- pack folders sorted internally by jacks
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 6) < getHighestDiffForSongBySkillset(b, 6)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Chordjack MSD sort -- pack folders sorted internally by chordjacks
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 7) < getHighestDiffForSongBySkillset(b, 7)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Technical MSD sort -- pack folders sorted internally by technical
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        return getHighestDiffForSongBySkillset(a, 8) < getHighestDiffForSongBySkillset(b, 8)
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Length sort -- alphabetical order, folders of length ranges
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(songs) do
                local fname = getLengthSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders and songs
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    local af = a:gsub("-", ""):gsub(":", "")
                    local bf = b:gsub("-", ""):gsub(":", "")
                    return tonumber(af) < tonumber(bf)
                end
            )
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getLengthSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Pack Clear Percent sort -- group by pack, order packs by percentage of grades, then sort by grade; songs ordered by grade
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- put all the songs in normal groups
            for _, song in ipairs(songs) do
                local fname = song:GetGroupName()
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- calculate stats for the groups before sorting
            WHEELDATA:RefreshStats()

            -- sort by folders by percentage of songs in the groups that have scores
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    local astats = WHEELDATA:GetFolderClearStats(a)
                    local bstats = WHEELDATA:GetFolderClearStats(b)
                    local aprop = astats.filesWithScores / math.max(astats.validFilesOverall, 1)
                    local bprop = bstats.filesWithScores / math.max(bstats.validFilesOverall, 1)
                    if aprop == bprop then
                        local agrade = astats.lamp
                        local bgrade = bstats.lamp
                        if agrade == bgrade then
                            return a:lower() < b:lower()
                        else
                            return compareGrades(agrade, bgrade)
                        end
                    else
                        return aprop > bprop
                    end
                end
            )
            -- sort each folder by grade, same grades are alphabetical
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    function(a,b)
                        local agrade = a:GetHighestGrade()
                        local bgrade = b:GetHighestGrade()
                        if agrade == bgrade then
                            return SongUtil.SongTitleComparator(a, b)
                        else
                            return compareGrades(agrade, bgrade)
                        end
                    end
                )
            end
        end,
        function(song)
            return song:GetGroupName()
        end,
        function(packName)
            return SONGMAN:GetSongGroupBannerPath(packName)
        end,
    },

    {   -- Month of last score sort -- group by month, order packs chronlogically, songs ordered alphabetically
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- put all the songs in group by month
            for _, song in ipairs(songs) do
                local fname = getLastMonthSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders by the month (newest first)
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    return compareDates(b,a)
                end
            )

            -- sort songs alphabetically
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getLastMonthSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Month of PB percent sort -- group by month, order packs chronlogically, songs ordered alphabetically
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- put all the songs in group by month
            for _, song in ipairs(songs) do
                local fname = getPBPercentMonthSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders by the month (newest first)
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    return compareDates(b,a)
                end
            )

            -- sort songs alphabetically
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getPBPercentMonthSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Month of PB SSR sort -- group by month, order packs chronlogically, songs ordered alphabetically
        function()
            WHEELDATA:ResetSorts()
            local songs = WHEELDATA:GetAllSongsPassingFilter()

            -- put all the songs in group by month
            for _, song in ipairs(songs) do
                local fname = getPBRatingMonthSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
                WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
            end

            -- sort folders by the month (newest first)
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b)
                    return compareDates(b,a)
                end
            )

            -- sort songs alphabetically
            for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
                table.sort(
                    songlist,
                    SongUtil.SongTitleComparator
                )
            end
        end,
        function(song)
            return getPBRatingMonthSortFoldernameForSong(song)
        end,
        function(packName)
            return ""
        end,
    },

    {   -- Tournament sort -- group by song group, sort all alphabetically, but if title has a [x-x] prefix, sort by that
    function()
        WHEELDATA:ResetSorts()
        local songs = WHEELDATA:GetAllSongsPassingFilter()

        -- for reasons determined by higher powers, literally mimic the behavior of AllSongsByGroup construction
        for _, song in ipairs(songs) do
            local fname = song:GetGroupName()
            if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
            else
                WHEELDATA.AllSongsByFolder[fname] = {song}
                WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
            end
            WHEELDATA.AllFilteredSongs[#WHEELDATA.AllFilteredSongs + 1] = song
        end

        local function getPrefixData(title)
            if title == nil then return "" end
            local f1, f2 = title:find("%[[%w%s%-]+%].+")
            if f1 ~= nil then title:sub(f1, f2) end
            return ""
        end

        -- sort the groups and then songlists in groups
        table.sort(WHEELDATA.AllFolders, function(a,b) return a:lower() < b:lower() end)
        for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
            table.sort(
                songlist,
                function(a,b)
                    local aPrefix = getPrefixData(a:GetMainTitle()) or ""
                    local aPrefixTranslit = getPrefixData(a:GetTranslitMainTitle()) or ""
                    local bPrefix = getPrefixData(b:GetMainTitle()) or ""
                    local bPrefixTranslit = getPrefixData(b:GetTranslitMainTitle()) or ""
                    local acomp = ""
                    local bcomp = ""
                    if aPrefix ~= "" then acomp = aPrefix or "" end
                    if aPrefixTranslit ~= "" then acomp = aPrefixTranslit or "" end
                    if bPrefix ~= "" then bcomp = bPrefix or "" end
                    if bPrefixTranslit ~= "" then bcomp = bPrefixTranslit or "" end
                    if (acomp == "" or acomp == nil) and (bcomp == "" or bcomp == nil) then
                        return SongUtil.SongTitleComparator(a, b)
                    else
                        return WHEELDATA:makeSortString(acomp) < WHEELDATA:makeSortString(bcomp)
                    end
                end
            )
        end
    end,
    function(song)
        return song:GetGroupName()
    end,
    function(packName)
        return SONGMAN:GetSongGroupBannerPath(packName)
    end,
    }
}

-- get the value and string value of the current sort
function WHEELDATA.GetCurrentSort(self)
    return self.CurrentSort, sortToString(self.CurrentSort)
end

-- set the value for the current sort
-- needs either a string or an index
-- returns a status of successful sort value change
function WHEELDATA.SetCurrentSort(self, s)
    if sortmodes[s] ~= nil then
        self.CurrentSort = s
        return true
    else
        local k = findKeyOf(sortmodes, s)
        if k ~= nil then
            self.CurrentSort = k
            return true
        end
    end
    return false
end

-- getter for the folder banner for a folder name
-- returns either a valid path or ""
function WHEELDATA.GetFolderBanner(self, folderName)
    return sortmodeImplementations[self.CurrentSort][3](folderName)
end

function WHEELDATA.SortByCurrentSortmode(self)
    local sortval, sortstr = self:GetCurrentSort()
    -- if sort is invalid, make sure the output is empty and exit
    if sortval == nil then self:ResetSorts() return end

    -- run the sort function based on the sortmode
    local tbefore = GetTimeSinceStart()
    local f = sortmodeImplementations[sortval][1]
    f()
    local tafter = GetTimeSinceStart()

    -- prevent some errors for empty song list
    if #self.AllFolders == 0 then
        self.AllFolders = {"NO SONGS"}
    end

    -- sort timing debug
    print(string.format("WHEELDATA -- Sorting took %f.", tafter - tbefore))
    MESSAGEMAN:Broadcast("FinishedSort")
end

-- update the song list for the current gamemode
-- sort all songs by current sort
function WHEELDATA.SetAllSongs(self)
    self.AllSongs = {}
    self.AllSongsByGroup = {}
    self.AllGroups = {}
    for _, song in ipairs(SONGMAN:GetAllSongs()) do
        if #song:GetChartsOfCurrentGameMode() > 0 then
            self.AllSongs[#self.AllSongs+1] = song
            local group = song:GetGroupName()
            if self.AllSongsByGroup[group] == nil then
                self.AllSongsByGroup[group] = {}
                self.AllGroups[#self.AllGroups+1] = group
            end
            self.AllSongsByGroup[group][#self.AllSongsByGroup[group]+1] = song
        end
    end
    self:SortByCurrentSortmode()
end

-- getter for the count of the list of songs before filtering, only for the current game
function WHEELDATA.GetSongCount(self)
    return #self.AllSongs
end

-- getter for the count of the list of songs visible after the filter
function WHEELDATA.GetFilteredSongCount(self)
    return #self.AllFilteredSongs
end

-- getter for the list of unfiltered groups
function WHEELDATA.GetAllGroups(self)
    return self.AllGroups
end

-- getter for the list of folders
function WHEELDATA.GetFilteredFolders(self)
    return self.AllFolders
end

-- getter for the list of songs in a folder
function WHEELDATA.GetSongsInFolder(self, name)
    return self.AllSongsByFolder[name]
end

-- getter for a random song in a folder
function WHEELDATA.GetRandomSongInFolder(self, name)
    if self.AllSongsByFolder[name] == nil then
        return nil
    else
        return self.AllSongsByFolder[name][math.random(#self.AllSongsByFolder[name])]
    end
end

-- getter for a random folder
function WHEELDATA.GetRandomFolder(self)
    return self.AllFolders[math.random(#self.AllFolders)]
end

-- to simplify a lot of copy paste....
-- builds and returns a list of folders and songs
function WHEELDATA.GetWheelItemsForOpenedFolder(self, name)
    local groups = self.AllFolders
    local songs = self.AllSongsByFolder[name]

    -- destroy the group we are looking for, create 2 resulting tables
    local groupTopHalf, groupLowHalf = tablesplit(groups, name)
    -- reconstruct the table, putting back the removed group and also the songs for it alongside
    local finalItems = tableconcat(groupTopHalf, {name}, songs, groupLowHalf)

    return finalItems
end

-- generous getter for people who like to use similarly named functions
function WHEELDATA.GetWheelItemsForClosedFolder(self, name)
    return self.AllFolders
end

-- getter for the index of a folder
-- NOTE: this assumes NO GROUPS ARE OPENED
function WHEELDATA.FindIndexOfFolder(self, name)
    local folderIndex = findKeyOf(self.AllFolders, name)
    if folderIndex == nil then return -1 end

    return folderIndex
end

-- find the index of the desired song in the filtered list of songs
-- first by looking through all groups in order, then opening a group and finding it within
-- this could instead use self:GetWheelItemsForOpenedFolder as a shortcut but today it shall not
-- group is entirely optional. if it is given, coerce the result into being in that group
function WHEELDATA.FindIndexOfSong(self, song, givenGroup)
    local sortmode = self:GetCurrentSort()
    if sortmode == nil then return 1 end

    local foldername = givenGroup or sortmodeImplementations[sortmode][2](song)
    local folderIndex = self:FindIndexOfFolder(foldername)
    if folderIndex == -1 then return 1 end
    local sdir = song:GetSongDir()

    local songIndex = 0
    for i, s in ipairs(self.AllSongsByFolder[foldername]) do
        if s:GetSongDir() == sdir then
            songIndex = i
            break
        end
    end

    return folderIndex + songIndex
end

-- a very generous helper function to do both of the above functions at once
-- group is entirely optional. if it is given, coerce the result group into being that group
function WHEELDATA.GetWheelItemsAndGroupAndIndexForSong(self, song, givenGroup)
    local sortmode = self:GetCurrentSort()
    
    -- handle this exceptional case just for completeness
    if sortmode == nil then
        local items1, items2 = tablesplit(self.AllFolders, self.AllFolders[1])
        return tableconcat(items1, {self.AllFolders[1]}, items2), self.AllFolders[1], 1
    end

    local index = self:FindIndexOfSong(song, givenGroup)
    local group = givenGroup or sortmodeImplementations[sortmode][2](song)
    local outitems = self:GetWheelItemsForOpenedFolder(group)

    return outitems, group, index
end

-- check to see if a stepstype is countable for average diff reasons
local function countableStepsTypeForDiff(stepstype)
    local thelist = {
        stepstype_dance_single = true,
        stepstype_dance_solo = true,
    }
    -- at this time the calc handles any type of file
    -- uncomment the below line to control what forms the average rating for a pack
    return true
    --return thelist[stepstype:lower()] ~= nil
end

-- get the average difficulty of all valid Steps in a list of Songs
local function getAverageDifficultyOfGroup(group)
    local out = {0,0,0,0,0,0,0,0}
    local chartcount = 0
    for _, song in ipairs(group) do
        for __, chart in ipairs(WHEELDATA:GetChartsMatchingFilter(song)) do
            if countableStepsTypeForDiff(chart:GetStepsType()) then
                chartcount = chartcount + 1
                for i, ___ in ipairs(ms.SkillSets) do
                    out[i] = out[i] + chart:GetMSD(1, i)
                end
            end
        end
    end
    if chartcount > 0 then
        for i, v in ipairs(out) do
            out[i] = v / chartcount
        end
    end
    return out
end

-- calculate the clear stats for a group, all scores within the group
-- does NOT consider rates
local function getClearStatsForGroup(group)
    local out = {
        lamp = nil, -- starting at nil, this lamp may be any Grade_TierXX. if Grade_Tier20, it is simply a Clear lamp
        clearPerGrade = {}, -- count of SONGS CLEARED per grade (the highest common grade for each song)
        totalScores = 0, -- count of SCORES in the whole group
        filesWithScores = 0, -- count of SONGS CLEARED overall
        validFilesOverall = 0, -- count of SONGS THAT ARE ELIGIBLE TO BE CLEARED overall
    }

    local maxlamp = "Grade_Tier01"
    local failed = false -- met the condition to have no grade lamp, but may have a Clear lamp
    for _, song in ipairs(group) do
        local foundgrade = nil -- highest grade of at least rate 1.0
        local foundgradeUnder1 = nil -- highest grade under 1.0
        local highestrateclear = 0 -- highest rate for this song where player obtained C or better
        local useswarps = false
        for __, chart in ipairs(WHEELDATA:GetChartsMatchingFilter(song)) do
            local scorestack = SCOREMAN:GetScoresByKey(chart:GetChartKey())
            useswarps = chart:GetTimingData():HasWarps()

            -- scorestack is nil if no scores on the chart
            -- skip if the chart has negbpms: these scores are always invalid for now and ruin lamps
            if scorestack ~= nil and not useswarps then
                -- the scores are in lists for each rate
                -- find the highest
                for ___, l in pairs(scorestack) do
                    local scoresatrate = l:GetScores()
                    for ____, s in ipairs(scoresatrate) do
                        out.totalScores = out.totalScores + 1
                        local grade = s:GetWifeGrade()
                        local rate = s:GetMusicRate()
                        -- check if this grade is at least a C
                        local gradepasses = compareGrades(grade, "Grade_Tier16")
                        if gradepasses then
                            if rate > highestrateclear then highestrateclear = rate end
                            if rate >= 1 then
                                if foundgrade == nil then
                                    foundgrade = grade
                                else
                                    -- this returns true if score grade is higher than foundgrade
                                    -- (looking for highest grade in this file)
                                    if compareGrades(grade, foundgrade) then
                                        foundgrade = grade
                                    end
                                end
                            else
                                if foundgradeUnder1 == nil then
                                    foundgradeUnder1 = grade
                                else
                                    if compareGrades(grade, foundgradeUnder1) then
                                        foundgradeUnder1 = grade
                                    end
                                end
                            end
                        end
                    end
                end
            end
        end

        -- skip stat intake for negbpm files for now
        if not useswarps then
            -- add this to the count of valid files
            out.validFilesOverall = out.validFilesOverall + 1

            -- no passing (C+) scores found on an entire song means no lamp is possible
            if foundgrade == nil then
                if foundgradeUnder1 == nil then
                    maxlamp = nil
                    failed = true
                else
                    out.filesWithScores = out.filesWithScores + 1
                    if not failed then
                        maxlamp = "Grade_Tier20"
                    end
                    -- count the number of Cleared songs (doesnt matter what grade)
                    if out.clearPerGrade["Grade_Tier20"] ~= nil then
                        out.clearPerGrade["Grade_Tier20"] = out.clearPerGrade["Grade_Tier20"] + 1
                    else
                        out.clearPerGrade["Grade_Tier20"] = 1
                    end
                end
            else
                out.filesWithScores = out.filesWithScores + 1
                -- check if we found a new lowest common max grade
                if not failed then
                    if highestrateclear < 1 then
                        maxlamp = "Grade_Tier20"
                        failed = true
                    elseif compareGrades(maxlamp, foundgrade) then
                        maxlamp = foundgrade
                    end
                end
                -- count the number of songs per grade (1 song may be assigned 1 grade)
                if out.clearPerGrade[foundgrade] ~= nil then
                    out.clearPerGrade[foundgrade] = out.clearPerGrade[foundgrade] + 1
                else
                    out.clearPerGrade[foundgrade] = 1
                end
            end
        end
    end

    if out.validFilesOverall == 0 then maxlamp = nil end
    out.lamp = maxlamp

    return out
end

-- refresh all stats based on the filtered song list
function WHEELDATA.RefreshStats(self)
    self.StatsByFolder = {}
    self.TotalStats = {
        lampCount = 0,
        clearPerGrade = {},
        scoreCount = 0,
    }
    for groupname, songlist in pairs(self.AllSongsByFolder) do
        local clearStats = getClearStatsForGroup(songlist)
        local avgDiff = getAverageDifficultyOfGroup(songlist)
        local lamp = clearStats.lamp
        local scoreCount = clearStats.totalScores
        if lamp ~= nil then
            self.TotalStats.lampCount = self.TotalStats.lampCount + 1
        end
        self.TotalStats.scoreCount = self.TotalStats.scoreCount + scoreCount
        if clearStats.clearPerGrade ~= nil then
            for grade, count in pairs(clearStats.clearPerGrade) do
                local curcount = self.TotalStats.clearPerGrade[grade]
                if curcount ~= nil then
                    self.TotalStats.clearPerGrade[grade] = curcount + count
                else
                    self.TotalStats.clearPerGrade[grade] = 1
                end
            end
        end

        self.StatsByFolder[groupname] = {
            count = #songlist,
            avgDiff = avgDiff,
            clearStats = clearStats,
        }
    end
end

-- getter for the total score count overall
-- redundant with the SCOREMAN function but can be useful
function WHEELDATA.GetTotalScoreCount(self)
    if self.TotalStats.scoreCount ~= nil then
        return self.TotalStats.scoreCount
    end
    return 0
end

-- getter for the total clear count for each grade
function WHEELDATA.GetTotalClearsByGrade(self, grade)
    if self.TotalStats.clearPerGrade == nil then return 0 end
    if self.TotalStats.clearPerGrade[grade] ~= nil then
        return self.TotalStats.clearPerGrade[grade]
    end
    return 0
end

-- getter for the total lamp count
function WHEELDATA.GetTotalLampCount(self)
    if self.TotalStats.lampCount ~= nil then
        return self.TotalStats.lampCount
    end
    return 0
end

-- getter for the folder count stat
function WHEELDATA.GetFolderCount(self, name)
    if self.StatsByFolder[name] ~= nil then
        return self.StatsByFolder[name].count
    end
    return 0
end

-- getter for the folder average diff stat
function WHEELDATA.GetFolderAverageDifficulty(self, name)
    if self.StatsByFolder[name] ~= nil then
        return self.StatsByFolder[name].avgDiff
    end
    return {0,0,0,0,0,0,0,0}
end

-- getter for the folder clear stats (lamps, score info)
function WHEELDATA.GetFolderClearStats(self, name)
    if self.StatsByFolder[name] ~= nil then
        return self.StatsByFolder[name].clearStats
    end
    return {lamp = nil,clearPerGrade = {},totalScores = 0,filesWithScores = 0,validFilesOverall = 0,}
end

-- sort all songs again basically
-- requires that AllSongs is set
function WHEELDATA.UpdateFilteredSonglist(self)
    self:SortByCurrentSortmode()
    self:RefreshStats()
end

-- basically the init function but doesnt reset the sortmode
function WHEELDATA.ReloadWheelData(self)
    -- reloads all the songs (the c++ filter is taken into account here)
    -- also reloads the folders
    self:SetAllSongs()
    -- recalculate folder stats
    self:RefreshStats()
    -- build the wheel data
    self:SetWheelItems(self:GetFilteredFolders())
end

-- init all with default values
-- group sort default
-- etc
function WHEELDATA.Init(self, forceReset)
    -- reset will handle setting sortmode to Group
    -- dont reset every time we init though
    -- this is run at the start of SelectMusic
    -- the end result is the filter is kept and the wheel data is kept
    if not firstTimeInit or forceReset then
        firstTimeInit = true
        self:Reset()
    else
        -- people apparently dont like search not resetting
        -- thats the only thing we will reset then
        self:ResetActiveFilterMetadata()
    end

    -- this will fill AllSongs and the SongsByGroup
    self:SetAllSongs()
    self:RefreshStats()
    -- update wheel items
    self:SetWheelItems(self:GetFilteredFolders())
end