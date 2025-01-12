#[macro_use]
extern crate log;
extern crate stderrlog;
extern crate clap;
extern crate ctrlc;
extern crate ipc_channel;
use std::env;
use std::fs;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::process::{Child,Command};
use ipc_channel::ipc::IpcSender as Sender;
use ipc_channel::ipc::IpcReceiver as Receiver;
use ipc_channel::ipc::IpcOneShotServer;
use ipc_channel::ipc::channel;
pub mod message;
pub mod oplog;
pub mod coordinator;
pub mod participant;
pub mod client;
pub mod checker;
pub mod tpcoptions;
use message::ProtocolMessage;
use participant::Participant;
//use std::time::Duration;  // 新增這行
//use std::thread;

///
/// pub fn spawn_child_and_connect(child_opts: &mut tpcoptions::TPCOptions) -> (std::process::Child, Sender<ProtocolMessage>, Receiver<ProtocolMessage>)
///
///     child_opts: CLI options for child process
///
/// 1. Set up IPC
/// 2. Spawn a child process using the child CLI options
/// 3. Do any required communication to set up the parent / child communication channels
/// 4. Return the child process handle and the communication channels for the parent
///
/// HINT: You can change the signature of the function if necessary
///
fn spawn_child_and_connect(child_opts: &mut tpcoptions::TPCOptions) -> (Child, Sender<ProtocolMessage>, Receiver<ProtocolMessage>) {
    // 步驟1: 設置一次性 IPC 伺服器來建立通訊管道
    let (ipc_one_shot_server, server_name) = IpcOneShotServer::new().expect("Failed to create IPC OneShotServer");

    // 步驟2: 更新子程序選項，將 IPC 路徑包含進去，以便子程序連接到伺服器
    child_opts.ipc_path = server_name;

    // 步驟3: 使用修改後的選項來啟動子程序
    let child = Command::new(env::current_exe().unwrap())
        .args(child_opts.as_vec())
        .spawn()
        .expect("Failed to execute child process");

    // 步驟4: 建立本地通訊管道 (tx, rx) 以進行父子之間的消息交換
    /*let (tx, rx) = channel().unwrap();
    // TODO
    // 步驟5: 接收子程序的連接，並獲取子程序的 tx 和伺服器名稱以便雙向通訊
    let (_, (child_tx, child_server_name)): (_, (Sender<ProtocolMessage>, String)) =
        ipc_one_shot_server.accept().expect("Failed to accept connection from child process");

    // 步驟6: 連接到子程序的伺服器名稱，並將協調器的 tx 發送給子程序
    let coordinator_to_child_tx = Sender::<Sender<ProtocolMessage>>::connect(child_server_name)
        .expect("Failed to connect to child's server name");
    coordinator_to_child_tx.send(tx).expect("Failed to send coordinator's tx to child");

    // 步驟7: 返回子程序的控制代碼以及已建立的通訊管道
    (child, child_tx, rx)*/
    debug!("{}::client::connect_to_coordinator(): {}", child_opts.num, child_opts.ipc_path);


    // 接受子程序的連接以建立通訊通道
    let (_, (tx_from_child, rx_to_child)) = ipc_one_shot_server.accept().expect("Failed to accept connection from child process");
    //let (_, (tx_from_child, rx_to_child)) = ipc_one_shot_server.accept().unwrap();

    // 返回子程序和通訊通道
    (child, tx_from_child, rx_to_child)
}

///
/// pub fn connect_to_coordinator(opts: &tpcoptions::TPCOptions) -> (Sender<ProtocolMessage>, Receiver<ProtocolMessage>)
///
///     opts: CLI options for this process
///
/// 1. Connect to the parent via IPC
/// 2. Do any required communication to set up the parent / child communication channels
/// 3. Return the communication channels for the child
///
/// HINT: You can change the signature of the function if necessasry
///
fn connect_to_coordinator(opts: &tpcoptions::TPCOptions) -> (Sender<ProtocolMessage>, Receiver<ProtocolMessage>) {
    // 步驟1: 創建本地通訊管道 (tx, rx) 用於通訊
    /*let (tx, rx) = channel().unwrap();

    // 步驟2: 設置一次性伺服器，以便協調器發送其發送者通道給子程序
    let (ipc_one_shot_server, server_name_for_coordinator) = IpcOneShotServer::new().expect("Failed to create child server");

    // 步驟3: 連接到協調器並發送子程序的 tx 和伺服器名稱
    let coordinator_sender = Sender::<(Sender<ProtocolMessage>, String)>::connect(opts.ipc_path.clone())
        .expect("Failed to connect to coordinator IPC path");
    coordinator_sender.send((tx, server_name_for_coordinator)).expect("Failed to send child tx and server name to coordinator");

    // 步驟4: 接收來自協調器的連接以獲取協調器的 tx
    let (_, coordinator_rx): (_, Sender<ProtocolMessage>) = ipc_one_shot_server.accept().expect("Failed to accept coordinator connection");

    // 返回協調器的 tx 和子程序的 rx 以便通訊
    (coordinator_rx, rx)*/

    
    // TODO
    // 連接到協調者
    let coordinator_sender = Sender::connect(opts.ipc_path.clone()).unwrap();
    // 創建通訊管道
    let (tx_from_coord, rx_from_coord) = channel::<ProtocolMessage>().unwrap();
    let (tx_to_coord, rx_to_coord) = channel::<ProtocolMessage>().unwrap();
    // 傳送子端的 tx 給協調者
    coordinator_sender.send((tx_from_coord, rx_to_coord)).unwrap();

    (tx_to_coord, rx_from_coord)
}

