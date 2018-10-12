local settings_prefix = "/" .. THEME:GetRealThemeDisplayName() .. "_settings/"
global_cur_game = GAMESTATE:GetCurrentGame():GetName():lower()

function force_table_elements_to_match_type(candidate, must_match, depth_remaining)
	for k, v in pairs(candidate) do
		if type(must_match[k]) ~= type(v) then
			candidate[k] = nil
		elseif type(v) == "table" and depth_remaining ~= 0 then
			force_table_elements_to_match_type(v, must_match[k], depth_remaining - 1)
		end
	end
	for k, v in pairs(must_match) do
		if type(candidate[k]) == "nil" then
			if type(v) == "table" then
				candidate[k] = DeepCopy(v)
			else
				candidate[k] = v
			end
		end
	end
end

local function slot_to_prof_dir(slot, reason)
	local prof_dir = "Save"
	if slot and slot ~= "ProfileSlot_Invalid" then
		prof_dir = PROFILEMAN:GetProfileDir(slot)
		if not prof_dir or prof_dir == "" then
			--Warn("Could not fetch profile dir to " .. reason .. ".")
			return
		end
	end
	return prof_dir
end

local function load_conf_file(fname)
	local file = RageFileUtil.CreateRageFile()
	local ret = {}
	if file:Open(fname, 1) then
		local data = loadstring(file:Read())
		setfenv(data, {})
		local success, data_ret = pcall(data)
		if success then
			ret = data_ret
		end
		file:Close()
	end
	file:destroy()
	return ret
end

local setting_mt = {
	__index = {
		init = function(self, name, file, default, match_depth)
			assert(type(default) == "table", "default for setting must be a table.")
			self.name = name
			self.file = file
			self.default = default
			self.match_depth = match_depth
			self.dirty_table = {}
			self.data_set = {}
			return self
		end,
		load = function(self, slot)
			slot = slot or "ProfileSlot_Invalid"
			local prof_dir = slot_to_prof_dir(slot, "read " .. self.name)
			if not prof_dir then
				self.data_set[slot] = DeepCopy(self.default)
			else
				local fname = prof_dir .. settings_prefix .. self.file
				if not FILEMAN:DoesFileExist(fname) then
					self.data_set[slot] = DeepCopy(self.default)
				else
					local from_file = load_conf_file(fname)
					if type(from_file) == "table" then
						if self.match_depth and self.match_depth ~= 0 then
							force_table_elements_to_match_type(from_file, self.default, self.match_depth - 1)
						end
						self.data_set[slot] = from_file
					else
						self.data_set[slot] = DeepCopy(self.default)
					end
				end
			end
			return self.data_set[slot]
		end,
		get_data = function(self, slot)
			slot = slot or "ProfileSlot_Invalid"
			return self.data_set[slot] or self:load(slot)
		end,
		set_data = function(self, slot, data)
			slot = slot or "ProfileSlot_Invalid"
			self.data_set[slot] = data
		end,
		set_dirty = function(self, slot)
			slot = slot or "ProfileSlot_Invalid"
			self.dirty_table[slot] = true
		end,
		check_dirty = function(self, slot)
			slot = slot or "ProfileSlot_Invalid"
			return self.dirty_table[slot]
		end,
		clear_slot = function(self, slot)
			slot = slot or "ProfileSlot_Invalid"
			self.dirty_table[slot] = nil
			self.data_set[slot] = nil
		end,
		save = function(self, slot)
			slot = slot or "ProfileSlot_Invalid"
			if not self:check_dirty(slot) then
				return
			end
			local prof_dir = slot_to_prof_dir(slot, "write " .. self.name)
			if not prof_dir then
				return
			end
			local fname = prof_dir .. settings_prefix .. self.file
			local file_handle = RageFileUtil.CreateRageFile()
			if not file_handle:Open(fname, 2) then
				Warn("Could not open '" .. fname .. "' to write " .. self.name .. ".")
			else
				local output = "return " .. lua_table_to_string(self.data_set[slot])
				file_handle:Write(output)
				file_handle:Close()
				file_handle:destroy()
			end
		end,
		save_all = function(self)
			for slot, data in pairs(self.data_set) do
				self:save(slot)
			end
		end
	}
}

function create_setting(name, file, default, match_depth)
	return setmetatable({}, setting_mt):init(name, file, default, match_depth)
end

function write_str_to_file(str, fname, str_name)
	local file_handle = RageFileUtil.CreateRageFile()
	if not file_handle:Open(fname, 2) then
		Warn("Could not open '" .. fname .. "' to write " .. str_name .. ".")
	else
		file_handle:Write(str)
		file_handle:Close()
		file_handle:destroy()
	end
end

function string_needs_escape(str)
	if str:match("^[a-zA-Z_][a-zA-Z_0-9]*$") then
		return false
	else
		return true
	end
end

function lua_table_to_string(t, indent, line_pos)
	indent = indent or ""
	line_pos = (line_pos or #indent) + 1
	local internal_indent = indent .. "  "
	local ret = "{"
	local has_table = false
	for k, v in pairs(t) do
		if type(v) == "table" then
			has_table = true
		end
	end
	if has_table then
		ret = "{\n" .. internal_indent
		line_pos = #internal_indent
	end
	local separator = ""
	local function do_value_for_key(k, v, need_key_str)
		if type(v) == "nil" then
			return
		end
		local k_str = k
		if type(k) == "number" then
			k_str = "[" .. k .. "]"
		else
			if string_needs_escape(k) then
				k_str = "[" .. ("%q"):format(k) .. "]"
			else
				k_str = k
			end
		end
		if need_key_str then
			k_str = k_str .. "= "
		else
			k_str = ""
		end
		local v_str = ""
		if type(v) == "table" then
			v_str = lua_table_to_string(v, internal_indent, line_pos + #k_str)
		elseif type(v) == "string" then
			v_str = ("%q"):format(v)
		elseif type(v) == "number" then
			if v ~= math.floor(v) then
				v_str = ("%.6f"):format(v)
				local last_nonz = v_str:reverse():find("[^0]")
				if last_nonz then
					v_str = v_str:sub(1, -last_nonz)
				end
			else
				v_str = tostring(v)
			end
		else
			v_str = tostring(v)
		end
		local to_add = k_str .. v_str
		if type(v) == "table" then
			if separator == "" then
				to_add = separator .. to_add
			else
				to_add = separator .. "\n" .. internal_indent .. to_add
			end
		else
			if line_pos + #separator + #to_add > 80 then
				line_pos = #internal_indent + #to_add
				to_add = separator .. "\n" .. internal_indent .. to_add
			else
				to_add = separator .. to_add
				line_pos = line_pos + #to_add
			end
		end
		ret = ret .. to_add
		separator = ", "
	end
	-- do the integer indices from 0 to n first, in order.
	do_value_for_key(0, t[0], true)
	for n = 1, #t do
		do_value_for_key(n, t[n], false)
	end
	for k, v in pairs(t) do
		local is_integer_key = (type(k) == "number") and (k == math.floor(k)) and k >= 0 and k <= #t
		if not is_integer_key then
			do_value_for_key(k, v, true)
		end
	end
	ret = ret .. "}"
	return ret
end

local slot_conversion = {
	[PLAYER_1] = "ProfileSlot_Player1",
	[PLAYER_2] = "ProfileSlot_Player2"
}
function pn_to_profile_slot(pn)
	return slot_conversion[pn] or "ProfileSlot_Invalid"
end
