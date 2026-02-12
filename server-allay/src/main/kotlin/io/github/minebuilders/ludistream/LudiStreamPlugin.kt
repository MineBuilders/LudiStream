package io.github.minebuilders.ludistream

import org.allaymc.api.plugin.Plugin

@Suppress("unused")
class LudiStreamPlugin : Plugin() {
    override fun onLoad() {
        println("KOTLIN call: start")
        val success = LudiStreamServer.start(114, 514)
        println("KOTLIN ret: start")
        println("    | $success")
    }
}
