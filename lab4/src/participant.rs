//!
//! participant.rs
//! Implementation of 2PC participant
//!
extern crate ipc_channel;
extern crate log;
extern crate rand;
extern crate stderrlog;

use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::Duration;
use std::thread;

use participant::rand::prelude::*;
use participant::ipc_channel::ipc::IpcReceiver as Receiver;
use participant::ipc_channel::ipc::TryRecvError;
use participant::ipc_channel::ipc::IpcSender as Sender;

use message::MessageType;
use message::ProtocolMessage;
use oplog;
///
/// ParticipantState
/// enum for Participant 2PC state machine
///
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ParticipantState {
    Quiescent,
    ReceivedP1,
    VotedAbort,
    VotedCommit,
    AwaitingGlobalDecision,
}

///
/// Participant
/// Structure for maintaining per-participant state and communication/synchronization objects to/from coordinator
///
#[derive(Debug)]
pub struct Participant {
    id_str: String,
    state: ParticipantState,
    log: oplog::OpLog,
    running: Arc<AtomicBool>,
    send_success_prob: f64,
    operation_success_prob: f64,
    tx: Sender<ProtocolMessage>,  // 新增 tx 字段
    coord_rx: Receiver<ProtocolMessage>,  // 新增 rx 字段
    total_ops: u32,
    success: u32,
    failures: u32,
    unknown: u32,
}

///
/// Participant
/// Implementation of participant for the 2PC protocol
/// Required:
/// 1. new -- Constructor
/// 2. pub fn report_status -- Reports number of committed/aborted/unknown for each participant
/// 3. pub fn protocol() -- Implements participant side protocol for 2PC
///
impl Participant {

    ///
    /// new()
    ///
    /// Return a new participant, ready to run the 2PC protocol with the coordinator.
    ///
    /// HINT: You may want to pass some channels or other communication
    ///       objects that enable coordinator->participant and participant->coordinator
    ///       messaging to this constructor.
    /// HINT: You may want to pass some global flags that indicate whether
    ///       the protocol is still running to this constructor. There are other
    ///       ways to communicate this, of course.
    ///
    pub fn new(
        id_str: String,
        log_path: String,
        r: Arc<AtomicBool>,
        send_success_prob: f64,
        operation_success_prob: f64,
        tx: Sender<ProtocolMessage>,  // 參數 tx
        coord_rx: Receiver<ProtocolMessage>,  // 參數 rx
        total_ops: u32,) -> Participant {

        Participant {
            id_str: format!("{}", id_str),
            state: ParticipantState::Quiescent,
            log: oplog::OpLog::new(log_path),
            running: r,
            send_success_prob: send_success_prob,
            operation_success_prob: operation_success_prob,
            // TODO
            tx,
            coord_rx,
            total_ops,
            success: 0,
            failures: 0,
            unknown:0,
        }
    }

    ///
    /// send()
    /// Send a protocol message to the coordinator. This can fail depending on
    /// the success probability. For testing purposes, make sure to not specify
    /// the -S flag so the default value of 1 is used for failproof sending.
    ///
    /// HINT: You will need to implement the actual sending
    ///
    pub fn send(&mut self, pm: ProtocolMessage) {
        let x: f64 = random();
        if x <= self.send_success_prob {
            // TODO: Send success
            // 嘗試傳送訊息，失敗則記錄錯誤
            match self.tx.send(pm) {
                Ok(()) => (),
                Err(error) => {
                    error!("{}::Could not send to coordinator", self.id_str);
                    panic!("{}::error: {:#?}", self.id_str, error);
                },
            }
        } else {
            // TODO: Send fail
            trace!("{}::Message dropped due to probability settings", self.id_str);
        }
    }
    // 獨立處理 ProtocolMessage 的生成和記錄
    fn handle_commit_decision(&mut self, coord_msg: &ProtocolMessage, message_type: MessageType) {
        if self.state == ParticipantState::AwaitingGlobalDecision {
            // Log the message with the specified type
            //self.log.append(message_type, coord_msg.txid.clone(), coord_msg.senderid.clone(), coord_msg.opid);
            self.log.append_msg_from_pm(coord_msg, message_type);

            match message_type {
                MessageType::CoordinatorCommit => self.success += 1,
                MessageType::CoordinatorAbort => self.failures += 1,
                _ => (),
            }
            self.state = ParticipantState::Quiescent;
        }
    }
    // Helper function to send a message with a new type
    fn send_message(&mut self, coord_msg: &ProtocolMessage, new_type: MessageType) {
        self.send(ProtocolMessage::generate(
            new_type,
            coord_msg.txid.clone(),
            self.id_str.clone(),
            coord_msg.opid,
        ));
    }
    ///
    /// perform_operation
    /// Perform the operation specified in the 2PC proposal,
    /// with some probability of success/failure determined by the
    /// command-line option success_probability.
    ///
    /// HINT: The code provided here is not complete--it provides some
    ///       tracing infrastructure and the probability logic.
    ///       Your implementation need not preserve the method signature
    ///       (it's ok to add parameters or return something other than
    ///       bool if it's more convenient for your design).
    ///
    //pub fn perform_operation(&mut self, request_option: &Option<ProtocolMessage>) -> bool {
    pub fn perform_operation(&mut self, request:&ProtocolMessage) -> bool {

        trace!("{}::Performing operation", self.id_str);
        let x: f64 = random();
        let success = x <= self.operation_success_prob;
        let new_type = if success { MessageType::ParticipantVoteCommit } else { MessageType::ParticipantVoteAbort };
        
        // 記錄操作日誌，Prepare/Ack階段
        self.log.append(new_type, request.txid.clone(), request.senderid.clone(), request.opid);

        let vote_type = if success {
        self.state = ParticipantState::VotedCommit;
            MessageType::ParticipantVoteCommit
        } else {
            self.state = ParticipantState::VotedAbort;
            MessageType::ParticipantVoteAbort
        };

        // 傳送投票結果, 參與者然後會根據結果向協調者傳送 ParticipantVoteCommit 或 ParticipantVoteAbort 訊息
        self.send_message(&request, vote_type);
        self.state = ParticipantState::AwaitingGlobalDecision;

        success
    }
    
