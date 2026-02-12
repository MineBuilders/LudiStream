add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

rule("levilamina-mod-linker")
on_config(function(target)
    target:add("shflags", "/DELAYLOAD:bedrock_runtime.dll")
end)
before_link(function(target)
    import("lib.detect.find_file")
    import("core.project.config")
    cprint("${bright green}[Levilamina Mod Linker]: ${reset}running prelink for " .. target:name() .. "...")
    local plat = config.get("plat") or "windows"
    local arch = config.get("arch") or "x64"
    local target_type = target:values("target_type") or "server"

    local outdir = path.join(config.builddir(), ".prelink", target:name())
    local libdir = path.join(outdir, "lib")
    os.mkdir(libdir)

    local paths = target:pkg("levilamina-" .. target_type):envs().PATH
    local data = assert(find_file("bedrock_runtime_data", paths), "Cannot find bedrock_runtime_data")
    local link = assert(find_file("prelink.exe", paths), "Cannot find prelink.exe")
    local inputs = table.copy(target:objectfiles())
    for _, dep in ipairs(target:orderdeps()) do
        if dep:kind() == "static" then
            local libfile = dep:targetfile()
            if libfile and os.isfile(libfile) then
                table.insert(inputs, libfile)
            end
        end
    end
    for _, pkg in ipairs(target:orderpkgs()) do
        for _, linkdir in ipairs(pkg:get("linkdirs") or {}) do
            for _, linkname in ipairs(pkg:get("links") or {}) do
                local libfile = path.join(linkdir, linkname .. ".lib")
                if os.isfile(libfile) then
                    table.insert(inputs, libfile)
                end
            end
        end
    end

    --cprint(string.format(
    --    "${dim green}[Levilamina Mod Linker]: ${reset dim}prelinking %s (%s-%s-%s) with %s",
    --    target:name(), target_type, plat, arch, data))
    os.runv(link, {
        string.format("%s-%s-%s", target_type, plat, arch),
        outdir,
        data,
        table.unpack(inputs)
    })

    target:data_set("levilamina_prelink_dir", libdir)
    target:add("linkdirs", libdir)
    target:add("links", "bedrock_runtime_api")
end)
after_build(function(target)
    os.rm(path.join(target:data("levilamina_prelink_dir"), "*.lib"))
end)
rule_end()

rule("levilamina-mod-packer")
after_build(function(target)
    local cargo_toml = io.readfile(path.join(os.projectdir(), "Cargo.toml"))
    local mod_name = target:name()
    local mod_file = path.filename(target:targetfile())
    local mod_version = assert(cargo_toml:match("version%s*=%s*\"([^\"]+)\""),
        "Failed to parse version from Cargo.toml")
    import("core.project.config")
    local mod_platform = target:values("target_type") or "server"
    local manifest = string.format([=[{
    "name": "%s",
    "entry": "%s",
    "version": "%s",
    "type": "native",
    "platform": "%s"
}]=], mod_name, mod_file, mod_version, mod_platform)
    local output_dir = path.join(os.projectdir(), "bin", mod_name)
    os.mkdir(output_dir)
    local manifest_file = path.join(output_dir, "manifest.json")
    io.writefile(manifest_file, manifest)

    local ori_target_file = target:targetfile()
    local target_file = path.join(output_dir, mod_file)
    os.cp(ori_target_file, target_file)

    local ori_pdb_file = path.join(path.directory(ori_target_file), path.basename(ori_target_file) .. ".pdb")
    local pdb_file = path.join(output_dir, path.basename(mod_file) .. ".pdb")
    if os.isfile(ori_pdb_file) then
        os.cp(ori_pdb_file, pdb_file)
    end

    cprint("${bright green}[LeviLamina Mod Packer]: ${reset}mod generated to " .. output_dir)
end)
rule_end()

rule("server-core-linker")
on_load(function(target)
    import("core.project.config")
    local mode = config.mode() or "release"
    local lib_src_dir = path.join(os.projectdir(), "server")
    local lib_target_dir = path.join(os.projectdir(), "target", "feature-ffi")
    local lib_profile_dir = path.join(lib_target_dir, mode == "debug" and "debug" or "release")
    local lib_name = "ludi_stream_server_core"
    local lib_static_file
    if target:is_plat("windows") then
        lib_static_file = path.join(lib_profile_dir, lib_name .. ".lib")
    else
        lib_static_file = path.join(lib_profile_dir, "lib" .. lib_name .. ".a")
    end
    target:data_set("server_core_src_dir", lib_src_dir)
    target:data_set("server_core_target_dir", lib_target_dir)
    target:data_set("server_core_static_file", lib_static_file)
    target:add("includedirs", path.join(lib_src_dir, "include"))
    target:add("linkdirs", lib_profile_dir)
    target:add("links", lib_name)
    if target:is_plat("windows") then
        target:add("syslinks", "ws2_32", "userenv", "ntdll", "advapi32", "bcrypt")
    end
end)
before_link(function(target)
    import("lib.detect.find_tool")
    local cargo = assert(find_tool("cargo"), "Cargo not found")
    local cargo_args = {"build", "--features", "ffi"}
    import("core.project.config")
    local mode = config.mode() or "release"
    if mode ~= "debug" then
        table.insert(cargo_args, "--release")
    end

    local lib_src_dir = target:data("server_core_src_dir")
    local cargo_toml = path.join(lib_src_dir, "Cargo.toml")
    local cargo_toml_bak = path.join(lib_src_dir, "Cargo.toml.bak")
    os.cp(cargo_toml, cargo_toml_bak)
    local cargo_toml_data = io.readfile(cargo_toml):gsub("# FFI # ", "")
    io.writefile(cargo_toml, cargo_toml_data)

    local lib_target_dir = target:data("server_core_target_dir")
    cprint("${bright green}[Server Core]: ${reset}building static library...")
    os.vrunv(cargo.program, cargo_args, {
        curdir = lib_src_dir,
        envs = {CARGO_TARGET_DIR = lib_target_dir}
    })
    os.cp(cargo_toml_bak, cargo_toml)
    os.rm(cargo_toml_bak)

    local lib_static_file = target:data("server_core_static_file")
    if not os.isfile(lib_static_file) then
        raise("Failed to build server core: " .. lib_static_file)
    end
    cprint("${bright green}[Server Core]: ${reset}built successfully")
end)
rule_end()


includes("client")
includes("server-levilamina")
