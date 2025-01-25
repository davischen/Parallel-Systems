# Two-Phase Commit (2PC) Implementation

This project implements the Two-Phase Commit (2PC) protocol using Rust. The 2PC protocol ensures distributed consistency across multiple participants in a distributed transaction system. It consists of components such as a coordinator, participants, and clients.

## Features

- **Coordinator**: Manages the distributed transaction, tracks participants and clients, and ensures transaction consistency.
- **Participant**: Executes operations on behalf of the transaction and votes to commit or abort based on success probability.
- **Client**: Initiates requests and awaits transaction results.
- **Fault Tolerance**: Includes timeout mechanisms for vote collection.
- **Logging**: Tracks transaction operations and states using an operation log.

---

## Components

### Coordinator

The coordinator:
- Handles incoming transaction requests from clients.
- Sends proposals to participants.
- Collects votes from participants to decide whether to commit or abort the transaction.
- Sends global decisions back to participants and clients.

#### Coordinator States
- **Quiescent**: Idle state, waiting for requests.
- **ReceivedRequest**: Received a new client request.
- **ProposalSent**: Proposal sent to participants.
- **ReceivedVotesCommit**: All participants voted to commit.
- **ReceivedVotesAbort**: At least one participant voted to abort.
- **SentGlobalDecision**: Global decision communicated.

### Participant

Each participant:
- Performs operations as per the transaction proposal.
- Votes commit or abort based on operation success probability.
- Awaits and adheres to the coordinator's final decision.

#### Participant States
- **Quiescent**: Idle state.
- **ReceivedP1**: Received phase 1 proposal from the coordinator.
- **VotedAbort**: Voted to abort.
- **VotedCommit**: Voted to commit.
- **AwaitingGlobalDecision**: Awaiting the coordinator's final decision.

### Client

Each client:
- Initiates transaction requests.
- Receives transaction results from the coordinator.
- Maintains statistics on committed, aborted, and unknown transactions.

---

## Command-Line Arguments

| Argument | Description |
|----------|-------------|
| `--mode` | Sets the mode: `run`, `client`, `participant`, or `check`. |
| `--log-path` | Directory for log files. |
| `--num-clients` | Number of client processes. |
| `--num-participants` | Number of participant processes. |
| `--num-requests` | Number of transaction requests per client. |
| `--send-success-prob` | Probability of message delivery success. |
| `--operation-success-prob` | Probability of transaction operation success. |
| `--verbosity` | Logging verbosity level. |

---

## Manual Compilation

To compile the program, use:

```bash
cargo build --release
```

---

## Usage Examples

### Run Coordinator
```bash
./two_phase_commit --mode run --log-path logs/ --num-clients 3 --num-participants 2 --num-requests 5
```

### Run Client
```bash
./two_phase_commit --mode client --log-path logs/ --num 0
```

### Run Participant
```bash
./two_phase_commit --mode participant --log-path logs/ --num 0
```

### Run Checker
```bash
./two_phase_commit --mode check --log-path logs/
```

---

## Key Functions

### Coordinator Functions
- `new`: Initializes a new coordinator.
- `participant_join`: Registers a participant.
- `client_join`: Registers a client.
- `protocol`: Executes the 2PC protocol.
- `report_status`: Reports transaction statistics.

### Participant Functions
- `new`: Initializes a new participant.
- `perform_operation`: Executes the operation and votes commit/abort.
- `protocol`: Implements the participant's 2PC protocol.
- `report_status`: Reports transaction statistics.

### Client Functions
- `new`: Initializes a new client.
- `send_next_operation`: Sends the next operation request.
- `recv_result`: Receives the result of the last request.
- `protocol`: Implements the client's 2PC protocol.
- `report_status`: Reports transaction statistics.

---

## Logging and Debugging

Logs are stored in the directory specified by `--log-path`. Verbosity can be controlled using the `--verbosity` flag. Use `stderrlog` for detailed tracing and debugging.

---

## Fault Tolerance

The implementation includes:
- Timeout-based vote collection.
- Handling of unknown or failed participants.
- Graceful shutdown on receiving a signal.

