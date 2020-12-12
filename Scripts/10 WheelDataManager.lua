-- like a singleton
-- this manages all the data for the musicwheel
-- the purpose of it is to essentially cache wheel related things and sorts
-- we don't have to keep resorting the wheel every time we open a pack if this is correct
-- etc

WHEELDATA = {}

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

    -- filter info
    self.ActiveFilter = {
        metadata = {    -- search metadata to filter by; filters out all non matches
            Title = "",
            Subtitle = "",
            Artist = "",
            Author = "",
        },
        valid = nil, -- a song or chart which passes this function is ACCEPTED
    }

    -- last generated list of WheelItems
    self.WheelItems = {}
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

-- check if the filter is active
function WHEELDATA.IsFiltered(self)
    return self.ActiveFilter ~= nil and (self.ActiveFilter.valid ~= nil or not self:IsSearchFilterEmpty()) or FILTERMAN:AnyActiveFilter()
end

-- get the current search table
function WHEELDATA.GetSearch(self)
    return self.ActiveFilter.metadata
end

-- set the search table for filtering
-- does not actually sort the songs, trigger that with WHEELDATA:SortByCurrentSortmode()
function WHEELDATA.SetSearch(self, t)
    if s == nil or s:gsub("^%s*(.-)%s*$", "%1") == "" then
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
    end
end

