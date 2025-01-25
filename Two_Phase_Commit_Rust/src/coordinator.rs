//!
//! coordinator.rs
//! Implementation of 2PC coordinator
//!
extern crate log;
extern crate stderrlog;
extern crate rand;
extern crate ipc_channel;

use std::collections::HashMap;
use std::sync::Arc;
//use std::sync::Mutex;  // not used
use std::sync::atomic::{AtomicBool, Ordering};
//use std::thread;
//use std::time::Duration;

use coordinator::ipc_channel::ipc::IpcSender as Sender;
use coordinator::ipc_channel::ipc::IpcReceiver as Receiver;
use coordinator::ipc_channel::ipc::TryRecvError;
//use coordinator::ipc_channel::ipc::channel;  // not used

//use message;
use message::MessageType;
use message::ProtocolMessage;
//use message::RequestStatus;  // not used
use oplog;

/// CoordinatorState
/// States for 2PC state machine
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CoordinatorState {
    Quiescent,  // in a state or period of inactivity or dormancy.
    ReceivedRequest,
    ProposalSent,
    ReceivedVotesAbort,
    ReceivedVotesCommit,
    SentGlobalDecision
}

/// Coordinator
/// Struct maintaining state for coordinator
#[derive(Debug)]
pub struct Coordinator {
    state: CoordinatorState,
    running: Arc<AtomicBool>,
    log: oplog::OpLog,
    // TODO
    successful_ops: u64,
    failed_ops: u64,
    unknown_ops: u64,
    clients: HashMap<String, (Sender<ProtocolMessage>, Receiver<ProtocolMessage>)>,
    participants: HashMap<String, (Sender<ProtocolMessage>, Receiver<ProtocolMessage>)>,
    total_num_requests: u32,
}

///
/// Coordinator
/// Implementation of coordinator functionality
/// Required:
/// 1. new -- Constructor
/// 2. protocol -- Implementation of coordinator side of protocol
/// 3. report_status -- Report of aggregate commit/abort/unknown stats on exit.
/// 4. participant_join -- What to do when a participant joins
/// 5. client_join -- What to do when a client joins
///
impl Coordinator {

    ///
    /// new()
    /// Initialize a new coordinator
    ///
    /// <params>
    ///     log_path: directory for log files --> create a new log there.
    ///     r: atomic bool --> still running?
    ///
    pub fn new(
        log_path: String,
        r: &Arc<AtomicBool>, 
        total_num_requests: u32) -> Coordinator {

        Coordinator {
            state: CoordinatorState::Quiescent,
            log: oplog::OpLog::new(log_path),
            running: r.clone(),
            // TODO
            successful_ops: 0,
            failed_ops: 0,
            unknown_ops: 0,
            clients: HashMap::new(),
            participants: HashMap::new(),
            total_num_requests,
        }
    }

    ///
    /// participant_join()
    /// Adds a new participant for the coordinator to keep track of
    ///
    /// HINT: Keep track of any channels involved!
    /// HINT: You may need to change the signature of this function
    ///
    pub fn participant_join(&mut self, name: &String, child_tx: Sender<ProtocolMessage>, rx: Receiver<ProtocolMessage>) {
        assert!(self.state == CoordinatorState::Quiescent);

        // TODO
        let key = name.clone();
        self.participants.insert(key, (child_tx, rx));
    }

    ///
    /// client_join()
    /// Adds a new client for the coordinator to keep track of
    ///
    /// HINT: Keep track of any channels involved!
    /// HINT: You may need to change the signature of this function
    ///
    pub fn client_join(&mut self, name: &String, child_tx: Sender<ProtocolMessage>, rx: Receiver<ProtocolMessage>) {
        assert!(self.state == CoordinatorState::Quiescent);

        // TODO
        let key = name.clone();
        self.clients.insert(key, (child_tx, rx));
    }

    ///
    /// report_status()
    /// Report the abort/commit/unknown status (aggregate) of all transaction
    /// requests made by this coordinator before exiting.
    ///
    pub fn report_status(&mut self) {
        // TODO: Collect actual stats
        let successful_ops: u64 = self.successful_ops;
        let failed_ops: u64 = self.failed_ops;
        let unknown_ops: u64 = self.unknown_ops;

        println!("coordinator     :\tCommitted: {:6}\tAborted: {:6}\tUnknown: {:6}", successful_ops, failed_ops, unknown_ops);
    }

