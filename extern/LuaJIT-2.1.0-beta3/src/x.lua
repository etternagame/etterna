local ffi = require("ffi")
ffi.cdef[[
typedef struct treeBranch treeBranch;
typedef struct treeBranch{
    double x;
    double y;
    treeBranch *left;
    treeBranch *right;
} treeBranch;
 
void *malloc(size_t size);
void free(void *ptr);
]]
local NULL = nil--ffi.new("void*")

local quad
do
    local tree_type = ffi.typeof("treeBranch")
    local ptr_tree_type = ffi.typeof("treeBranch *")
    local typesize    = ffi.sizeof(tree_type)
    quad = function() 
        local ptr_raw = ffi.C.malloc(typesize)
        local ptr = ffi.cast(ptr_tree_type, ptr_raw)

        return ptr
    end
end


local function merge(lower, greater)
    if lower == NULL then return greater end
    if greater == NULL then return lower end
 
    if lower[0].y < greater[0].y then
        lower[0].right = merge(lower[0].right, greater)
 
        return lower
    else
        greater[0].left = merge(lower, greater[0].left)
 
        return greater
    end
end
 
local function split_binary(orig, value)
    if orig == NULL then return NULL, NULL end
 
    if orig[0].x < value then
        local origRight, origLeft = split_binary(orig[0].right, value)
        orig[0].right = origRight
 
        return orig, origLeft
    else
        local origRight, origLeft = split_binary(orig[0].left, value)
        orig[0].left = origLeft
 
        return origRight, orig
    end
end
 
local function merge3(lower, equal, greater)
    return merge(merge(lower, equal), greater)
end
 
local function split(orig, value)
    local lower, equalGreater = split_binary(orig, value)
    local equal, greater = split_binary(equalGreater, value + 1)
 
    return lower, equal, greater
end
 
local function tree_has_value(tree, x)
    local lower, equal, greater = split(tree.root, x)
    local res = equal ~= NULL
    tree.root = merge3(lower, equal, greater)
 
    return res
end
 
local function tree_insert(tree, x)
    local lower, equal, greater = split(tree.root, x)
 
    if equal == NULL then
        --equal = quad(x, math.random(0, 2 ^ 31), NULL, NULL) -- disabled randomization for test case
        equal = quad()--x, 42, NULL, NULL)
        equal[0].x = x;
        equal[0].y = 42;
        equal[0].left = NULL;
        equal[0].right = NULL;
    end
 
    tree.root = merge3(lower, equal, greater)
end
 
local function tree_erase(tree, x)
    local lower, equal, greater = split(tree.root, x)
    if equal ~= NULL then
        ffi.C.free(equal)
    end
    tree.root = merge(lower, greater)
end
 
local function main()
    local tree = {
        root = NULL
    }
 
    local cur = 5
    local res = 0
 
    for i = 1, 100000 do
        local a = i % 3
        cur = (cur * 57 + 43) % 10007
 
        if a == 0 then
            tree_insert(tree, cur)
        elseif a == 1 then
            tree_erase(tree, cur)
        elseif a == 2 then
            if tree_has_value(tree, cur) then
                res = res + 1
            end
        end
    end
 
    print(res)
end
 
local t = os.clock()
main()
print(os.clock() - t)