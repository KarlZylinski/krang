local lfs = require "lfs"

local files_to_build = {}

function string.ends_with(str, e)
   return e == '' or string.sub(str,-string.len(e)) == e
end

function string.replace_end(str, n, new)
    local str_len = string.len(str)
    local new_str_end = str_len - n

    if new_str_end < 1 then
        new_str_end = 1
    end

    return str:sub(1, new_str_end) .. new
end

for filename in lfs.dir(".") do
    if string.ends_with(filename, ".cpp") then
        table.insert(files_to_build, filename)
    end
end

function arg_contain(str)
  for _, value in pairs(arg) do
    if value == str then
      return true
    end
  end
  return false
end

local set_env = arg_contain("set_env")
local build = arg_contain("build")
local run = arg_contain("run")
local use_debug = arg_contain("use_debug")

function run_or_die(cmd)
    if os.execute(cmd) ~= 0 then
        os.exit(1)
    end
end

if set_env then
    local env_number = arg_contain("env_number_110") and "110" or arg_contain("env_number_120") and "120" or arg_contain("env_number_130") and "130" or "140"
    local vs_dir = os.getenv("VS" .. env_number .. "COMNTOOLS")

    if vs_dir == nil then
        print("Could not find visual studio.")
        os.exit(1)
    end

    run_or_die("\"" .. vs_dir .. "..\\..\\VC\\vcvarsall.bat\" amd64")
end

if build then
    lfs.mkdir("build")

    local object_files = ""
    local extra_compile_opts = "/Os"
    local extra_link_opts = "/debug"

    if use_debug then
        extra_compile_opts = "/D DEBUG"
        extra_link_opts = "/debug"
    end

    for _, filename in ipairs(files_to_build) do
        local object_filename = "build\\" .. string.replace_end(filename, 3, "o")

        local build_cmd = "cl.exe /FI helpers.h /D _HAS_EXCEPTIONS=0 /nologo /W4 /WX /Gm /EHsc /TP /wd4505 /wd4201 /wd4100 /c /D _CRT_SECURE_NO_WARNINGS /Zi /MTd " .. extra_compile_opts .. " /Fo" .. object_filename .. " " .. filename
        run_or_die(build_cmd)
        object_files = object_files .. object_filename .. " "
    end

    if #object_files > 0 then
        local link_cmd = "link.exe " .. extra_link_opts .. " /subsystem:console /entry:mainCRTStartup dbghelp.lib user32.lib /out:krang.exe " .. object_files
        run_or_die(link_cmd)
    end
end

if run then
    run_or_die("skugga.exe")
end
