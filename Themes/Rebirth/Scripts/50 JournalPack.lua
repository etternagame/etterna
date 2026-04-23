-- Journal: top-level pack markdown generator (ties banner + tags + markdown)
-- Spec: Docs/Journal.md §3, §4.

JOURNAL = JOURNAL or {}
JOURNAL.pack = {}

---------------------------------------------------------------
-- Get Song's directory name (last path segment of song dir, stable identifier).

local function songDirName(song)
    if not song or not song.GetSongDir then return "_" end
    local d = song:GetSongDir()
    if not d or d == "" then return "_" end
    -- strip trailing slash
    d = d:gsub("[/\\]$", "")
    -- take last segment
    local seg = d:match("([^/\\]+)$")
    return seg or d
end

---------------------------------------------------------------
-- Display title for heading. Falls back gracefully.

local function songTitle(song)
    if not song then return "" end
    if song.GetDisplayMainTitle then
        local t = song:GetDisplayMainTitle()
        if t and t ~= "" then return t end
    end
    if song.GetMainTitle then
        local t = song:GetMainTitle()
        if t and t ~= "" then return t end
    end
    return songDirName(song)
end

---------------------------------------------------------------
-- Group songs by group (pack) name.
-- Returns map: packName -> { [songDirName] = songObj, ... }

local function collectByPack()
    local map = {}
    if not SONGMAN or not SONGMAN.GetAllSongs then return map end
    local all = SONGMAN:GetAllSongs()
    for _, s in ipairs(all) do
        local grp = s.GetGroupName and s:GetGroupName() or "Unknown"
        map[grp] = map[grp] or {}
        map[grp][songDirName(s)] = s
    end
    return map
end
JOURNAL.pack.collectByPack = collectByPack

---------------------------------------------------------------
-- Regenerate / update .md for a single pack. Preserves user memos.
-- songs = { [songDirName] = songObj, ... }
-- Returns (newSectionCount, updatedSectionCount).

function JOURNAL.pack.regenerate(packName, songs)
    local path = JOURNAL.packMarkdownPath(packName)
    local existing = JOURNAL.readText(path) or ""
    local doc = JOURNAL.md.parse(existing)
    local byTitle = JOURNAL.md.indexByTitle(doc)

    local added, updated = 0, 0

    -- Stable order: sort song dir names
    local dirs = {}
    for k in pairs(songs) do dirs[#dirs + 1] = k end
    table.sort(dirs)

    for _, dir in ipairs(dirs) do
        local song = songs[dir]
        local title = songTitle(song)
        local bannerRel = JOURNAL.banner.copyFor(song, packName, dir)
        local tags = JOURNAL.tags.get(packName, dir)

        local idx = byTitle[title]
        if idx then
            -- Existing section: sync managed lines only.
            JOURNAL.md.syncManagedLines(doc.sections[idx], bannerRel, tags)
            updated = updated + 1
        else
            -- New section: append fresh template.
            local section = JOURNAL.md.newSection({
                title = title,
                bannerPath = bannerRel,
                tags = tags,
            })
            doc.sections[#doc.sections + 1] = section
            byTitle[title] = #doc.sections
            added = added + 1
        end
    end

    local text = JOURNAL.md.serialize(doc)
    JOURNAL.writeText(path, text)
    return added, updated
end

---------------------------------------------------------------
-- Full regenerate across all packs. Called on DFRFinished.

function JOURNAL.pack.regenerateAll()
    if not JOURNAL.isEnabled() then return end
    if not JOURNAL.ensureLayout() then
        JOURNAL.log("ensureLayout failed; aborting regenerateAll")
        return
    end
    JOURNAL.tags.load()

    local t0 = GetTimeSinceStart and GetTimeSinceStart() or 0
    local byPack = collectByPack()
    local packCount, addedTotal, updatedTotal = 0, 0, 0
    for packName, songs in pairs(byPack) do
        local a, u = JOURNAL.pack.regenerate(packName, songs)
        packCount = packCount + 1
        addedTotal = addedTotal + a
        updatedTotal = updatedTotal + u
    end
    local t1 = GetTimeSinceStart and GetTimeSinceStart() or 0
    JOURNAL.log(string.format(
        "regenerateAll: %d packs, +%d new sections, %d updated (%.2fs)",
        packCount, addedTotal, updatedTotal, t1 - t0
    ))
end

---------------------------------------------------------------
-- Register MessageManager hook: regenerate on DFRFinished (and once on boot).

local function registerHook()
    if not MESSAGEMAN then return end
    -- MessageCommand-style handlers expect an actor; we use a global listener.
    -- The ScreenSelectMusic overlay will also trigger manual regen on demand.
end

registerHook()
