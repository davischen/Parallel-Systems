//!
//! client.rs
//! Implementation of 2PC client
//!
extern crate ipc_channel;
extern crate log;
extern crate stderrlog;

use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
//use std::collections::HashMap;

use client::ipc_channel::ipc::IpcReceiver as Receiver;
use client::ipc_channel::ipc::IpcSender as Sender;
//use client::ipc_channel::ipc::TryRecvError;

use message;
use message::MessageType;
use message::ProtocolMessage;

use std::thread;
use std::time::Duration;

// Client state and primitives for communicating with the coordinator
#[derive(Debug)]
pub struct Client {
    pub id_str: String,
    pub running: Arc<AtomicBool>,
    pub num_requests: u32,
    pub coord_tx: Sender<ProtocolMessage>,  // 新增 tx
    pub rx: Receiver<ProtocolMessage>,  // 新增 rx
    pub success: u32,
    pub failures: u32,
    pub unknown: u32,
}

///
/// Client Implementation
/// Required:
/// 1. new -- constructor
/// 2. pub fn report_status -- Reports number of committed/aborted/unknown
/// 3. pub fn protocol(&mut self, n_requests: i32) -- Implements client side protocol
///
impl Client {

    ///
    /// new()
    ///
    /// Constructs and returns a new client, ready to run the 2PC protocol
    /// with the coordinator.
    ///
    /// HINT: You may want to pass some channels or other communication
    ///       objects that enable coordinator->client and client->coordinator
    ///       messaging to this constructor.
    /// HINT: You may want to pass some global flags that indicate whether
    ///       the protocol is still running to this constructor
    ///
    pub fn new(id_str: String,
               running: Arc<AtomicBool>,
                coord_tx: Sender<ProtocolMessage>,
                rx: Receiver<ProtocolMessage>) -> Client {
        Client {
            id_str: format!{"client_{}",id_str},
            running: running,
            num_requests: 0,
            // TODO
            coord_tx,
            rx,
            success: 0,
            failures: 0,
            unknown:0,
        }
    }

    ///
    /// wait_for_exit_signal(&mut self)
    /// Wait until the running flag is set by the CTRL-C handler
    ///
    pub fn wait_for_exit_signal(&mut self) {
        trace!("{}::Waiting for exit signal", self.id_str.clone());

        // TODO
        let sleep_interval = Duration::from_millis(300); // 初始休眠時間設置為500ms
        while self.running.load(Ordering::Relaxed) {
            if let Ok(protocol_message) = self.rx.try_recv() {
                if protocol_message.mtype == MessageType::CoordinatorExit {
                    trace!("{}::Received CoordinatorExit message, exiting.", self.id_str);
                    break;
                }
            }
            thread::sleep(sleep_interval);
        }
        /*舊的作法，速度慢
        while self.running.load(Ordering::Relaxed) {
            match self.rx.try_recv() {
                Ok(protocol_message) => {
                    if protocol_message.mtype == MessageType::CoordinatorExit {
                        trace!("{}::Received CoordinatorExit message, exiting.", self.id_str);
                        break;
                    }
                },
                Err(TryRecvError::Empty | TryRecvError::IpcError(ipc_channel::ipc::IpcError::Disconnected)) => {
                    // 當通道沒有訊息或已斷開，跳過處理並使用增量休眠
                },
                Err(error) => {
                    error!("{}::Unexpected error receiving from coordinator: {:?}", self.id_str, error);
                    panic!("Unexpected error: {:?}", error);
                }
            }

            // 增加休眠時間以減少繁忙等待，並設置最大休眠間隔
            thread::sleep(sleep_interval);
            sleep_interval = std::cmp::min(sleep_interval * 2, Duration::from_secs(2)); // 最高休眠時間2秒
        }*/

        trace!("{}::Exiting", self.id_str.clone());
       
    }

