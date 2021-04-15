local vmdef = require "jit.vmdef"
if #arg == 0 then
    print("No argument specified.")
    return
end

for i = 1, #arg do
    local op = arg[i]
    print("opcode " .. op .. ":")
    print(string.sub(vmdef.bcnames, op*6+1, op*6+6))
end