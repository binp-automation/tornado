use crate::{
    channel::Channel,
    config::Point,
    epics,
    proto::{AppMsg, AppMsgMut, AppMsgTag},
};
use async_std::sync::{Arc, Mutex};
use ferrite::{
    channel::MsgWriter,
    misc::{AsyncCounter, DoubleVec},
};
use flatty::portable::{le::I32, NativeCast};
use futures::join;
use std::{future::Future, ops::Deref};

pub struct Dac {
    pub channel: Arc<Mutex<MsgWriter<AppMsg, Channel>>>,
    pub epics: epics::Dac,
}

pub struct DacHandle {
    requested: Arc<AsyncCounter>,
}

impl Dac {
    pub fn run(self) -> (impl Future<Output = ()>, DacHandle) {
        let channel = self.channel;
        let max_len = self.epics.array.max_len();

        let buffer = DoubleVec::<Point>::new(max_len);
        let (mut read_buffer, write_buffer) = buffer.split();
        let requested = Arc::new(AsyncCounter::new(0));

        let handle = DacHandle {
            requested: requested.clone(),
        };

        let mut epics_array = self.epics.array;
        let array_write_buffer = write_buffer.clone();
        let array_read_loop = async move {
            loop {
                let epics_guard = epics_array.read_in_place().await;
                {
                    let mut buffer_guard = array_write_buffer.lock().await;
                    buffer_guard.clear();
                    buffer_guard.extend(epics_guard.iter().map(|x| *x as i32));
                    println!("[app] array_read: {:?}", epics_guard.deref());
                }
                array_write_buffer.set_ready();
            }
        };

        let mut epics_scalar = self.epics.scalar;
        let scalar_read_loop = async move {
            loop {
                let value = epics_scalar.read().await;
                {
                    let mut buffer_guard = write_buffer.lock().await;
                    buffer_guard.clear();
                    buffer_guard.push(value as i32);
                    println!("[app] scalar_read: {}", value);
                }
                write_buffer.set_ready();
            }
        };

        let msg_send_loop = async move {
            let mut slice = &[][..];
            loop {
                let requested_count = requested.wait_sub(1..=max_len).await;
                println!("[app] requested_count: {}", requested_count);
                let mut channel_guard = channel.lock().await;
                let mut msg_guard = channel_guard.init_default_msg().unwrap();
                msg_guard.reset_tag(AppMsgTag::DacData).unwrap();
                if let AppMsgMut::DacData(msg) = msg_guard.as_mut() {
                    while msg.points.len() < requested_count {
                        if slice.is_empty() {
                            read_buffer.try_swap().await;
                            slice = read_buffer.as_slice();
                        }
                        let count = msg.points.extend_from_iter(slice.iter().map(|x| I32::from_native(*x)));
                        slice = &slice[count..];
                    }
                    println!("[app] points_send: {:?}", &msg.points);
                } else {
                    unreachable!();
                }
                msg_guard.write().await.unwrap();
            }
        };

        (
            async move {
                join!(array_read_loop, scalar_read_loop, msg_send_loop);
            },
            handle,
        )
    }
}

impl DacHandle {
    pub fn request(&self, count: usize) {
        println!("[app] request: {}", count);
        self.requested.add(count);
    }
}
