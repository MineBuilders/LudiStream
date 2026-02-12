#pragma once

#include "ll/api/mod/NativeMod.h"

namespace ludistream {

class LudiStreamMod {

public:
    static LudiStreamMod& getInstance();

    LudiStreamMod() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    [[nodiscard]] bool load() const;

    /// @return True if the mod is enabled successfully.
    [[nodiscard]] bool enable() const;

    /// @return True if the mod is disabled successfully.
    [[nodiscard]] bool disable() const;

    /// @return True if the mod is unloaded successfully.
    [[nodiscard]] bool unload() const;

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace ludistream
