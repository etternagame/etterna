-- Journal: tags.json persistence
-- Schema (Docs/Journal.md §6):
--   { defined = {tag, ...}, assigned = { ["pack/song"] = {tag, ...} } }

JOURNAL = JOURNAL or {}
JOURNAL.tags = {}

local DEFAULT_DEFINED = {
    "Etterna/4KEY/Jumpstream",
    "Etterna/4KEY/Handstream",
    "Etterna/4KEY/Chordjack",
    "Etterna/4KEY/Stream",
    "Etterna/4KEY/Technical",
    "Etterna/4KEY/Jack",
    "Etterna/4KEY/Stamina",
    "Etterna/4KEY/LN",
}

---------------------------------------------------------------
-- Minimal JSON encode/decode (sufficient for our flat schema).
-- For production maps with nested tables of strings, this works; if we ever
-- need richer structures, swap in a proper lib.

local json = {}

local function escape_str(s)
    return (s:gsub("[\\\"\n\r\t]", {
        ["\\"] = "\\\\", ['"'] = '\\"',
        ["\n"] = "\\n", ["\r"] = "\\r", ["\t"] = "\\t",
    }))
end

local function encode_value(v, indent)
    local t = type(v)
    if t == "string" then
        return '"' .. escape_str(v) .. '"'
    elseif t == "number" or t == "boolean" then
        return tostring(v)
    elseif t == "nil" then
        return "null"
    elseif t == "table" then
        -- array if sequential 1..n with no gaps
        local n = #v
        local isArr = (n > 0)
        for k in pairs(v) do
            if type(k) ~= "number" or k ~= math.floor(k) or k < 1 or k > n then
                isArr = false; break
            end
        end
        local pad = string.rep("  ", indent or 0)
        local padInner = string.rep("  ", (indent or 0) + 1)
        if isArr then
            if n == 0 then return "[]" end
            local parts = {}
            for i = 1, n do parts[i] = padInner .. encode_value(v[i], (indent or 0) + 1) end
            return "[\n" .. table.concat(parts, ",\n") .. "\n" .. pad .. "]"
        else
            local keys = {}
            for k in pairs(v) do keys[#keys + 1] = k end
            table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
            if #keys == 0 then return "{}" end
            local parts = {}
            for _, k in ipairs(keys) do
                parts[#parts + 1] = padInner .. '"' .. escape_str(tostring(k)) .. '": ' .. encode_value(v[k], (indent or 0) + 1)
            end
            return "{\n" .. table.concat(parts, ",\n") .. "\n" .. pad .. "}"
        end
    end
    return "null"
end
json.encode = function(v) return encode_value(v, 0) end

-- Decode: minimal recursive-descent. Enough for round-tripping encode().
local pos
local function skip_ws(s)
    while pos <= #s do
        local c = s:sub(pos, pos)
        if c == " " or c == "\t" or c == "\n" or c == "\r" then pos = pos + 1 else break end
    end
end
local decode_value
local function decode_string(s)
    assert(s:sub(pos, pos) == '"'); pos = pos + 1
    local out = {}
    while pos <= #s do
        local c = s:sub(pos, pos)
        if c == '"' then pos = pos + 1; return table.concat(out) end
        if c == "\\" then
            local e = s:sub(pos + 1, pos + 1)
            pos = pos + 2
            if e == "n" then out[#out + 1] = "\n"
            elseif e == "t" then out[#out + 1] = "\t"
            elseif e == "r" then out[#out + 1] = "\r"
            elseif e == "\"" then out[#out + 1] = "\""
            elseif e == "\\" then out[#out + 1] = "\\"
            elseif e == "/" then out[#out + 1] = "/"
            else out[#out + 1] = e end
        else
            out[#out + 1] = c; pos = pos + 1
        end
    end
    error("unterminated string")
end
local function decode_array(s)
    pos = pos + 1  -- '['
    skip_ws(s)
    local arr = {}
    if s:sub(pos, pos) == "]" then pos = pos + 1; return arr end
    while true do
        skip_ws(s)
        arr[#arr + 1] = decode_value(s)
        skip_ws(s)
        local c = s:sub(pos, pos)
        if c == "," then pos = pos + 1
        elseif c == "]" then pos = pos + 1; return arr
        else error("expected , or ]") end
    end
end
local function decode_object(s)
    pos = pos + 1  -- '{'
    skip_ws(s)
    local obj = {}
    if s:sub(pos, pos) == "}" then pos = pos + 1; return obj end
    while true do
        skip_ws(s)
        local k = decode_string(s)
        skip_ws(s)
        assert(s:sub(pos, pos) == ":", "expected :"); pos = pos + 1
        skip_ws(s)
        obj[k] = decode_value(s)
        skip_ws(s)
        local c = s:sub(pos, pos)
        if c == "," then pos = pos + 1
        elseif c == "}" then pos = pos + 1; return obj
        else error("expected , or }") end
    end
end
decode_value = function(s)
    skip_ws(s)
    local c = s:sub(pos, pos)
    if c == '"' then return decode_string(s) end
    if c == "{" then return decode_object(s) end
    if c == "[" then return decode_array(s) end
    if c == "t" and s:sub(pos, pos + 3) == "true" then pos = pos + 4; return true end
    if c == "f" and s:sub(pos, pos + 4) == "false" then pos = pos + 5; return false end
    if c == "n" and s:sub(pos, pos + 3) == "null" then pos = pos + 4; return nil end
    -- number
    local numStr = s:match("^(%-?%d+%.?%d*[eE]?[+%-]?%d*)", pos)
    if numStr and numStr ~= "" then pos = pos + #numStr; return tonumber(numStr) end
    error("unexpected char at pos " .. pos .. ": " .. c)
end
json.decode = function(str)
    pos = 1
    return decode_value(str)
end

JOURNAL.tags.json = json  -- exposed for other modules / tests

---------------------------------------------------------------
-- Keys

local function songKey(packName, songDir)
    return packName .. "/" .. songDir
end
JOURNAL.tags.songKey = songKey

---------------------------------------------------------------
-- Cache (in-memory mirror of tags.json)

local cached = nil

function JOURNAL.tags.load()
    if not JOURNAL.isEnabled() then return nil end
    local path = JOURNAL.tagsPath()
    local text = JOURNAL.readText(path)
    if not text or text == "" then
        cached = { defined = DEFAULT_DEFINED, assigned = {} }
        JOURNAL.tags.save()
        return cached
    end
    local ok, data = pcall(json.decode, text)
    if not ok or type(data) ~= "table" then
        JOURNAL.log("tags.json parse failed, using defaults: " .. tostring(data))
        cached = { defined = DEFAULT_DEFINED, assigned = {} }
        return cached
    end
    data.defined = data.defined or {}
    data.assigned = data.assigned or {}
    cached = data
    return cached
end

function JOURNAL.tags.save()
    if not cached then return false end
    if not JOURNAL.isEnabled() then return false end
    return JOURNAL.writeText(JOURNAL.tagsPath(), json.encode(cached))
end

function JOURNAL.tags.get(packName, songDir)
    if not cached then JOURNAL.tags.load() end
    if not cached then return {} end
    return cached.assigned[songKey(packName, songDir)] or {}
end

function JOURNAL.tags.set(packName, songDir, tagList)
    if not cached then JOURNAL.tags.load() end
    if not cached then return false end
    local key = songKey(packName, songDir)
    if tagList and #tagList > 0 then
        cached.assigned[key] = tagList
    else
        cached.assigned[key] = nil
    end
    return JOURNAL.tags.save()
end

function JOURNAL.tags.defined()
    if not cached then JOURNAL.tags.load() end
    if not cached then return {} end
    return cached.defined
end

function JOURNAL.tags.addDefined(tag)
    if not cached then JOURNAL.tags.load() end
    if not cached then return false end
    for _, t in ipairs(cached.defined) do
        if t == tag then return true end
    end
    cached.defined[#cached.defined + 1] = tag
    return JOURNAL.tags.save()
end

function JOURNAL.tags.toggle(packName, songDir, tag)
    local cur = JOURNAL.tags.get(packName, songDir)
    local found = nil
    for i, t in ipairs(cur) do
        if t == tag then found = i; break end
    end
    if found then
        table.remove(cur, found)
    else
        cur[#cur + 1] = tag
    end
    JOURNAL.tags.set(packName, songDir, cur)
    return not found  -- returns true if tag was ADDED
end
