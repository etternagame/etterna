-- Journal: markdown parse / merge / write
-- Spec: Docs/Journal.md §3 (format), §4 (merge strategy).

JOURNAL = JOURNAL or {}
JOURNAL.md = {}

---------------------------------------------------------------
-- Section model:
--   A section = one song entry.
--   - heading   : "# <title>"  (line starting with `#` followed by space)
--   - bannerLn  : auto-managed, pattern `^%!%[%[.*banners/.*%]%]`
--   - tagLine   : auto-managed, line that starts with `#Etterna/` (or the user's tag prefix)
--   - body      : everything else (memos, screenshots, user heading, etc.) — untouched
--
-- Non-section prefix (lines before the first `# ` heading) = preserved as-is.

local BANNER_LINE_PATTERN = "^%!%[%[.-banners/"        -- `![[.../banners/...]]`
local TAG_LINE_PATTERN    = "^#Etterna[/%w]"           -- `#Etterna/...`
local HEADING_PATTERN     = "^# (.+)$"                 -- top-level heading only

---------------------------------------------------------------
-- Parse .md text into { preface, sections = [{title, lines}, ...] }

function JOURNAL.md.parse(text)
    local doc = { preface = {}, sections = {} }
    if not text or text == "" then return doc end

    local current = nil
    for line in (text .. "\n"):gmatch("([^\n]*)\n") do
        local title = line:match(HEADING_PATTERN)
        if title then
            current = { title = title, lines = { line } }
            doc.sections[#doc.sections + 1] = current
        elseif current then
            current.lines[#current.lines + 1] = line
        else
            doc.preface[#doc.preface + 1] = line
        end
    end
    return doc
end

---------------------------------------------------------------
-- Within a section's `lines` array, find index of banner / tag lines.
-- Returns (bannerIdx or nil, tagIdx or nil). Search only first ~8 lines
-- to avoid matching user's own `#` tags buried deep in memo body.

local function locateManagedLines(lines)
    local bannerIdx, tagIdx = nil, nil
    local limit = math.min(#lines, 8)
    for i = 2, limit do  -- skip heading at [1]
        local l = lines[i]
        if not bannerIdx and l:match(BANNER_LINE_PATTERN) then
            bannerIdx = i
        elseif not tagIdx and l:match(TAG_LINE_PATTERN) then
            tagIdx = i
        end
    end
    return bannerIdx, tagIdx
end

---------------------------------------------------------------
-- Build a fresh section for a newly-seen song.
-- info = { title, bannerPath or nil, tags = {} or nil }

function JOURNAL.md.newSection(info)
    local lines = { "# " .. info.title, "" }
    if info.bannerPath and info.bannerPath ~= "" then
        lines[#lines + 1] = "![[" .. info.bannerPath .. "|128]]"
        lines[#lines + 1] = ""
    end
    if info.tags and #info.tags > 0 then
        lines[#lines + 1] = JOURNAL.md.formatTagLine(info.tags)
        lines[#lines + 1] = ""
    end
    return { title = info.title, lines = lines }
end

---------------------------------------------------------------
-- Format tag list as single space-separated line.
function JOURNAL.md.formatTagLine(tags)
    local parts = {}
    for _, t in ipairs(tags) do
        parts[#parts + 1] = "#" .. t
    end
    return table.concat(parts, " ")
end

---------------------------------------------------------------
-- Sync managed lines in an existing section.
-- bannerPath / tags may be nil (leave unchanged).
function JOURNAL.md.syncManagedLines(section, bannerPath, tags)
    local bannerIdx, tagIdx = locateManagedLines(section.lines)

    if bannerPath and bannerPath ~= "" then
        local newLn = "![[" .. bannerPath .. "|128]]"
        if bannerIdx then
            section.lines[bannerIdx] = newLn
        else
            -- insert after heading + blank
            table.insert(section.lines, 2, "")
            table.insert(section.lines, 2, newLn)
        end
    end

    if tags then
        local newLn = JOURNAL.md.formatTagLine(tags)
        -- tag line is intentionally empty if no tags — keep user's manual tags out of our scope
        if newLn == "" then
            if tagIdx then
                table.remove(section.lines, tagIdx)
            end
        else
            if tagIdx then
                section.lines[tagIdx] = newLn
            else
                -- insert after banner area (or after heading if no banner)
                local insertAt = (bannerIdx or 1) + 1
                -- skip trailing blank
                while section.lines[insertAt] == "" and insertAt < #section.lines do
                    insertAt = insertAt + 1
                end
                table.insert(section.lines, insertAt, newLn)
                table.insert(section.lines, insertAt + 1, "")
            end
        end
    end
end

---------------------------------------------------------------
-- Serialize doc back to text.

function JOURNAL.md.serialize(doc)
    local buf = {}
    for _, l in ipairs(doc.preface) do buf[#buf + 1] = l end
    for _, s in ipairs(doc.sections) do
        for _, l in ipairs(s.lines) do buf[#buf + 1] = l end
    end
    -- trim duplicate trailing blanks
    while #buf > 0 and buf[#buf] == "" do buf[#buf] = nil end
    buf[#buf + 1] = ""  -- single trailing newline
    return table.concat(buf, "\n")
end

---------------------------------------------------------------
-- Index sections by title for fast lookup
function JOURNAL.md.indexByTitle(doc)
    local idx = {}
    for i, s in ipairs(doc.sections) do idx[s.title] = i end
    return idx
end

---------------------------------------------------------------
-- Append a dated memo line to the given section.
-- If today's entry already exists, append to end of today's block.
-- Otherwise append `* YYYY-MM-DD <text>` at end of section.
function JOURNAL.md.appendMemo(section, text)
    local today = JOURNAL.today()
    local line = "* " .. today .. " " .. text
    -- find last blank then append
    local insertAt = #section.lines
    while insertAt > 1 and section.lines[insertAt] == "" do
        insertAt = insertAt - 1
    end
    table.insert(section.lines, insertAt + 1, line)
    table.insert(section.lines, insertAt + 2, "")
end

---------------------------------------------------------------
-- Attach a screenshot link under today's entry (or create today's entry first).
function JOURNAL.md.attachScreenshot(section, relPath)
    local today = JOURNAL.today()
    local todayLn = "* " .. today
    -- find today's entry (any line starting with `* YYYY-MM-DD`, YYYY-MM-DD = today)
    local idx = nil
    for i = 1, #section.lines do
        local l = section.lines[i]
        if l:find("^%* " .. today) then
            idx = i
        end
    end
    local attach = "  [[" .. relPath .. "]]"
    if idx then
        -- insert after the last contiguous block belonging to today's entry
        local insertAt = idx + 1
        while insertAt <= #section.lines do
            local l = section.lines[insertAt]
            if l:sub(1, 2) == "* " or l:match(HEADING_PATTERN) then break end
            insertAt = insertAt + 1
        end
        table.insert(section.lines, insertAt, attach)
    else
        -- create today's stub, then attach under it
        JOURNAL.md.appendMemo(section, "")
        -- appendMemo inserted "* today " + trailing "" at end; attach goes right before trailing blank
        table.insert(section.lines, #section.lines, attach)
    end
end