    ///
    /// protocol()
    /// Implements the coordinator side of the 2PC protocol
    /// HINT: If the simulation ends early, don't keep handling requests!
    /// HINT: Wait for some kind of exit signal before returning from the protocol!
    ///
    pub fn protocol(&mut self) {
        let mut num_requests = 0;
    
        // 持續運行直到達到最大請求數或 `running` 狀態變為 `false`
        // 將多個客戶端的請求批量收集到一個向量中。
        while self.running.load(Ordering::Relaxed) && num_requests <= self.total_num_requests {
            // 若達到總請求數，則關閉協調器並退出循環
            if num_requests == self.total_num_requests {
                self.shutdown_protocol();
                break;
            }
    
            // 從所有客戶端接收請求並收集到 `client_requests` 向量中
            let client_requests: Vec<_> = self.clients.iter()
                .filter_map(|(_c_key, c_value)| c_value.1.recv().ok())
                .collect();
    
            // 處理每個客戶端請求
            for client_request in client_requests {
                self.handle_client_request(client_request);
                num_requests += 1;
            }
    
            // 將協調器狀態設為靜止狀態
            self.state = CoordinatorState::Quiescent;
        }
    
        // 報告當前狀態
        self.report_status();
    }
    
    fn handle_client_request(&mut self, mut client_request: ProtocolMessage) {
        // Prepare/Ack 階段
        // 設置狀態為已接收請求
        self.state = CoordinatorState::ReceivedRequest;
        
        // 修改請求類型並複製給參與者請求
        client_request.mtype = MessageType::CoordinatorPropose;
        let participant_request = client_request.clone();
        
        // 記錄並將提案發送給所有參與者, 從所有參與者接收回覆（即 "Ack"）
        self.log.append_msg(&client_request);
        self.send_message_participant(participant_request);//self.send_message(participant_request, "participant");
    
        // 收集所有參與者的投票
        self.state = CoordinatorState::ProposalSent;
        let (vote_count, unknown) = self.collect_votes();

        //Commit/Ack 階段
        // 根據投票結果提交或中止交易，決策傳遞是一次性的，不會等待或重試。
        if vote_count == self.participants.len() {
            self.commit_transaction(client_request);
        } else {
            self.abort_transaction(client_request, unknown);
        }
    }
    //實現簡單的超時檢查：當超時達到 timeout 限制（100 毫秒）後，標記 unknown 狀態。若收到參與者的正確回覆則立即計算投票數。
    fn collect_votes(&self) -> (usize, bool) {
        let mut vote_count = 0;
        let mut unknown = false;
    
        // 遍歷所有參與者並接收其投票
        for (_, p_value) in self.participants.iter() {
            let start = std::time::Instant::now();
            let timeout = 100;
    
            // 在超時前嘗試接收投票
            while self.running.load(Ordering::Relaxed) {
                if start.elapsed().as_millis() > timeout as u128 {
                    unknown = true;
                    break;
                }
    
                // 處理接收的投票訊息
                match p_value.1.try_recv() {
                    Ok(p_vote) if p_vote.mtype == MessageType::ParticipantVoteCommit => {
                        vote_count += 1;
                        break;
                    }
                    Ok(_) => continue, // 處理其他類型的 Ok(_) 情況
                    Err(TryRecvError::Empty) => continue, //如果在接收參與者回覆時出現 Err(TryRecvError::Empty)，則直接略過該錯誤，不進行重試。, 如果參與者未能在限定時間內回覆，則該請求會被標記為失敗（中止）
                    Err(_) => {
                        //沒有重試機制，一旦參與者回覆錯誤或者超時後則直接記錄失敗狀態。
                        trace!("coord::Could not receive from participant");
                        break;
                    },
                }
                /*let result = p_value.1.try_recv();
                if let Ok(p_vote) = result {
                    if p_vote.mtype == MessageType::ParticipantVoteCommit {
                        vote_count += 1;
                        break;
                    } else {
                        continue; // 處理其他類型的 Ok(_) 情況
                    }
                } else if let Err(TryRecvError::Empty) = result {
                    continue; // 如果在接收參與者回覆時出現 Err(TryRecvError::Empty)，則直接略過該錯誤
                } else {
                    // 沒有重試機制，一旦參與者回覆錯誤或者超時後則直接記錄失敗狀態
                    trace!("coord::Could not receive from participant");
                    break;
                }*/

            }
        }
    
        (vote_count, unknown)
    }
    
