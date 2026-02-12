package io.github.minebuilders.ludistream

import kotlin.io.path.createTempFile

/** `-1` represents an invalid handle. */
typealias PlayerHandle = Long

object LudiStreamServer {
    init {
        val os = System.getProperty("os.name").lowercase()
        val (prefix, suffix) = when {
            os.contains("win") -> "" to ".dll"
            os.contains("linux") -> "lib" to ".so"
            else -> error("Unsupported OS ($os)")
        }
        val libFilename = "${prefix}ludi_stream_server_core$suffix"
        val tempFile = createTempFile(prefix, suffix).toFile()
        tempFile.deleteOnExit()
        tempFile.outputStream().use { out ->
            javaClass.getResourceAsStream("/$libFilename")
                ?.use { it.copyTo(out) }
                ?: error("JNI not found in JAR ($libFilename)")
        }
        System.load(tempFile.absolutePath)
    }

    external fun start(port: Short, forward: Short = -1): Boolean
    external fun stop()
    external fun kick(playerId: PlayerHandle)
    external fun updatePlayerTransform(
        playerId: PlayerHandle,
        worldId: Short,
        x: Float, y: Float, z: Float,
        yaw: Float,
    )

    @Suppress("unused")
    @JvmStatic
    fun authenticate(token: String): PlayerHandle {
        println("KOTLIN recv: authenticate")
        println("    | $token")
        return 1919810
    }

    @Suppress("unused")
    @JvmStatic
    fun getPlayerName(playerId: PlayerHandle): String? {
        println("KOTLIN recv: get_player_name")
        println("    | $playerId")
        return "Test_$playerId"
    }

    @Suppress("unused")
    @JvmStatic
    fun onConnected(playerId: PlayerHandle): Unit = TODO()

    @Suppress("unused")
    @JvmStatic
    fun onDisconnected(playerId: PlayerHandle): Unit = TODO()
}
