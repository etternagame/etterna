-- like a singleton
-- this manages all the data for the musicwheel
-- the purpose of it is to essentially cache wheel related things and sorts
-- we don't have to keep resorting the wheel every time we open a pack if this is correct
-- etc

WHEELDATA = {
    AllSongs = {},
    AllSongsByGroup = {},
    -- for the current sort, filtering and organization purposes
    AllSongsByFolder = {}, -- multipurpose; on Group sort, is identical to AllSongsByGroup
    AllFolders = {}, -- this can be groups or "folders" like in the Title sort, etc
    CurrentSort = 1, -- 1 is the "enum value" for Group sort, it should stay that way
}

-- quickly empty the sorted lists
function WHEELDATA:ResetSorts(self)
    self.AllFolders = {}
    self.AllSongsByFolder = {}
end

local sortmodes = {
    "Group", -- group by pack, all "alphabetical order"
    "Title", -- group by title letter
}
local function sortToString(val)
    return sortmodes[val]
end

-- mimicing SongUtil::MakeSortString here to keep behavior consistent
local function makeSortString(s)
    local st = s:upper()
    if #st > 0 then
        if st:find(".") == 1 then
            st = string.sub(st, 2)
        end
        if st:find("#") == 1 then
            return st
        end

        local fchar = string.byte(st)
        if (fchar < string.byte("A") or fchar > string.byte("Z")) and (fchar < string.byte("0") or fchar > string.byte("9")) then
            st = string.char(126) .. st
        end
    end
    return st
end

-- functions responsible for actually sorting things according to the sortmodes table
local sortmodeImplementations = {
    function() -- "Group" sort -- by pack, alphabetical
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
    function() -- "Title" sort -- by song title, alphabetical
        WHEELDATA:ResetSorts()

        local function getFoldernameForSong(song)
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
            return string.sub(st, 1, 1)
        end

        -- go through AllSongs and construct it as we go, then sort
        for _, song in ipairs(WHEELDATA.AllSongs) do
            local foldername = getFoldernameForSong(song)
            if WHEELDATA.AllSongsByFolder[foldername] ~= nil then
                WHEELDATA.AllSongsByFolder[foldername][#WHEELDATA.AllSongsByFolder[foldername] + 1] = song
            else
                WHEELDATA.AllSongsByFolder[foldername] = {song}
                WHEELDATA.AllFolders[#WHEELDATA.AllFolders + 1] = foldername
            end
        end
        table.sort(WHEELDATA.AllFolders)
        for _, songlist in pairs(WHEELDATA.AllSongsByFolder) do
            table.sort(
                songlist,
                SongUtil.SongTitleComparator
            )
        end
    end,
}

-- get the value and string value of the current sort
function WHEELDATA:GetCurrentSort(self)
    return self.CurrentSort, sortToString(self.CurrentSort)
end

function WHEELDATA:SortByCurrentSortmode(self)
    local sortval, sortstr = self:GetCurrentSort()
    -- if sort is invalid, make sure the output is empty and exit
    if sortval == nil then self:ResetSorts() return end

    -- run the sort function based on the sortmode
    local tbefore = GetTimeSinceStart()
    local f = sortmodeImplementations[sortval]
    f()
    local tafter = GetTimeSinceStart()

    ms.ok(string.format("WHEELDATA -- Sorting took %f.", tafter - tbefore))
end

-- update the song list for the current gamemode
-- sort all songs by current sort
function WHEELDATA:GetAllSongs(self)
    self.AllSongs = {}
    self.AllSongsByGroup = {}
    for _, song in ipairs(SONGMAN:GetAllSongs()) do
        if #song:GetChartsOfCurrentGameMode() > 0 then
            self.AllSongs[#self.AllSongs+1] = song
            local group = song:GetGroupName()
            self.AllSongsByGroup[group][#self.AllSongsByGroup[group]+1] = song
        end
    end
    self:SortByCurrentSortmode()
end

-- refresh all stats based on the filtered song list
function WHEELDATA:RefreshStats(self)


end
