-- Journal: banner image copy + naming
-- Spec: Docs/Journal.md §2 (filename rule), §7 (banner copy on pack reload).

JOURNAL = JOURNAL or {}
JOURNAL.banner = {}

---------------------------------------------------------------
-- Parse extension from #BANNER path (lowercase). Default to "png" if ambiguous.
local function extOf(path)
    if not path or path == "" then return "png" end
    local ext = path:match("%.([%w]+)$")
    if not ext then return "png" end
    return ext:lower()
end

---------------------------------------------------------------
-- Resolve Song object's banner source path on disk.
-- Song:GetBannerPath() returns either absolute or relative-to-song-dir path.
-- Returns nil if no banner available.
local function resolveBannerSrc(song)
    if not song then return nil end
    local fn = nil
    if song.GetBannerPath then
        fn = song:GetBannerPath()
    end
    if fn == nil or fn == "" then
        if song.GetBackgroundPath then
            fn = song:GetBackgroundPath()
        end
    end
    if fn == nil or fn == "" then return nil end
    return fn
end

---------------------------------------------------------------
-- Copy banner for a single song.
-- Returns relative path (relative to JournalRoot) like "banners/Pack__Song.png"
-- or nil if no banner / copy failed.

function JOURNAL.banner.copyFor(song, packName, songDir)
    local src = resolveBannerSrc(song)
    if not src then return nil end
    local ext = extOf(src)
    local name = JOURNAL.bannerName(packName, songDir, ext)
    local dst = JOURNAL.joinPath(JOURNAL.bannerRoot(), name)
    local relPath = JOURNAL.cfg().bannerDir .. "/" .. name

    -- Skip copy if dst already exists and size matches (cheap check).
    if JOURNAL.fileExists(dst) then
        return relPath
    end

    local ok = JOURNAL.copyBinary(src, dst)
    if not ok then
        JOURNAL.log("banner copy failed: " .. src .. " -> " .. dst)
        return nil
    end
    return relPath
end