-- give a song, chartkey, or steps
-- works as AND for now, where if both search and valid are set then both have to pass
-- returns true if filter passed
function WHEELDATA.FilterCheck(self, g)
    if not self:IsFiltered() then return true end
    local passed = true

    -- todo: although this function almost only runs on Songs, needs to be generalized to work with all of these
    if type(g) == "string" then
        -- working with a Chartkey

        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(g) then
                return false
            end
        end
    elseif g.GetAllSteps then
        -- working with a Song
        -- song search
        local title = g:GetDisplayMainTitle():lower()
        local author = g:GetOrTryAtLeastToGetSimfileAuthor():lower()
        local artist = g:GetDisplayArtist():lower()
        local subtitle = g:GetDisplaySubTitle():lower()
        if not self:IsSearchFilterEmpty() then
            if self.ActiveFilter.metadata.Title ~= "" then
               if title:find(self.ActiveFilter.metadata.Title) == nil then return false end
            end
            if self.ActiveFilter.metadata.Subtitle ~= "" then
                if subtitle:find(self.ActiveFilter.metadata.Subtitle) == nil then return false end
            end
            if self.ActiveFilter.metadata.Author ~= "" then
                if author:find(self.ActiveFilter.metadata.Author) == nil then return false end
            end
            if self.ActiveFilter.metadata.Artist ~= "" then
                if artist:find(self.ActiveFilter.metadata.Artist) == nil then return false end
            end
        end
        -- lua filter
        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(g) then
                return false
            end
        end

        -- c++ filter
        local charts = g:GetChartsMatchingFilter()
        if #charts == 0 then return false end

    elseif g.GetChartKey then
        -- working with a Steps

        -- lua filter
        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(g) then
                return false
            end
        end

        -- c++ filter
        local ck = g:GetChartKey()
        local s = SONGMAN:GetSongByChartkey(ck)
        if s ~= nil then
            local tmpbool = false
            local charts = s:GetChartsMatchingFilter()
            for _,c in ipairs(charts) do
                if c:GetChartKey() == ck then
                    tmpbool = true
                end
            end
            return tmpbool and passed
        end
    end

    return passed
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
function WHEELDATA.GetChartsMatchingFilter(self, song)
    local charts = song:GetChartsMatchingFilter()
    local t = {}
    for i, c in ipairs(charts) do
        if self.ActiveFilter.valid ~= nil then
            if not self.ActiveFilter.valid(c) then
                -- failed to pass
            else
                -- passed
                t[#t+1] = c
            end
        else
            -- passed implicitly
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
end

local sortmodes = {
    "Group", -- group by pack, all "alphabetical order"
    "Title", -- group by title letter
    "Author", -- group by chartist
}
local function sortToString(val)
    return sortmodes[val]
end

-- mimicing SongUtil::MakeSortString here to keep behavior consistent
local function makeSortString(s)
    local st = s:upper()
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
    title = makeSortString(title)
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
-- gets the author of a file, for foldername purposes
local function getAuthorSortFoldernameForSong(song)
    local author = song:GetOrTryAtLeastToGetSimfileAuthor()
    if author:upper() == "AUTHOR UNKNOWN" then author = "??????" end
    return author
end

-- functions responsible for actually sorting things according to the sortmodes table
-- each member is a table of functions
-- the first function modifies WHEELDATA by sorting its items
-- the second function describes the method used to find the folder that a song is in
local sortmodeImplementations = {
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
        end
    },
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
function WHEELDATA.FindIndexOfSong(self, song)
    local sortmode = self:GetCurrentSort()
    if sortmode == nil then return 1 end

    local foldername = sortmodeImplementations[sortmode][2](song)
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
function WHEELDATA.GetWheelItemsAndGroupAndIndexForSong(self, song)
    local sortmode = self:GetCurrentSort()
    
    -- handle this exceptional case just for completeness
    if sortmode == nil then
        local items1, items2 = tablesplit(self.AllFolders, self.AllFolders[1])
        return tableconcat(items1, {self.AllFolders[1]}, items2), self.AllFolders[1], 1
    end

    local group = sortmodeImplementations[sortmode][2](song)
    local index = self:FindIndexOfSong(song)
    local outitems = self:GetWheelItemsForOpenedFolder(group)

    return outitems, group, index
end

-- check to see if a stepstype is countable for average diff reasons
local function countableStepsTypeForDiff(stepstype)
    local thelist = {
        stepstype_dance_single = true,
        stepstype_dance_solo = true,
    }
    return thelist[stepstype:lower()] ~= nil
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
    }

    local maxlamp = "Grade_Tier01"
    local failed = false -- met the condition to have no grade lamp, but may have a Clear lamp
    for _, song in ipairs(group) do
        local foundgrade = nil -- highest grade of at least rate 1.0
        local foundgradeUnder1 = nil -- highest grade under 1.0
        local highestrateclear = 0 -- highest rate for this song where player obtained C or better
        for __, chart in ipairs(WHEELDATA:GetChartsMatchingFilter(song)) do
            local scorestack = SCOREMAN:GetScoresByKey(chart:GetChartKey())

            -- scorestack is nil if no scores on the chart
            if scorestack ~= nil then
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

        -- no passing (C+) scores found on an entire song means no lamp is possible
        if foundgrade == nil then
            if foundgradeUnder1 == nil then
                maxlamp = nil
                failed = true
            else
                maxlamp = "Grade_Tier20"
                -- count the number of Cleared songs (doesnt matter what grade)
                if out.clearPerGrade[maxlamp] ~= nil then
                    out.clearPerGrade[maxlamp] = out.clearPerGrade[maxlamp] + 1
                else
                    out.clearPerGrade[maxlamp] = 1
                end
            end
        else
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
    out.lamp = maxlamp

    return out
end

-- refresh all stats based on the filtered song list
function WHEELDATA.RefreshStats(self)
    self.StatsByFolder = {}
    for groupname, songlist in pairs(self.AllSongsByFolder) do
        self.StatsByFolder[groupname] = {
            count = #songlist,
            avgDiff = getAverageDifficultyOfGroup(songlist),
            clearStats = getClearStatsForGroup(songlist),
        }
    end
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
    return {nil, {}, 0}
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
function WHEELDATA.Init(self)
    -- reset will handle setting sortmode to Group
    self:Reset()
    -- this will fill AllSongs and the SongsByGroup
    self:SetAllSongs()
    self:RefreshStats()
    -- update wheel items
    self:SetWheelItems(self:GetFilteredFolders())
end