#![allow(unsafe_op_in_unsafe_fn)]

#[cfg(any(feature = "ffi", feature = "jni"))]
mod platform;
