# Lab Judge System (LJS)
A high-performance judge system built for programming labs and coding examinations.
LJS provides a secure and structured environment for compiling, running, and evaluating student submissions with minimal overhead, using IOI's isolate sandbox for process isolation and resource control.

## Features
* __Run Mode__ - Test solutions against public example test cases.
* __Custom Run__ - Execute solutions on custom user-provided input.
* __Submit Mode__ - Evaluate solutions against hidden test cases.
* Secure __sandboxed execution__ using IOI's isolate sandbox.
* __Configurable Constraints__ (in progress):
    * Time Limits
    * Memory Limits
    * Directory Paths
    * Language/Compiler Settings.
* __Concurrent Submission Handling__ (in progress).
* __Database Integration__ (in progress).
* __Structured File Permission Model__ for secure test case isolation.
* __Minimal Runtime Overhead__ focused on systems-level performance.

## Directory Structure for the Lab
The following diagram shows the expected filesystem layout for the professor-side lab configuration, including directory hierarchy and file permission organization.

![LJS Professor Filesystem Structure](LJS_prof_file_structure.png)

## Filesystem Layout Overview
- ```Problem/``` Directory
    - Contains all problems for a lab.
    - ```prob_x/``` Directory
        - individual problem directory which contains:
            - Problem Statement
            - Example Test Cases
- ```Hidden_data``` Directory
    - Contains data which should only be accessible to authorized users and the judge process.
    - ```Hidden Test Case``` Directory
        - Contains hidden test cases
    - ```Solution``` Directory
        - Contains the optimal solution

## Usage
### Custom run

Run the solution on custom input.
```bash
./LJS custom_run <solution file.cpp>
```
### Run

Run the solution against sample test cases.
```bash
./LJS run <lab number> <problem number> <solution file.cpp>
```
### Submit

Submit the solution for evaluation against all test cases.
```bash
LJS submit <lab number> <problem number> <solution file.cpp>
```

## Planned Improvements
- Config-driven environment setup
- Persistent submissions database
- Detailed verdict logs and analytics

## Known Issues
* Race conditions may occur during multiple simultaneous submissions. 
* Some execution paths currently lack proper timeout safeguards.
* Certain environment configurations are still hardcoded.

## Tech Stack
- __Language:__ C++
- __Compiler:__ ```g++```
- __Platform:__ Linux
- __System APIs:__ POSIX System Calls
- __Process Management:__ ```fork()```, ```execv()```, ```wait()```
- __Sandboxing:__ [IOI isolate](https://github.com/ioi/isolate)
- __File Handling:__ Linux Permissions & Directory Management

## Motivation
Programming labs often rely on manual evaluation or heavyweight online judge systems.
LJS aims to provide a lightweight, fast, and locally deployable alternative specifically optimized for academic lab environments.

## Future Vision
The long-term goal of LJS is to evolve into a fully scalable programming lab platform with features like:
- Live student Leaderboard
- Live monitoring and analytics
- Plagiarism Detector
- Helpful towards the learning journey of the students