    fn commit_transaction(&mut self, mut client_request: ProtocolMessage) {
        // 在提交或中止決策傳遞給參與者和客戶端時，不會等待確認回覆，而是一次性將決策訊息發送出去，假設所有接收方都能順利接收到訊息。
        // 設置狀態為已接收提交投票
        self.state = CoordinatorState::ReceivedVotesCommit;
        
        // 記錄並通知參與者提交交易
        client_request.mtype = MessageType::CoordinatorCommit;
        self.log.append_msg(&client_request);
        self.send_message_participant(client_request.clone());//self.send_message(client_request.clone(), "participant");
    
        // 通知客戶端提交成功，訊息給所有參與者，但並不等待每個參與者的確認回覆（ack）
        client_request.mtype = MessageType::ClientResultCommit;
        self.send_message_client(client_request);//self.send_message(client_request, "client");
        self.successful_ops += 1;
    }
    
    fn abort_transaction(&mut self, mut client_request: ProtocolMessage, unknown: bool) {
        // 設置狀態為已接收中止投票
        self.state = CoordinatorState::ReceivedVotesAbort;
        
        // 設置中止訊息的類型，若為未知錯誤，添加識別符
        client_request.mtype = MessageType::CoordinatorAbort;
        if unknown {
            client_request.txid = format!("{}_unknown", client_request.txid);
        }
        self.log.append_msg(&client_request);
        self.send_message_participant(client_request.clone());//self.send_message(client_request.clone(), "participant");
    
        // 通知客戶端中止交易
        client_request.mtype = MessageType::ClientResultAbort;
        
        self.send_message_client(client_request);//self.send_message(client_request, "client");
        self.failed_ops += 1;
    }
    
    fn shutdown_protocol(&self) {
        // 創建關閉訊息並發送給所有客戶端和參與者
        let shutdown_msg = ProtocolMessage::instantiate(MessageType::CoordinatorExit, 0, String::from(""), String::from(""), 0);
        self.send_message_client_exit(shutdown_msg.clone());
        self.send_message_participant(shutdown_msg);//self.send_message(shutdown_msg, "participant");
    }
    fn send_message_participant(&self, pm: ProtocolMessage) {
        for (_p_key, p_value) in self.participants.iter() {
            if let Err(error) = p_value.0.send(pm.clone()) {
                error!("coord Interrupt::Could not send to participant");
                //error!("coord::Could not send to participant");
                error!("coord Interrupt::error: {:#?}", error);
                break;
            }
        }
    }

    fn send_message_client(&self, pm: ProtocolMessage) {
        if let Some(c_value) = self.clients.get(&pm.senderid) {
            if let Err(error) = c_value.0.send(pm.clone()) {
                error!("coord Interrupt::Could not send to client");
                //error!("coord::Could not send to client");
                error!("coord Interrupt::error: {:#?}", error);
            }
        }
    }
    /*fn send_message(&self, pm: ProtocolMessage, target: &str) {
        /*match target {
            "participant" => {
                // 將訊息發送給所有參與者
                for (_p_key, p_value) in self.participants.iter() {
                    if let Err(error) = p_value.0.send(pm.clone()) {
                        error!("coord::Could not send to participant");
                        error!("coord::error: {:#?}", error);
                        break;
                    }
                }
            }
            "client" => {
                // 將訊息發送給指定的客戶端
                if let Some(c_value) = self.clients.get(&pm.senderid) {
                    if let Err(error) = c_value.0.send(pm.clone()) {
                        error!("coord::Could not send to client");
                        error!("coord::error: {:#?}", error);
                    }
                }
            }
            _ => {
                error!("coord::Invalid target specified for send_message");
            }
        }*/
        if target == "participant" {
            // 將訊息發送給所有參與者
            for (_p_key, p_value) in self.participants.iter() {
                if let Err(error) = p_value.0.send(pm.clone()) {
                    error!("coord::Could not send to participant");
                    error!("coord::error: {:#?}", error);
                    break;
                }
            }
        } else if target == "client" {
            // 將訊息發送給指定的客戶端
            if let Some(c_value) = self.clients.get(&pm.senderid) {
                if let Err(error) = c_value.0.send(pm.clone()) {
                    error!("coord::Could not send to client");
                    error!("coord::error: {:#?}", error);
                }
            }
        } else {
            error!("coord::Invalid target specified for send_message");
        }
    }*/
    
    fn send_message_client_exit(&self, pm: ProtocolMessage) {
        // 將結束訊息發送給所有客戶端
        for (c_key, _c_value) in self.clients.iter() {
            let mut pm_mod = pm.clone();
            pm_mod.senderid = String::from(c_key);
            self.send_message_client(pm_mod);//self.send_message(pm_mod, "client");
        }
    }    
}