pub fn run_check(mut opts: tpcoptions::TPCOptions) {
    // 將模式設定為 "check"，以便啟動檢查程序
    opts.mode = "check".to_string();

    // 創建並執行子程序以運行 checker
    let mut child = Command::new(env::current_exe().unwrap())
        .args(opts.as_vec())
        .spawn()
        .expect("Failed to execute child process");

    // 等待檢查程序完成
    child.wait().expect("Error in waiting for check process");
}

///
/// pub fn run(opts: &tpcoptions:TPCOptions, running: Arc<AtomicBool>)
///     opts: An options structure containing the CLI arguments
///     running: An atomically reference counted (ARC) AtomicBool(ean) that is
///         set to be false whenever Ctrl+C is pressed
///
/// 1. Creates a new coordinator
/// 2. Spawns and connects to new clients processes and then registers them with
///    the coordinator
/// 3. Spawns and connects to new participant processes and then registers them
///    with the coordinator
/// 4. Starts the coordinator protocol
/// 5. Wait until the children finish execution
///
fn run(opts: & tpcoptions::TPCOptions, running: Arc<AtomicBool>) {
    let coord_log_path = format!("{}//{}", opts.log_path, "coordinator.log");

    // TODO
    // 創建協調者並初始化日誌
    
    // 記錄程式開始執行的時間
    debug!("coord::Execute run()");
    trace!("coord::run(): coord_log_path: {}", coord_log_path);
    let now = std::time::Instant::now();

    // 計算總請求數量
    let total_num_requests = opts.num_clients * opts.num_requests;

    // 初始化協調器，並傳入日誌路徑、是否正在執行的參數及總請求數
    let mut coord = coordinator::Coordinator::new(coord_log_path, &running, total_num_requests);

    // 用來存放所有子程序的處理程序集合
    let mut children_handler: Vec<(String, Child)> = Vec::new();

    // 啟動所有客戶端子程序並連接
    for i in 0..opts.num_clients {
        // 建立針對客戶端的配置
        let mut temp_opts = opts.clone();
        temp_opts.mode = "client".to_string();  // 設定模式為 "client"
        temp_opts.num = i;  // 設定客戶端編號
        let process_name = format!("client_{}", i);  // 設定客戶端名稱

        // 啟動子程序並連接協調器
        let (child, child_tx, rx) = spawn_child_and_connect(&mut temp_opts.clone());

        // 將客戶端的傳送和接收通道註冊到協調器中
        //let key = format!("client_{}", i);
        coord.client_join(&process_name, child_tx, rx);

        // 將子程序的名稱和處理程序儲存到 `children_handler` 向量中
        children_handler.push((process_name, child));
    }

    // 啟動所有參與者子程序並連接
    for i in 0..opts.num_participants {
        // 建立針對參與者的配置
        let mut temp_opts = opts.clone();
        temp_opts.mode = "participant".to_string();  // 設定模式為 "participant"
        temp_opts.num = i;  // 設定參與者編號
        let process_name = format!("participant_{}", i);  // 設定參與者名稱

        // 啟動子程序並連接協調器
        let (child, participant_tx, rx) = spawn_child_and_connect(&mut temp_opts.clone());

        // 將參與者的傳送和接收通道註冊到協調器中
        //let key = format!("participant_{}", i);
        coord.participant_join(&process_name, participant_tx, rx);

        // 將子程序的名稱和處理程序儲存到 `children_handler` 向量中
        children_handler.push((process_name, child));
    }
    // 啟動協調器的二階段提交協議處理
    coord.protocol();

    // 檢查是否有中止信號，若有則終止所有子程序
    if !running.load(Ordering::Relaxed) {
        for child_handler in children_handler.iter_mut() {
            match child_handler.1.kill() {
                Ok(()) => info!("{}::Child process killed", child_handler.0),
                Err(error) => warn!("{}::Child.kill() produced an error: {}", child_handler.0, error),
            }
        }
    }

    // 等待所有子程序完成其執行
    info!("Wait for children to finish their process");
    for child_handler in children_handler.iter_mut() {
        match child_handler.1.wait() {
            Ok(r) => info!("{}::{}", child_handler.0, r),
            Err(error) => warn!("{}::Child.wait produced an error: {}", child_handler.0, error),
        }
    }

    // 計算並顯示程式執行的總時間
    let elapsed_time = now.elapsed().as_millis();
    info!("Total elapsed time of the coordinator and program: {}", elapsed_time);
    // 執行檢查程序以驗證協議執行結果
    //run_check(opts.clone());
    
}