    ///
    /// report_status()
    /// Report the abort/commit/unknown status (aggregate) of all transaction
    /// requests made by this coordinator before exiting.
    ///
    pub fn report_status(&mut self) {
        // TODO: Collect actual stats
        let success: u32 = self.success;
        let failures: u32 = self.failures;
        let unknown_ops = self.unknown;
        
        println!("{:16}:\tCommitted: {:6}\tAborted: {:6}\tUnknown: {:6}", self.id_str.clone(), success, failures, unknown_ops);
    }
    ///
    /// wait_for_exit_signal(&mut self)
    /// Wait until the running flag is set by the CTRL-C handler
    ///
    pub fn wait_for_exit_signal(&mut self) {
        trace!("{}::Waiting for exit signal", self.id_str.clone());

        // TODO*/
        //let sleep_interval = Duration::from_millis(300); // 初始休眠時間設置為500ms

        //while self.running.load(Ordering::SeqCst) {};
        /*while self.running.load(Ordering::SeqCst) {
            if let Ok(protocol_message) = self.coord_rx.try_recv() {
                if protocol_message.mtype == MessageType::CoordinatorExit {
                    trace!("{}::Received CoordinatorExit message, exiting.", self.id_str);
                    break;
                }
            }
            thread::sleep(sleep_interval);
        }*/

        
        /*最早的作法-》不好
        while self.running.load(Ordering::Relaxed) {
            if let Ok(protocol_message) = self.coord_rx.try_recv() {
                if protocol_message.mtype == MessageType::CoordinatorExit {
                    trace!("{}::Received CoordinatorExit message, exiting.", self.id_str);
                    break;
                }
            } else if let Err(TryRecvError::Empty) | Err(TryRecvError::IpcError(ipc_channel::ipc::IpcError::Disconnected)) = self.coord_rx.try_recv() {
                // 當通道沒有訊息或已斷開，跳過處理並使用增量休眠
            } else if let Err(error) = self.coord_rx.try_recv() {
                error!("{}::Unexpected error receiving from coordinator: {:?}", self.id_str, error);
                panic!("Unexpected error: {:?}", error);
            }

            // 增加休眠時間以減少繁忙等待，並設置最大休眠間隔
            thread::sleep(sleep_interval);
            sleep_interval = std::cmp::min(sleep_interval * 2, Duration::from_secs(2)); // 最高休眠時間2秒
        }*/

        //速度慢的用法, 好像不一定慢
        let mut sleep_interval = Duration::from_millis(300); // 初始休眠時間設置為500ms

        while self.running.load(Ordering::Relaxed) {
            match self.coord_rx.try_recv() {
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
        }

        trace!("{}::Exiting", self.id_str.clone());
    }

    
    ///
    /// protocol()
    /// Implements the participant side of the 2PC protocol
    /// HINT: If the simulation ends early, don't keep handling requests!
    /// HINT: Wait for some kind of exit signal before returning from the protocol!
    ///
    pub fn protocol(&mut self) {
        trace!("{}::Beginning protocol", self.id_str.clone());

        // TODO
        trace!("{}::Beginning protocol", self.id_str);

        let mut num_requests = 0;
        while self.running.load(Ordering::Relaxed) && num_requests < self.total_ops {
            // 接收協調者的提案
            let request = self.coord_rx.recv().expect("Failed to receive from coordinator");
            self.state = ParticipantState::ReceivedP1;

            // 執行操作並根據結果設置消息類型, 傳送投票結果
            self.perform_operation(&request);
            /*let operation_successful = self.perform_operation(&request);
            let vote_type = if operation_successful {
                self.state = ParticipantState::VotedCommit;
                MessageType::ParticipantVoteCommit
            } else {
                self.state = ParticipantState::VotedAbort;
                MessageType::ParticipantVoteAbort
            };

            // 傳送投票結果
            self.send_message(&request, vote_type);
            self.state = ParticipantState::AwaitingGlobalDecision;*/

            // 接收最終決策並記錄結果, 接收commit的訊息
            /*if let Some(global_decision) = self.coord_rx.recv().ok() {
                if global_decision.mtype == MessageType::CoordinatorCommit {
                    self.handle_commit_decision(&global_decision, MessageType::CoordinatorCommit);
                } else if global_decision.mtype == MessageType::CoordinatorAbort {
                    self.handle_commit_decision(&global_decision, MessageType::CoordinatorAbort);
                }
                // 若不符合以上條件，則不做任何操作
            }*/
            if let Some(global_decision) = self.coord_rx.recv().ok() {
                match global_decision.mtype {
                    MessageType::CoordinatorCommit => self.handle_commit_decision(&global_decision, MessageType::CoordinatorCommit),
                    MessageType::CoordinatorAbort => self.handle_commit_decision(&global_decision, MessageType::CoordinatorAbort),
                    _ => (),
                }
            }
            num_requests += 1;
        }

        self.wait_for_exit_signal();
        self.report_status();
    }
}
