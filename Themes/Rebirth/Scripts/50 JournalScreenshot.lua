-- Journal: screenshot auto-attach.
-- Hooks `ScreenshotSaved` broadcast (from StepMania::SaveScreenshot in C++).
-- Copies the image into <Root>/screenshots/ with a collision-free name and
-- appends a `[[path]]` link to the current song's today-entry.
-- Spec: Docs/Journal.md §7.

JOURNAL = JOURNAL or {}
JOURNAL.screenshot = {}

---------------------------------------------------------------
-- Resolve the absolute on-disk source path of a just-saved screenshot.
-- C++ Path parameter is VFS-relative, e.g. "Screenshots/screen001.png".
-- We need to resolve it to the OS path. RageFileManager exposes
-- ResolvePath via MemoryDriver but not directly to Lua; instead we use
-- PROFILEMAN or the documented convention: Save/<Path>.

local function resolveSourcePath(path)
    if not path or path == "" then return nil end
    -- Etterna root is usually Etterna install dir OR Save dir. Screenshots
    -- always go under Save/. PROFILEMAN:GetStatsPrefix() exposes Save dir root.
    local saveDir = nil
    if PROFILEMAN and PROFILEMAN.GetStatsPrefix then
        saveDir = PROFILEMAN:GetStatsPrefix()  -- not ideal, but often usable
    end
    -- Fallback: use relative; io.open called from Etterna cwd will find "Save/..."
    -- prefix.
    return "Save/" .. path
end

---------------------------------------------------------------
-- Main handler

function JOURNAL.screenshot.handle(fileName, path)
    if not JOURNAL.isEnabled() then return end
    if not GAMESTATE then return end
    local song = GAMESTATE:GetCurrentSong()
    if not song then
        -- No song context (e.g. title screen) — keep original, no attach.
        return
    end
    if not JOURNAL.ensureLayout() then return end

    local pack = song.GetGroupName and song:GetGroupName() or "Unknown"
    local songDir = song.GetSongDir and song:GetSongDir() or ""
    songDir = songDir:gsub("[/\\]$", "")
    local seg = songDir:match("([^/\\]+)$") or songDir
    local title = (song.GetDisplayMainTitle and song:GetDisplayMainTitle()) or
                  (song.GetMainTitle and song:GetMainTitle()) or seg

    -- ext from fileName
    local ext = fileName:match("%.([%w]+)$") or "png"

    -- Compute source path. C++ returns FileName (basename) + Path (VFS). The
    -- simplest reliable approach: read via io.open relative to cwd.
    local src = resolveSourcePath(path)
    if not src or not JOURNAL.fileExists(src) then
        JOURNAL.log("screenshot source not found: " .. tostring(src))
        return
    end

    local dstName = JOURNAL.screenshotName(pack, seg, ext)
    local dstAbs = JOURNAL.joinPath(JOURNAL.screenshotRoot(), dstName)
    if not JOURNAL.copyBinary(src, dstAbs) then
        JOURNAL.log("screenshot copy failed to " .. dstAbs)
        return
    end

    local relPath = JOURNAL.cfg().screenshotDir .. "/" .. dstName

    -- Append link to today's memo
    local mdPath = JOURNAL.packMarkdownPath(pack)
    local existing = JOURNAL.readText(mdPath) or ""
    local doc = JOURNAL.md.parse(existing)
    local idx = JOURNAL.md.indexByTitle(doc)[title]
    local section
    if idx then
        section = doc.sections[idx]
    else
        local banner = JOURNAL.banner.copyFor(song, pack, seg)
        local tags = JOURNAL.tags.get(pack, seg)
        section = JOURNAL.md.newSection({ title = title, bannerPath = banner, tags = tags })
        doc.sections[#doc.sections + 1] = section
    end
    JOURNAL.md.attachScreenshot(section, relPath)
    JOURNAL.writeText(mdPath, JOURNAL.md.serialize(doc))
    JOURNAL.log("attached screenshot to " .. title)
end
