-- like a singleton
-- this manages all the data for the musicwheel
-- the purpose of it is to essentially cache wheel related things and sorts
-- we don't have to keep resorting the wheel every time we open a pack if this is correct
-- etc

WHEELDATA = {}

-- quickly reset the WHEELDATA storage
function WHEELDATA.Reset(self)
    -- 1 is the "enum value" for Group sort, it should stay that way
    self.CurrentSort = 1

    -- library of all Songs for this Game (all Styles)
    self.AllSongs = {}
    self.AllSongsByGroup = {}
    self.AllGroups = {}

    -- for the current sort, filtering and organization purposes
    self.AllSongsByFolder = {} -- multipurpose; on Group sort, is identical to AllSongsByGroup
    self.AllFolders = {} -- this can be groups or "folders" like in the Title sort, etc
    self.StatsByFolder = {} -- stats for each folder
end

-- quickly empty the sorted lists
function WHEELDATA.ResetSorts(self)
    self.AllFolders = {}
    self.AllSongsByFolder = {}
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
            -- sort each group
            for groupname, songlist in pairs(WHEELDATA.AllSongsByGroup) do
                WHEELDATA.AllSongsByFolder[groupname] = songlist
                table.sort(
                    WHEELDATA.AllSongsByFolder[groupname],
                    SongUtil.SongTitleComparator
                )
                WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = groupname
            end
            -- sort the groups
            table.sort(
                WHEELDATA.AllFolders,
                function(a,b) return a:lower() < b:lower() end
            )
        end,
        function(song)
            return song:GetGroupName()
        end,
    },

    {   -- "Title" sort -- by song title, alphabetical within
        function()
            WHEELDATA:ResetSorts()

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(WHEELDATA.AllSongs) do
                local foldername = getTitleSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[foldername] ~= nil then
                    WHEELDATA.AllSongsByFolder[foldername][#WHEELDATA.AllSongsByFolder[foldername] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[foldername] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = foldername
                end
            end
            -- sort groups and then songlists in groups
            table.sort(WHEELDATA.AllFolders)
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

            -- go through AllSongs and construct it as we go, then sort
            for _, song in ipairs(WHEELDATA.AllSongs) do
                local fname = getAuthorSortFoldernameForSong(song)
                if WHEELDATA.AllSongsByFolder[fname] ~= nil then
                    WHEELDATA.AllSongsByFolder[fname][#WHEELDATA.AllSongsByFolder[fname] + 1] = song
                else
                    WHEELDATA.AllSongsByFolder[fname] = {song}
                    WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = fname
                end
            end
            -- sort groups and then songlists in groups
            table.sort(WHEELDATA.AllFolders)
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

function WHEELDATA.SortByCurrentSortmode(self)
    local sortval, sortstr = self:GetCurrentSort()
    -- if sort is invalid, make sure the output is empty and exit
    if sortval == nil then self:ResetSorts() return end

    -- run the sort function based on the sortmode
    local tbefore = GetTimeSinceStart()
    local f = sortmodeImplementations[sortval][1]
    f()
    local tafter = GetTimeSinceStart()

    -- sort timing debug
    --ms.ok(string.format("WHEELDATA -- Sorting took %f.", tafter - tbefore))
end

-- update the song list for the current gamemode
-- sort all songs by current sort
function WHEELDATA.SetAllSongs(self)
    self.AllSongs = {}
    self.AllSongsByGroup = {}
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
        for __, chart in ipairs(song:GetChartsOfCurrentGameMode()) do
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
        lamp = nil, -- starting at nil, this lamp may be any Grade_TierXX (consider midgrades)
        clearPerGrade = {}, -- count of SONGS CLEARED per grade
        totalScores = 0, -- count of SCORES in the whole group
    }

    local maxlamp = "Grade_Tier01"
    local failed = false
    for _, song in ipairs(group) do
        local foundgrade = nil
        -- probably need to replace this getter with something more filtered
        for __, chart in ipairs(song:GetChartsOfCurrentGameMode()) do
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
                        if foundgrade == nil then
                            foundgrade = grade
                        else
                            if compareGrades(grade, foundgrade) then
                                foundgrade = grade
                            end
                        end
                    end
                end
            end
        end

        -- no scores found on an entire song means no clear lamp is possible
        if foundgrade == nil then
            maxlamp = nil
            failed = true
        else
            if not failed and compareGrades(maxlamp, foundgrade) then
                maxlamp = foundgrade
            end
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

-- init all with default values
-- group sort default
-- etc
function WHEELDATA.Init(self)
    -- reset will handle setting sortmode to Group
    self:Reset()
    -- this will fill AllSongs and the SongsByGroup
    self:SetAllSongs()
    self:RefreshStats()
end