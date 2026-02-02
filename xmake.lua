add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

rule("levilamina-mod-packer")
after_build(function(target)
    local cargo_toml = io.readfile(path.join(os.projectdir(), "Cargo.toml"))
    local mod_name = target:name()
    local mod_file = path.filename(target:targetfile())
    local mod_version = cargo_toml:match("%[workspace%.package%].-version%s*=%s*\"([^\"]+)\"")
        or cargo_toml:match("version%s*=%s*\"([^\"]+)\"")
    if not mod_version then
        raise("Failed to parse version from Cargo.toml")
    end
    import("core.project.config")
    local mod_platform = config.get("target_type")
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

includes("client")