    ///
    /// send_next_operation(&mut self)
    /// Send the next operation to the coordinator
    ///
    pub fn send_next_operation(&mut self) {
        // 增加請求計數並生成唯一的 TXID
        // Create a new request with a unique TXID.
        self.num_requests = self.num_requests + 1;
        let txid = format!("{}_op_{}", self.id_str.clone(), self.num_requests);
        // 建立 ClientRequest 類型的協議消息
        let pm = message::ProtocolMessage::generate(
            message::MessageType::ClientRequest,
            txid.clone(),
            self.id_str.clone(),
            self.num_requests);
        info!("{}::Sending operation #{}", self.id_str.clone(), self.num_requests);

        // TODO
        if let Err(e) = self.coord_tx.send(pm) {
            error!("{}::Failed to send operation #{}: {:?}", self.id_str, self.num_requests, e);
        }
        
        trace!("{}::Sent operation #{}", self.id_str, self.num_requests);
    }

    ///
    /// recv_result()
    /// Wait for the coordinator to respond with the result for the
    /// last issued request. Note that we assume the coordinator does
    /// not fail in this simulation
    ///
    pub fn recv_result(&mut self) {
        info!("{}::Receiving Coordinator Result", self.id_str);
        // TODO
        // 使用 match 來處理接收結果，方便處理錯誤情況
        let result = match self.rx.recv() {
            Ok(protocol_message) => protocol_message,
            Err(error) => {
                error!("{}::Could not receive from coordinator", self.id_str);
                panic!("{}::error: {:#?}", self.id_str, error);
            },
        };        
            
        // 根據消息類型更新計數
        match result.mtype {
            message::MessageType::ClientResultCommit => self.success += 1,
            message::MessageType::ClientResultAbort => {
                self.failures += 1;
                if result.txid.contains("unknown") {
                    self.unknown += 1;
                }
            },
            _ => {},
        }
        
        /*info!("{}::Received Coordinator Response: {:?}", self.id_str, result);
        let result = match self.rx.recv() {
            Ok(protocol_message) => protocol_message,
            Err(error) => {
                //self.report_status();
                error!("{}::Could not receive from coordinator", self.id_str);
                panic!("{}::error: {:#?}", self.id_str, error);
            },
        };        
        
        if result.mtype == message::MessageType::ClientResultCommit {
            self.success += 1;
        }

        if result.mtype == message::MessageType::ClientResultAbort {
            self.failures += 1;
            if result.txid.contains("unknown"){
                self.unknown += 1;
            }
        }

        info!("{}::Received Coordinator Response: {:?}", self.id_str.clone(), result);*/
    }

    ///
    /// report_status()
    /// Report the abort/commit/unknown status (aggregate) of all transaction
    /// requests made by this client before exiting.
    ///
    pub fn report_status(&mut self) {
        // TODO: Collect actual stats
        let success = self.success;
        let failures = self.failures;
        let unknown_ops =self.unknown;

        println!("{:16}:\tCommitted: {:6}\tAborted: {:6}\tUnknown: {:6}", self.id_str.clone(), success, failures, unknown_ops);
    }

    ///
    /// protocol()
    /// Implements the client side of the 2PC protocol
    /// HINT: if the simulation ends early, don't keep issuing requests!
    /// HINT: if you've issued all your requests, wait for some kind of
    ///       exit signal before returning from the protocol method!
    ///
    pub fn protocol(&mut self, n_requests: u32) {

        // TODO
        /*for _ in 0..n_requests {
            if self.running.load(Ordering::SeqCst) {
                self.send_next_operation();
                info!("{}::Receiving Coordinator result", self.id_str.clone());
                self.recv_result();
            } else {
                break;
            }
        }
        info!("Client {} done.", self.id_str);
        self.wait_for_exit_signal();
        self.report_status();*/
        debug!("{}::protocol(): Enter Client Protocol", self.id_str);
        for _ in 0..n_requests {
            if self.running.load(Ordering::Relaxed) {
                self.send_next_operation();
                self.recv_result();
            } else {
                break;
            }
        }
        info!("Client {} done.", self.id_str);

        self.wait_for_exit_signal();
        self.report_status();
    }
}
