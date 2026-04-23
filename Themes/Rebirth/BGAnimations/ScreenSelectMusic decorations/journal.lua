-- Journal: ScreenSelectMusic overlay
-- - Subscribes to DFRFinished → regenerate all pack markdowns.
-- - Hooks Ctrl+Shift+C → memo entry for current song.
-- - Hooks Ctrl+Shift+T → tag toggle cycle for current song.
-- Spec: Docs/Journal.md §5 (hotkeys), §3 (markdown), §6 (tags).

local t = Def.ActorFrame {
    Name = "JournalOverlay",
}

---------------------------------------------------------------
-- Identity helpers

local function currentSong()
    if not GAMESTATE then return nil end
    return GAMESTATE:GetCurrentSong()
end

local function songIdentity(song)
    if not song then return nil, nil, nil end
    local pack = song.GetGroupName and song:GetGroupName() or "Unknown"
    local dir = song.GetSongDir and song:GetSongDir() or ""
    dir = dir:gsub("[/\\]$", "")
    local seg = dir:match("([^/\\]+)$") or dir
    local title = (song.GetDisplayMainTitle and song:GetDisplayMainTitle()) or
                  (song.GetMainTitle and song:GetMainTitle()) or seg
    return pack, seg, title
end

---------------------------------------------------------------
-- Append memo to current song's .md

local function appendMemoToCurrentSong(text)
    local song = currentSong()
    if not song then return false, "no song" end
    local pack, dir, title = songIdentity(song)
    local path = JOURNAL.packMarkdownPath(pack)
    local existing = JOURNAL.readText(path) or ""
    local doc = JOURNAL.md.parse(existing)
    local idx = JOURNAL.md.indexByTitle(doc)[title]
    local section
    if idx then
        section = doc.sections[idx]
    else
        local banner = JOURNAL.banner.copyFor(song, pack, dir)
        local tags = JOURNAL.tags.get(pack, dir)
        section = JOURNAL.md.newSection({ title = title, bannerPath = banner, tags = tags })
        doc.sections[#doc.sections + 1] = section
    end
    JOURNAL.md.appendMemo(section, text)
    return JOURNAL.writeText(path, JOURNAL.md.serialize(doc))
end

---------------------------------------------------------------
-- Memo entry flow via ScreenTextEntry

local function openMemoEntry()
    if not JOURNAL.isEnabled() then
        ms.ok("[Journal] disabled — set JournalOutputDir + enable")
        return
    end
    local song = currentSong()
    if not song then
        ms.ok("[Journal] no song selected")
        return
    end
    local _, _, title = songIdentity(song)

    SCREENMAN:AddNewScreenToTop("ScreenTextEntry")
    local scr = SCREENMAN:GetTopScreen()
    if not (scr and scr.Load) then
        ms.ok("[Journal] ScreenTextEntry unavailable")
        return
    end
    scr:Load({
        Question = "Memo: " .. title,
        InitialAnswer = "",
        MaxInputLength = 400,
        OnOK = function(answer)
            if answer and answer ~= "" then
                local ok = appendMemoToCurrentSong(answer)
                ms.ok(ok and ("[Journal] memo saved to " .. title) or "[Journal] write failed")
            end
        end,
        OnCancel = function() end,
    })
end

---------------------------------------------------------------
-- Tag cycle toggle (Phase D placeholder — real UI in next iteration)

local tagCycleIdx = 1

local function cycleToggleTag()
    if not JOURNAL.isEnabled() then
        ms.ok("[Journal] disabled")
        return
    end
    local song = currentSong()
    if not song then return end
    local pack, dir, title = songIdentity(song)
    local defined = JOURNAL.tags.defined()
    if #defined == 0 then
        ms.ok("[Journal] no tags defined")
        return
    end
    local tag = defined[((tagCycleIdx - 1) % #defined) + 1]
    tagCycleIdx = tagCycleIdx + 1
    local added = JOURNAL.tags.toggle(pack, dir, tag)
    -- sync tag line in markdown
    local path = JOURNAL.packMarkdownPath(pack)
    local existing = JOURNAL.readText(path) or ""
    local doc = JOURNAL.md.parse(existing)
    local idx = JOURNAL.md.indexByTitle(doc)[title]
    if idx then
        JOURNAL.md.syncManagedLines(doc.sections[idx], nil, JOURNAL.tags.get(pack, dir))
        JOURNAL.writeText(path, JOURNAL.md.serialize(doc))
    end
    ms.ok("[Journal] " .. (added and "+" or "-") .. tag)
end

---------------------------------------------------------------
-- DFRFinished hook: regenerate all pack .md

t[#t + 1] = Def.Actor {
    Name = "JournalDFRListener",
    OnCommand = function(self)
        -- Trigger initial regen with slight delay
        self:sleep(0.2):queuecommand("InitialRegen")
    end,
    InitialRegenCommand = function()
        if JOURNAL.isEnabled() then
            JOURNAL.pack.regenerateAll()
        end
    end,
    DFRFinishedMessageCommand = function()
        if JOURNAL.isEnabled() then
            JOURNAL.pack.regenerateAll()
        end
    end,
}

---------------------------------------------------------------
-- Input listener

t[#t + 1] = Def.Actor {
    Name = "JournalInputListener",
    OnCommand = function(self)
        local screen = SCREENMAN:GetTopScreen()
        if not screen or not screen.AddInputCallback then return end
        screen:AddInputCallback(function(event)
            if event.type ~= "InputEventType_FirstPress" then return end
            if not (INPUTFILTER:IsControlPressed() and INPUTFILTER:IsShiftPressed()) then return end
            local key = event.DeviceInput and event.DeviceInput.button
            if key == "DeviceButton_c" then
                openMemoEntry()
            elseif key == "DeviceButton_t" then
                cycleToggleTag()
            end
        end)
    end,
}

return t
