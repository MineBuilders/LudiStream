import org.gradle.internal.os.OperatingSystem

plugins {
    // this is necessary to avoid the plugins to be loaded multiple times
    // in each subproject's classloader
    alias(libs.plugins.kotlin.jvm) apply false
}

group = "io.github.minebuilders.ludistream"
description = "An in-game audio streaming & voice system " +
        "for Minecraft Bedrock Edition, " +
        "powered by the client mod " +
        "with vanilla-compatible fallback!"

version = Regex("""version\s*=\s*"([^"]+)"""")
    .find(file("Cargo.toml").readText())
    ?.groupValues
    ?.get(1)
    ?: error("Failed to parse version from Cargo.toml")

subprojects {
    group = rootProject.group
    description = rootProject.description
    version = rootProject.version
}

tasks.register<Exec>("buildServerCore") {
    val srcDir = rootProject.file("server")
    val manifest = srcDir.resolve("Cargo.toml")
    val manifestBak = srcDir.resolve("Cargo.toml.bak")
    doFirst {
        manifest.copyTo(manifestBak, overwrite = true)
        val data = manifest.readText().replace("# JNI # ", "")
        manifest.writeText(data)
    }
    doLast {
        assert(manifestBak.exists())
        manifestBak.copyTo(manifest, overwrite = true)
        manifestBak.delete()
    }
    val targetDir = rootProject.file("target/feature-jni")
    workingDir = srcDir
    environment("CARGO_TARGET_DIR", targetDir)
    commandLine("cargo", "build", "--features", "jni", "--release")
    val libFilename = OperatingSystem.current()
        .getSharedLibraryName("ludi_stream_server_core")
    inputs.dir(srcDir)
    outputs.file(targetDir.resolve("release/$libFilename"))
}
