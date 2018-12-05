require "lxp"

local count = 0
callbacks = {
    StartElement = function (parser, name)
        io.write("+ ", string.rep(" ", count), name, "\n")
        count = count + 1
    end,
    EndElement = function (parser, name)
        count = count - 1
        io.write("- ", string.rep(" ", count), name, "\n")
    end
}

p = lxp.new(callbacks)

for l in io.lines() do  -- iterate lines
    p:parse(l)          -- parses the line
    p:parse("\n")       -- parses the end of line
end
p:parse()               -- finishes the document
p:close()               -- closes the parser