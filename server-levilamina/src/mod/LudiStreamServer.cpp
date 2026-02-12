#include "LudiStreamServer.h"

#include "ludistream/ServerCommon.h"
#include <iostream>

extern "C" {

PlayerHandle ludistream_platform_authenticate(const char* token) {
    std::cout << "CPP recv: authenticate" << std::endl;
    std::cout << "    | " << token << std::endl;
    return 1919810;
}
bool ludistream_platform_get_player_name(
    const PlayerHandle playerId,
    char*              buffer,
    const size_t       bufferLen,
    size_t*            outLen
) {
    std::cout << "CPP recv: get_player_name" << std::endl;
    std::cout << "    | " << playerId << std::endl;
    const std::string name = "Test_" + std::to_string(playerId);

    if (name.size() > bufferLen) return false;
    std::memcpy(buffer, name.data(), name.size());
    *outLen = name.size();
    return true;
}
void ludistream_platform_on_connected(const PlayerHandle playerId) {
    std::cout << "CPP recv: on_connected" << std::endl;
    std::cout << "    | " << playerId << std::endl;
}
void ludistream_platform_on_disconnected(const PlayerHandle playerId) {
    std::cout << "CPP recv: on_disconnected" << std::endl;
    std::cout << "    | " << playerId << std::endl;
}
}

namespace ludistream::server {

void start() {
    std::cout << "CPP call: start" << std::endl;
    const auto success = ludistream_server_start(114, 514);
    std::cout << "CPP ret: start" << std::endl;
    std::cout << "    | " << success << std::endl;
}

} // namespace ludistream::server
