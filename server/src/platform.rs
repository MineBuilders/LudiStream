#[cfg(feature = "ffi")]
mod ffi;
#[cfg(feature = "jni")]
mod jni;

#[cfg(feature = "ffi")]
use ffi as platform;
#[cfg(feature = "jni")]
use jni as platform;

#[cfg(all(feature = "ffi", feature = "jni"))]
compile_error!("features `ffi` and `jni` cannot be enabled at the same time.");

#[repr(transparent)]
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct PlayerHandle(pub i64);
impl PlayerHandle {
    pub fn is_valid(&self) -> bool {
        self.0 != -1
    }
}

#[inline]
pub fn authenticate(token: &str) -> PlayerHandle {
    unsafe { platform::authenticate(token) }
}

#[inline]
pub fn get_player_name(player_id: PlayerHandle) -> Option<String> {
    unsafe { platform::get_player_name(player_id) }
}

#[inline]
pub fn notify_connected(player_id: PlayerHandle) {
    unsafe { platform::on_connected(player_id) }
}

#[inline]
pub fn notify_disconnected(player_id: PlayerHandle) {
    unsafe { platform::on_disconnected(player_id) }
}
