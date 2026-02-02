#include "LudiStreamClient.h"

#include "ll/api/mod/RegisterHelper.h"

namespace ludistream {

LudiStreamClient& LudiStreamClient::getInstance() {
    static LudiStreamClient instance;
    return instance;
}

bool LudiStreamClient::load() const {
    getSelf().getLogger().debug("Loading...");
    return true;
}

bool LudiStreamClient::enable() const {
    getSelf().getLogger().debug("Enabling...");
    return true;
}

} // namespace ludistream

LL_REGISTER_MOD(ludistream::LudiStreamClient, ludistream::LudiStreamClient::getInstance());
