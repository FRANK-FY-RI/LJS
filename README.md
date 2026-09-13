# Lab Judge System (LJS)
A high-performance, lightweight, concurrent judge system built for programming labs and coding examinations written in C++17. 

LJS provides a secure, structured, and multithreaded environment for compiling, running, and evaluating student submissions with minimal overhead. LJS follows a persistent client-server architecture. Clients communicate with the judge server through UNIX domain sockets, while submissions are processed concurrently using a custom thread pool.

The server compiles submitted programs with controlled resource limits and executes them inside an IOI isolate sandbox. The judging pipeline supports both public-test execution and hidden-test submission.

## Features
* **Client-Server Architecture** - A persistent background server daemon (`LJS_server`) handles connections from lightweight CLI clients (`LJS_client`).
* **Concurrent Submission Handling** - Fully thread-safe judging using a custom-built thread pool and thread-safe queue.
* **Run Mode** - Test solutions against public example test cases.
* **Submit Mode** - Evaluate solutions against hidden test cases.
* **Resource Limits Enforced Compilation** - Limits CPU, Memory, wall-time and file size to prevent malicious code from crashing the server.
* **Secure Sandboxed Execution** - Uses IOI's `isolate` sandbox to strictly limit memory, execution time, and system access.
* **Config Driven environment setup** - Professors and Administrators can configure various environment variables and resource limits.
* **Thread-safe isolate sandbox ID allocation**
* **Source-path and file-ownership validation** - Ensures that users can only submit their own source codes.
* **Automatic cleanup of temporary files and sandboxes**
* **Strongly typed verdict handling**

(Note: The `custom_run` feature has been deprecated in favor of streamlined standard run/submit pipelines).*

## Architecture and Directory Structure

### Judging Pipeling
![LJS Professor Filesystem Structure](assets/LJS_architecture_overview.png)

### Lab Filesystem Layout (Professor-side)
The following diagram shows the expected filesystem layout for configuring labs, including directory hierarchy and file permission organization.

![LJS Professor Filesystem Structure](assets/LJS_prof_file_structure.png)

### Filesystem Layout Overview
- ```Problem/``` Directory
    - Contains all problems for a lab.
    - ```prob_x/``` Directory
        - individual problem directory which contains:
            - Problem Statement
            - Example Test Cases (`.in` and `.ans` files)
- ```Hidden_data``` Directory
    - Contains data which should only be accessible to authorized users and the judge process.
    - ```Hidden Test Case``` Directory
        - Contains hidden test cases 

## Security and Stress Testing

LJS includes an automated test suite for evaluating process-management robustness, resource-limit enforcement, and behavior under concurrent workloads. The suite exercises adversarial execution scenarios and high-load conditions to help identify reliability and isolation issues.

The test suite was initially generated with AI assistance and subsequently integrated and validated as part of the project.

## Build
### Requirements
- Linux
- `g++` with C++17 support
- [IOI isolate](https://github.com/ioi/isolate)

Build the project using:
```bash
make
```

Delete/clean the  build artifacts using:
```base
make clean
```

## Usage
Because LJS now uses a client-server model, you must have the server running before submitting code.

### 1. Start the Judge Server
Run the server daemon. It will listen for incoming UNIX socket connections.

```bash
./LJS_server
```
### 2. Run a Solution (Client)
In a separate terminal, test a solution against the public example test cases.

```bash
./LJS_client run <lab number> <problem number> <solution file.cpp>
# Example: ./LJS_client run 1 1 prob_1.cpp
```
### 3. Submit a Solution (Client)
Submit the solution for formal evaluation against all test cases.

```bash
LJS submit <lab number> <problem number> <solution file.cpp>
```

## Tech Stack

- Language: C++17

- Compiler: g++

- Concurrency: [Thread-Pool](https://github.com/FRANK-FY-RI/Thread-Pool) and [MPMC Concurrent Job Queue](https://github.com/FRANK-FY-RI/Concurrent-queues)

- Networking/IPC: POSIX UNIX Domain Sockets (AF_UNIX)

- Process Management: `fork()`, `execv()`, `waitpid()`, `dup2()`  

- Sandboxing: [IOI isolate](https://github.com/ioi/isolate)

- Platform: Linux

## Planned Improvements
- **Database Integration:** Persistent submissions database to replace live-socket-only feedback.
- **Detailed verdict logs:** Better analytics and output tracing.


## Motivation
Programming labs often rely on manual evaluation or heavyweight, bloated online judge systems. LJS aims to provide a lightweight, blazingly fast, and locally deployable alternative specifically optimized for the concurrency and security needs of academic lab environments.

## Future Vision
The long-term goal of LJS is to evolve into a fully scalable programming lab platform with features like:
- Live student Leaderboard
- Live monitoring and analytics
- Plagiarism Detector
- Integrated feedback mechanisms to aid the learning journey of students.
