local func = require "func"

local imap = func.imap
local chains = func.chains
local list = func.list

local dirs = {...}

local function find_source(dir)
    local prefix = io.popen('realpath ' .. dir):read()
    local proc = io.popen(string.format([[find %s -name "*.lua" -type f]], prefix))
    local function gen_source(path)
        return {
            name = string.sub(path, #prefix + 2, -(#'.lua' + 1)),
            bytes = string.dump(loadfile(path))
        }
    end
    return imap(gen_source, proc:lines())
end

local function xor_string(s)
    local key = string.format("qdr+%d-key", string.len(s))
    local index = 1
    local char_byte = {}
    while index <= string.len(s) do
        local sed = string.byte(key, ((index - 1) % string.len(key)) + 1)
        table.insert(char_byte, string.byte(s, index) ~ sed)
        index = index + 1
    end
    return string.char(table.unpack(char_byte))
end

local source_list = list(chains(imap(find_source, dirs)))

local hdr = {}
local codes = {}

for _, v in ipairs(source_list) do
    local code = string.pack('s4', xor_string(v.bytes))
    local name = xor_string(string.gsub(v.name, '/', '_'))
    table.insert(hdr, string.pack('s4I4', name, #code))
    table.insert(codes, code)
end

local bin_f = io.open('out.bin', 'w')

bin_f:write('~qdr~')
bin_f:write(table.concat(hdr))
bin_f:write(string.pack('s4', ''))
bin_f:write(table.concat(codes))

bin_f:close()
