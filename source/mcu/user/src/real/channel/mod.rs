mod message;
mod raw;
mod wrapper;

pub use message::{ReadGuard, Reader, UninitWriteGuard, WriteGuard, Writer};
pub use wrapper::{Channel, ReadChannel, WriteChannel};

const RPMSG_REMOTE_ID: u32 = 0;
