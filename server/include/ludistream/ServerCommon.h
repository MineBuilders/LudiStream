#pragma once
#include <memory>

/** `-1` represents an invalid handle. */
using PlayerHandle = int64_t;

extern "C" {

PlayerHandle ludistream_platform_authenticate(const char* token);
bool         ludistream_platform_get_player_name(PlayerHandle playerId, char* buffer, size_t bufferLen, size_t* outLen);
void         ludistream_platform_on_connected(PlayerHandle playerId);
void         ludistream_platform_on_disconnected(PlayerHandle playerId);

bool ludistream_server_start(int16_t port, int16_t forward);
void ludistream_server_stop();
void ludistream_server_kick(PlayerHandle playerId);
void ludistream_server_update_player_transform(
    PlayerHandle playerId,
    int16_t      worldId,
    float        x,
    float        y,
    float        z,
    float        yaw
);
}
