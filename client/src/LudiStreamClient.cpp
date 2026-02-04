#include "LudiStreamClient.h"

#include "ll/api/mod/RegisterHelper.h"
#include "ui/ImGui.h"

namespace ludistream {

LudiStreamClient& LudiStreamClient::getInstance() {
    static LudiStreamClient instance;
    return instance;
}

bool LudiStreamClient::load() const {
    getSelf().getLogger().debug("Loading...");
    ui::prepareImGui();
    return true;
}

} // namespace ludistream

LL_REGISTER_MOD(ludistream::LudiStreamClient, ludistream::LudiStreamClient::getInstance());
