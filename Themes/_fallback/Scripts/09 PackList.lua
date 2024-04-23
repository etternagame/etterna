--- Pack download list sugar
-- Basically this extends the existing DownloadManager functionality
-- @module 09_PackList

PackList = {}

local function whenRequestFinished(self)
    return function(packs)
        self.packs = packs
        MESSAGEMAN:Broadcast("PackListRequestFinished", {packList=self, packs=packs})
    end
end

-- get the current page of packs
function PackList:GetPacks()
    if self:IsAwaitingRequest() then return {} end
    return self.packs
end

-- get all cached packs
function PackList:GetCachedPacks()
    return self.currentPagination:GetCachedResults()
end

-- is the pack list currently loading?
function PackList:IsAwaitingRequest()
    return self.currentPagination:IsAwaitingRequest()
end

-- move to next page. request packs if needed.
function PackList:NextPage()
    if self:IsAwaitingRequest() then
        return false
    end
    self.currentPagination:NextPage(whenRequestFinished(self))
    return true
end

-- move to prev page. request packs if needed
function PackList:PrevPage()
    if self:IsAwaitingRequest() then
        return false
    end
    self.currentPagination:PrevPage(whenRequestFinished(self))
    return true
end

-- current page number for current search
function PackList:GetCurrentPage()
    if self.currentPagination == nil then return 0 end
    return self.currentPagination:GetCurrentPage()
end

-- max page number for current search
function PackList:GetTotalPages()
    if self.currentPagination == nil then return 0 end
    return self.currentPagination:GetTotalPages()
end

function PackList:GetTotalResults()
    if self.currentPagination == nil then return 0 end
    return self.currentPagination:GetTotalResults()
end

-- execute a seach. usually this invokes a request unless it is a duplicate
function PackList:FilterAndSearch(name, tags, perPage)
    self.lastName = name
    self.lastTags = tags
    self.lastPerPage = perPage
    self.currentPagination = DLMAN:GetPackPagination(name, tags, perPage, self.sortColumn, self.sortIsAscending)

    if not self:IsAwaitingRequest() then
        self.currentPagination:GetResults(whenRequestFinished(self))
    end

    return self
end

function PackList:SortByColumn(column)
    if column == self.sortColumn then
        self.sortIsAscending = not self.sortIsAscending
    else
        self.sortColumn = column
        self.sortIsAscending = true
    end
    self:FilterAndSearch(self.lastName, self.lastTags, self.lastPerPage)
end

function PackList:SortByName()
    self:SortByColumn("name")
end

function PackList:SortByPlays()
    self:SortByColumn("play_count")
end

function PackList:SortBySize()
    self:SortByColumn("bytes")
end

function PackList:SortBySongs()
    self:SortByColumn("song_count")
end

function PackList:SortByOverall()
    self:SortByColumn("overall")
end

function PackList:new()
    local packlist = {}
    packlist.packs = {} -- represents the packs on the current visible page
    packlist.currentPagination = nil -- represents the internal pack search pagination
    packlist.sortColumn = "name"
    packlist.sortIsAscending = true
    packlist.lastName = ""
    packlist.lastTags = {}
    packlist.lasPerPage = 1
    setmetatable(
        packlist,
        {
            __index = function(t, i)
                return t.packs[i] or PackList[i]
            end
        }
    )
    return packlist
end
