--- Mostly default lua function overrides
-- @module 01_base

--- Override Lua's loadfile to use lua.ReadFile.
-- @string file
-- @treturn string|nil chunk The file contents (Or nil if there was an error)
-- @treturn string|nil error The error (Or nil if succesful)
function loadfile(file)
	local data, err = lua.ReadFile(file)
	if not data then
		return nil, ("what " .. file)
	end

	local chunk, err =
		load(
		function()
			local ret = data
			data = nil
			return ret
		end,
		"@" .. file
	)

	if not chunk then
		return nil, err
	end

	-- Set the environment, like loadfile does.
	setfenv(chunk, getfenv(2))
	return chunk
end

--- Override Lua's dofile to use our loadfile.
-- @tparam string file filename
-- @treturn string|error
function dofile(file)
	if not file then
		error("dofile(nil) unsupported", 2)
	end
	local chunk, err = loadfile(file)
	if not chunk then
		error(err, 2)
	end
	return chunk()
end

--- Like ipairs(), but returns only values.
-- @tab t
-- @treturn function iterator
-- @usage for v in ivalues({1,2}) do print(tostring(v)) end
function ivalues(t)
	local n = 0
	return function()
		n = n + 1
		return t[n]
	end
end
--- lua.GetThreadVariable alias
-- Mostly used for noteskins
-- @string varname
-- @treturn any value
-- @function Var
-- @usage `local x = Var Var "Button"`
Var = lua.GetThreadVariable
