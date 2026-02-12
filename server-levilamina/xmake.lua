add_rules("mode.debug", "mode.release")

add_requires("levilamina~server", {
    alias = "levilamina-server",
    configs = {target_type = "server"}
})

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("ludi-stream-server-levilamina")
    set_values("target_type", "server")
    add_rules("server-core-linker")
    add_rules("levilamina-mod-linker")
    add_rules("levilamina-mod-packer")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE")
    add_packages("levilamina-server")
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")
