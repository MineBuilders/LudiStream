use crate::platform::PlayerHandle;
use jni::objects::{JClass, JString, JValue};
use jni::sys::{jboolean, jfloat, jint, jlong, jshort};
use jni::{JNIEnv, JavaVM};
use std::sync::OnceLock;

static CLASS_NAME: &str = "io/github/minebuilders/ludistream/LudiStreamServer";
static JVM: OnceLock<JavaVM> = OnceLock::new();

#[allow(non_snake_case)]
#[unsafe(no_mangle)]
extern "system" fn JNI_OnLoad(vm: JavaVM, _reserved: *mut std::ffi::c_void) -> jint {
    let _ = JVM.set(vm);
    jni::sys::JNI_VERSION_1_6
}

unsafe fn get_env<'local>() -> JNIEnv<'local> {
    JVM.get()
        .expect("JVM not initialized")
        .attach_current_thread_as_daemon()
        .expect("Failed to attach thread")
}

pub unsafe fn authenticate(token: &str) -> PlayerHandle {
    let mut env = get_env();
    let token_j = env.new_string(token).unwrap();
    let args = &[JValue::Object(&token_j)];
    let result = env
        .call_static_method(CLASS_NAME, "authenticate", "(Ljava/lang/String;)J", args)
        .expect("Failed to call authenticate")
        .j()
        .expect("Return value was not a long");
    PlayerHandle(result)
}

pub unsafe fn get_player_name(player_id: PlayerHandle) -> Option<String> {
    let mut env = get_env();
    let args = &[JValue::Long(player_id.0)];
    let result_j = env
        .call_static_method(CLASS_NAME, "getPlayerName", "(J)Ljava/lang/String;", args)
        .expect("Failed to call getPlayerName")
        .l()
        .expect("Return value was not an object");
    if result_j.is_null() {
        return None;
    }
    Some(
        env.get_string(&JString::from(result_j))
            .expect("Failed to get string")
            .into(),
    )
}

pub unsafe fn on_connected(player_id: PlayerHandle) {
    let args = &[JValue::Long(player_id.0)];
    get_env()
        .call_static_method(CLASS_NAME, "onConnected", "(J)V", args)
        .expect("Failed to call onConnected");
}

pub unsafe fn on_disconnected(player_id: PlayerHandle) {
    let args = &[JValue::Long(player_id.0)];
    get_env()
        .call_static_method(CLASS_NAME, "onDisconnected", "(J)V", args)
        .expect("Failed to call onDisconnected");
}

#[allow(non_snake_case)]
#[unsafe(no_mangle)]
pub unsafe extern "system" fn Java_io_github_minebuilders_ludistream_LudiStreamServer_start(
    _env: JNIEnv,
    _class: JClass,
    port: jshort,
    forward: jshort,
) -> jboolean {
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
    // todo!();
    u8::from(true)
}

#[allow(non_snake_case)]
#[unsafe(no_mangle)]
pub unsafe extern "system" fn Java_io_github_minebuilders_ludistream_LudiStreamServer_stop(
    _env: JNIEnv,
    _class: JClass,
) {
    todo!();
}

#[allow(non_snake_case)]
#[unsafe(no_mangle)]
pub unsafe extern "system" fn Java_io_github_minebuilders_ludistream_LudiStreamServer_kick(
    _env: JNIEnv,
    _class: JClass,
    player_id: jlong,
) {
    todo!();
}

#[allow(non_snake_case)]
#[unsafe(no_mangle)]
pub unsafe extern "system" fn Java_io_github_minebuilders_ludistream_LudiStreamServer_updatePlayerTransform(
    _env: JNIEnv,
    _class: JClass,
    player_id: jlong,
    world_id: jshort,
    x: jfloat,
    y: jfloat,
    z: jfloat,
    yaw: jfloat,
) {
    todo!();
}
