#include "LudiStreamMod.h"

#include "LudiStreamServer.h"
#include "ll/api/mod/RegisterHelper.h"

namespace ludistream {

LudiStreamMod& LudiStreamMod::getInstance() {
    static LudiStreamMod instance;
    return instance;
}

bool LudiStreamMod::load() const {
    getSelf().getLogger().debug("Loading...");
    server::start();
    return true;
}

bool LudiStreamMod::enable() const {
    getSelf().getLogger().debug("Enabling...");
    return true;
}

bool LudiStreamMod::disable() const {
    getSelf().getLogger().debug("Disabling...");
    return true;
}

bool LudiStreamMod::unload() const {
    getSelf().getLogger().debug("Unloading...");
    return true;
}

} // namespace ludistream

LL_REGISTER_MOD(ludistream::LudiStreamMod, ludistream::LudiStreamMod::getInstance());
