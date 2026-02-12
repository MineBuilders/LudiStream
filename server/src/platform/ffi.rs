use crate::platform::PlayerHandle;
use std::ffi::{CString, c_char};

unsafe extern "C" {
    fn ludistream_platform_authenticate(token: *const c_char) -> PlayerHandle;
    fn ludistream_platform_get_player_name(
        player_id: PlayerHandle,
        buffer: *mut c_char,
        buffer_len: usize,
        out_len: *mut usize,
    ) -> bool;
    #[link_name = "ludistream_platform_on_connected"]
    pub fn on_connected(player_id: PlayerHandle);
    #[link_name = "ludistream_platform_on_disconnected"]
    pub fn on_disconnected(player_id: PlayerHandle);
}

pub unsafe fn authenticate(token: &str) -> PlayerHandle {
    let token_c = CString::new(token).unwrap();
    ludistream_platform_authenticate(token_c.as_ptr())
}

pub unsafe fn get_player_name(player_id: PlayerHandle) -> Option<String> {
    let mut buffer = Vec::<u8>::with_capacity(64);
    let mut out_len = 0;
    ludistream_platform_get_player_name(
        player_id,
        buffer.as_mut_ptr() as *mut i8,
        buffer.capacity(),
        &mut out_len,
    )
    .then(|| {
        buffer.set_len(out_len);
        String::from_utf8_unchecked(buffer)
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ludistream_server_start(port: i16, forward: i16) -> bool {
    println!("RUST recv: start");
    println!("    | port = {}", port);
    println!("    | forward = {}", forward);
    println!("RUST call: authenticate");
    let player_id = authenticate("THIS_IS_TOKEN_001");
    println!("RUST ret: authenticate");
    println!("    | {:?}", player_id);
    println!("RUST call: get_player_name");
    let player_name = get_player_name(player_id);
    println!("RUST ret: get_player_name");
    println!("    | {:?}", player_name);
    // todo!()
    true
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ludistream_server_stop() {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ludistream_server_kick(player_id: PlayerHandle) {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ludistream_server_update_player_transform(
    player_id: PlayerHandle,
    world_id: i16,
    x: f32,
    y: f32,
    z: f32,
    yaw: f32,
) {
    todo!()
}