///
/// pub fn run_client(opts: &tpcoptions:TPCOptions, running: Arc<AtomicBool>)
///     opts: An options structure containing the CLI arguments
///     running: An atomically reference counted (ARC) AtomicBool(ean) that is
///         set to be false whenever Ctrl+C is pressed
///
/// 1. Connects to the coordinator to get tx/rx
/// 2. Constructs a new client
/// 3. Starts the client protocol
///
fn run_client(opts: & tpcoptions::TPCOptions, running: Arc<AtomicBool>) {
    // TODO
    debug!("{}::client::run_client()", opts.num);

    //debug!("{}::client::run_client(): Connect to Coordinator to get tx/rx", opts.num);
    let (coordinator_tx, rx) = connect_to_coordinator(&opts);

    // Constructs a new client
    let mut client = client::Client::new( format!("{}", opts.num), running, coordinator_tx, rx);

    // Starts the client protocol
    client.protocol(opts.num_requests);
}

///
/// pub fn run_participant(opts: &tpcoptions:TPCOptions, running: Arc<AtomicBool>)
///     opts: An options structure containing the CLI arguments
///     running: An atomically reference counted (ARC) AtomicBool(ean) that is
///         set to be false whenever Ctrl+C is pressed
///
/// 1. Connects to the coordinator to get tx/rx
/// 2. Constructs a new participant
/// 3. Starts the participant protocol
///
fn run_participant(opts: & tpcoptions::TPCOptions, running: Arc<AtomicBool>) {
    debug!("{}::participant_::run_participant()", opts.num);
    let participant_id_str = format!("participant_{}", opts.num);
    let participant_log_path = format!("{}//{}.log", opts.log_path, participant_id_str);

    // TODO
    // Step 1: Connect to the coordinator to get tx/rx channels
    let (coordinator_tx, rx) = connect_to_coordinator(opts);
    //Step 2: Construct a new participant
    let mut participant = Participant::new(
        participant_id_str.clone(),
        participant_log_path,
        running,
        opts.send_success_probability,
        opts.operation_success_probability,
        coordinator_tx,
        rx,
        opts.num_requests * opts.num_clients,
    );
    // Step 3: Start the participant protocol
    participant.protocol();
}

fn main() {
    // Parse CLI arguments
    let opts = tpcoptions::TPCOptions::new();
    // Set-up logging and create OpLog path if necessary
    stderrlog::new()
            .module(module_path!())
            .quiet(false)
            .timestamp(stderrlog::Timestamp::Millisecond)
            .verbosity(opts.verbosity)
            .init()
            .unwrap();
    match fs::create_dir_all(opts.log_path.clone()) {
        Err(e) => error!("Failed to create log_path: \"{:?}\". Error \"{:?}\"", opts.log_path, e),
        _ => (),
    }

    // Set-up Ctrl-C / SIGINT handler
    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();
    let m = opts.mode.clone();
    ctrlc::set_handler(move || {
        r.store(false, Ordering::SeqCst);
        if m == "run" {
            print!("\n");
        }
    }).expect("Error setting signal handler!");

    // Execute main logic
    match opts.mode.as_ref() {
        "run" => run(&opts, running),
        "client" => run_client(&opts, running),
        "participant" => run_participant(&opts, running),
        "check" => checker::check_last_run(opts.num_clients, opts.num_requests, opts.num_participants, &opts.log_path),
        _ => panic!("Unknown mode"),
    }
}
