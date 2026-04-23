-- Journal: core module
-- Provides: config (JOURNAL.cfg), path helpers, file I/O, sanitization, date.
-- Loaded before other Journal modules (50-prefix). Spec: Docs/Journal.md.

JOURNAL = JOURNAL or {}

---------------------------------------------------------------
-- Config (persisted via theme ThemePrefs under Save/<Theme>/journal.lua)

local defaultConfig = {
    enabled = false,           -- master switch, default off so vanilla users unaffected
    outputDir = "",            -- absolute path to journal root (Obsidian vault)
    bannerDir = "banners",     -- relative to outputDir
    screenshotDir = "screenshots",
    tagsFile = "tags.json",
    cacheFile = ".journal-cache.json",
}

journalConfig = create_setting("journal", "journal.lua", defaultConfig, -1)
journalConfig:load()

function JOURNAL.cfg()
    return journalConfig:get_data()
end

function JOURNAL.saveCfg()
    journalConfig:set_dirty()
    journalConfig:save()
end

function JOURNAL.isEnabled()
    local c = JOURNAL.cfg()
    return c.enabled and c.outputDir ~= nil and c.outputDir ~= ""
end

---------------------------------------------------------------
-- Path helpers (all absolute, using `/` — normalized for Windows by io.open)

local function joinPath(a, b)
    if a:sub(-1) == "/" or a:sub(-1) == "\\" then
        return a .. b
    end
    return a .. "/" .. b
end
JOURNAL.joinPath = joinPath

function JOURNAL.root()
    return JOURNAL.cfg().outputDir
end

function JOURNAL.bannerRoot()
    return joinPath(JOURNAL.root(), JOURNAL.cfg().bannerDir)
end

function JOURNAL.screenshotRoot()
    return joinPath(JOURNAL.root(), JOURNAL.cfg().screenshotDir)
end

function JOURNAL.tagsPath()
    return joinPath(JOURNAL.root(), JOURNAL.cfg().tagsFile)
end

function JOURNAL.cachePath()
    return joinPath(JOURNAL.root(), JOURNAL.cfg().cacheFile)
end

function JOURNAL.packMarkdownPath(packName)
    return joinPath(JOURNAL.root(), JOURNAL.sanitize(packName) .. ".md")
end

---------------------------------------------------------------
-- Name sanitization: safe filename on both Linux and Windows.
-- Replace reserved chars with `_`. Preserves unicode.

local FORBIDDEN = "[\\/:%*%?\"<>%|%c]"

function JOURNAL.sanitize(s)
    if not s or s == "" then return "_" end
    local out = s:gsub(FORBIDDEN, "_"):gsub("%s+$", ""):gsub("^%s+", "")
    if out == "" then return "_" end
    return out
end

-- Banner / screenshot filename: {pack}__{song}[.ext]
function JOURNAL.bannerName(packName, songDir, ext)
    return JOURNAL.sanitize(packName) .. "__" .. JOURNAL.sanitize(songDir) .. "." .. ext
end

function JOURNAL.screenshotName(packName, songDir, ext)
    local ts = os.date("%Y%m%d_%H%M%S")
    return JOURNAL.sanitize(packName) .. "__" .. JOURNAL.sanitize(songDir) .. "__" .. ts .. "." .. ext
end

---------------------------------------------------------------
-- File I/O via Lua stdlib (works on any absolute path, no VFS needed).

local function detectWindows()
    -- Etterna's Lua exposes os.getenv; Windows sets COMSPEC, Unix doesn't.
    local cs = os.getenv and os.getenv("COMSPEC")
    if cs and cs ~= "" then return true end
    -- fallback: package.config first char is path sep
    if package and package.config then
        return package.config:sub(1, 1) == "\\"
    end
    return false
end
JOURNAL.isWindows = detectWindows()

-- mkdir -p equivalent. Returns true on success.
function JOURNAL.ensureDir(path)
    if not path or path == "" then return false end
    local cmd
    if JOURNAL.isWindows then
        -- Windows mkdir recursively creates with extensions enabled (default)
        -- Quote path; backslash or forward-slash both accepted.
        cmd = 'if not exist "' .. path .. '" mkdir "' .. path .. '"'
    else
        cmd = 'mkdir -p "' .. path .. '"'
    end
    local ok = os.execute(cmd)
    return ok == true or ok == 0
end

function JOURNAL.readText(path)
    local f = io.open(path, "r")
    if not f then return nil end
    local s = f:read("*all")
    f:close()
    return s
end

-- Atomic write: write to tmp then rename. Returns true on success.
function JOURNAL.writeText(path, content)
    local tmp = path .. ".tmp"
    local f, err = io.open(tmp, "w")
    if not f then
        print("[Journal] writeText open failed: " .. tostring(err))
        return false
    end
    f:write(content)
    f:close()
    local ok, rerr = os.rename(tmp, path)
    if not ok then
        print("[Journal] writeText rename failed: " .. tostring(rerr))
        os.remove(tmp)
        return false
    end
    return true
end

function JOURNAL.copyBinary(srcPath, dstPath)
    local src = io.open(srcPath, "rb")
    if not src then return false end
    local data = src:read("*all")
    src:close()
    local dst = io.open(dstPath, "wb")
    if not dst then return false end
    dst:write(data)
    dst:close()
    return true
end

function JOURNAL.fileExists(path)
    local f = io.open(path, "rb")
    if f then f:close(); return true end
    return false
end

---------------------------------------------------------------
-- Date / timestamp

function JOURNAL.today()
    return os.date("%Y-%m-%d")
end

function JOURNAL.nowStamp()
    return os.date("%Y%m%d_%H%M%S")
end

---------------------------------------------------------------
-- Logging (single prefix for easy grep)

function JOURNAL.log(msg)
    print("[Journal] " .. tostring(msg))
end

---------------------------------------------------------------
-- Initialize required directories. Safe to call repeatedly.
function JOURNAL.ensureLayout()
    if not JOURNAL.isEnabled() then return false end
    local root = JOURNAL.root()
    if not JOURNAL.ensureDir(root) then
        JOURNAL.log("ensureDir failed for root: " .. root)
        return false
    end
    JOURNAL.ensureDir(JOURNAL.bannerRoot())
    JOURNAL.ensureDir(JOURNAL.screenshotRoot())
    return true
end
