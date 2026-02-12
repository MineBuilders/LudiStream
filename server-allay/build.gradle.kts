plugins {
    alias(libs.plugins.kotlin.jvm)
    alias(libs.plugins.allaymc.gradle)
}

allay {
    api = "0.24.0"

    plugin {
        entrance = ".LudiStreamPlugin"
        name = "LudiStream"
        authors += "Cdm2883"
        website = "https://github.com/MineBuilders/LudiStream"
    }
}

val packServerCore by tasks.registering(Copy::class) {
    val buildServerCore = rootProject.tasks.named<Exec>("buildServerCore")
    dependsOn(buildServerCore)
    from(buildServerCore.map { it.outputs.files })
    into(layout.buildDirectory.dir("resources/main"))
}

tasks.named("processResources") {
    dependsOn(packServerCore)
}

val packPlugin by tasks.registering(Copy::class) {
    from(tasks.named<Jar>("shadowJar").map { it.archiveFile })
    into(rootProject.file("bin"))
    rename { "ludi-stream-server-allay.jar" }
}

afterEvaluate {
    tasks.named("shadowJar") { finalizedBy(packPlugin) }
}
