--- Actor creation utilities
-- @module 02_ActorDef

-- Convert "@/path/file" to "/path/".
local function DebugPathToRealPath(p)
	if not p or p:sub(1, 1) ~= "@" then
		return nil
	end

	local Path = p:sub(2)
	local pos = Path:find_last("/")
	return string.sub(Path, 1, pos)
end

local function MergeTables(left, right)
	local ret = {}
	for key, val in pairs(left) do
		ret[key] = val
	end

	for key, val in pairs(right) do
		if ret[key] then
			if type(val) == "function" and type(ret[key]) == "function" then
				local f1 = ret[key]
				local f2 = val
				val = function(...)
					f1(...)
					return f2(...)
				end
			else
				Warn(string.format('%s\n\nOverriding "%s": %s with %s', debug.traceback(), key, type(ret[key]), type(val)))
			end
		end

		ret[key] = val
	end
	setmetatable(ret, getmetatable(left))
	return ret
end

DefMetatable = {
	__concat = function(left, right)
		return MergeTables(left, right)
	end
}

--- This is used as follows:
--
-- @usage Def["ActorType"]({}) or Def.ActorType({})
-- ex. Def.ActorFrame {}
-- Note: The {} here are just syntax sugar for a function call ({})
-- @table Def
Def = {}
setmetatable(
	Def,
	{
		__index = function(self, Class)
			-- t is an actor definition table.  name is the type
			-- given to Def.  Fill in standard fields.
			return function(t)
				if not ActorUtil.IsRegisteredClass(Class) then
					error(Class .. " is not a registered actor class", 2)
				end

				t.Class = Class

				local level = 2
				if t._Level then
					level = t._Level + 1
				end
				local info = debug.getinfo(level, "Sl")

				-- Source file of caller:
				local Source = info and info.source or ""
				t._Source = Source

				t._Dir = DebugPathToRealPath(Source)

				-- Line number of caller:
				t._Line = info and info.currentline or 0

				setmetatable(t, DefMetatable)
				return t
			end
		end
	}
)

--- Resolve a path (Returns resolved path string)
-- @string path
-- @number level integer, call stack level for error()
-- @boolean optional is this path required or optional
function ResolveRelativePath(path, level, optional)
	if path:sub(1, 1) ~= "/" then
		-- "Working directory":
		local sDir = DebugPathToRealPath(debug.getinfo(level + 1, "S").source)
		assert(sDir)

		path = sDir .. path
	end

	path = ActorUtil.ResolvePath(path, level + 1, optional)
	return path
end

--- Load an actor template.
-- @string path
-- @number level integer, call stack level for error()
function LoadActorFunc(path, level)
	level = level or 1

	if path == "" then
		error("Passing in a blank filename is a great way to eat up RAM. Good thing we warn you about this.")
	end

	if level and level > 0 then
		local ResolvedPath = ResolveRelativePath(path, level + 1)
		if not ResolvedPath then
			error(path .. ": not found", level + 1)
		end
		path = ResolvedPath
	end

	local Type = ActorUtil.GetFileType(path)
	if PREFSMAN:GetPreference("VerboseLogging") > 1 then
		Trace("Loading " .. path .. ", type " .. tostring(Type))
	end

	if Type == "FileType_Lua" then
		-- Load the file.
		local chunk, errmsg = loadfile(path)
		if not chunk then
			error(errmsg)
		end
		return chunk
	end

	if Type == "FileType_Bitmap" or Type == "FileType_Movie" then
		return function()
			return Def.Sprite {
				_Level = level + 1,
				Texture = path
			}
		end
	elseif Type == "FileType_Sound" then
		return function()
			return Def.Sound {
				_Level = level + 1,
				File = path
			}
		end
	elseif Type == "FileType_Model" then
		return function()
			return Def.Model {
				_Level = level + 1,
				Meshes = path,
				Materials = path,
				Bones = path
			}
		end
	elseif Type == "FileType_Directory" then
		return function()
			return Def.BGAnimation {
				_Level = level + 1,
				AniDir = path
			}
		end
	end

	error(path .. ": unknown file type (" .. tostring(Type) .. ")", level + 1)
end

--- Load and create an actor template.
-- @string path
function LoadActor(path, ...)
	local t = LoadActorFunc(path, 2)
	assert(t)
	return t(...)
end

--- Same as LoadActor but sets everything in params as
-- "Thread Variables" accessed via Var("name")
-- @string path path to file without extension
-- @table params doing Var(key) in the file in path will index this table
function LoadActorWithParams(path, params, ...)
	local t = LoadActorFunc(path, 2)
	assert(t)
	return lua.RunWithThreadVariables(
		function(...)
			return t(...)
		end,
		params,
		...
	)
end

--- Function to easily create a BitmapText Actor
-- Can be used like LoadFont("Common normal")
-- or LoadFont("Common", "normal")
function LoadFont(a, b)
	local sSection = b and a or ""
	local sFile = b or a
	if sFile == "" or not sFile then
		sSection = "Common"
		sFile = "normal"
	end
	local sPath = THEME:GetPathF(sSection, sFile)
	return Def.BitmapText {
		_Level = 2,
		File = sPath
	}
end

--- Same as LoadFont but for ColorBitmapText Actor
function LoadColorFont(a, b)
	local sSection = b and a or ""
	local sFile = b or a
	if sFile == "" or not sFile then
		sSection = "Common"
		sFile = "normal"
	end
	local sPath = THEME:GetPathF(sSection, sFile)
	return Def.ColorBitmapText {
		_Level = 2,
		File = sPath
	}
end

--- Returns an ActorFrame with the numeric table/array t as children
function WrapInActorFrame(t)
	return Def.ActorFrame {children = t}
end

---
function StandardDecorationFromTable(MetricsName, t)
	if type(t) == "table" then
		t =
			t ..
			{
				InitCommand = function(self)
					self:name(MetricsName)
					ActorUtil.LoadAllCommandsAndSetXY(self, Var "LoadingScreen")
				end
			}
	end
	return t
end

---
function StandardDecorationFromFile(MetricsName, FileName)
	local t = LoadActor(THEME:GetPathG(Var "LoadingScreen", FileName))
	return StandardDecorationFromTable(MetricsName, t)
end

---
function StandardDecorationFromFileOptional(MetricsName, FileName)
	if ShowStandardDecoration(MetricsName) then
		return StandardDecorationFromFile(MetricsName, FileName)
	end
end

---
function ShowStandardDecoration(MetricsName)
	return THEME:GetMetric(Var "LoadingScreen", "Show" .. MetricsName)
end

-- @table NullActor
--blank actor because these come in handy from time to time
NullActor = {Class = "Actor", _Source = "(null actor)"}

-- (c) 2006 Glenn Maynard
-- All rights reserved.
--
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, and/or sell copies of the Software, and to permit persons to
-- whom the Software is furnished to do so, provided that the above
-- copyright notice(s) and this permission notice appear in all copies of
-- the Software and that both the above copyright notice(s) and this
-- permission notice appear in supporting documentation.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
-- THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
-- INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
-- OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
-- OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
-- OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
-- PERFORMANCE OF THIS SOFTWARE.
