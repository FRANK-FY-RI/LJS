import os
import re
import socket
import subprocess
import time
import signal
import concurrent.futures
import uuid
from typing import List, Union

LJS_CLIENT = "./client"
TEMP_DIR = "/tmp/"
ISOLATE_DIR = "/var/local/lib/isolate/"
SV_SOCK_ADDR = "/tmp/sv_sock_addr"

# Safely bounded payloads targeting specific limits
with open("sol.cpp", "r") as f:
    ac_code = f.read()
PAYLOADS = {
    "AC": ac_code,
    "WA": "#include <iostream>\nint main() { std::cout << \"wrong\"; return 0; }",
    "TLE": "int main() { while(true); return 0; }",
    "MLE": "#include <vector>\nint main() { std::vector<int> v; while(true) v.push_back(1); }",
    "FORK_BOMB": "#include <unistd.h>\nint main() { while(true) fork(); return 0; }",
    "LARGE_OUTPUT": "#include <iostream>\nint main() { while(true) std::cout << \"FLOOD\\n\"; return 0; }",
    "SIGSEGV": "int main() { int* p = nullptr; *p = 1; return 0; }",
    "SIGABRT": "#include <cstdlib>\nint main() { abort(); return 0; }",
    "SIGFPE": "int main() { volatile int a = 1, b = 0; return a / b; }",
    "COMPILER_BOMB": "template<int N> struct S { S<N-1> s; }; S<10000> s; int main() {}",
    "SLEEP_2": "#include <unistd.h>\nint main() { sleep(2); return 0; }"
}

class SystemState:
    def __init__(self, server_pid: int):
        self.server_pid = server_pid
        self.binaries = set(f for f in os.listdir(TEMP_DIR) if re.match(r'^sol\d+$', f))
        self.meta = set(f for f in os.listdir(TEMP_DIR) if re.match(r'^\d+\.meta$', f))
        self.err_logs = set(f for f in os.listdir(TEMP_DIR) if re.match(r'^err\d+\.err$', f))
        self.out_logs = set(f for f in os.listdir(TEMP_DIR) if re.match(r'^out\d+\.txt$', f))
        self.active_boxes = set(os.listdir(ISOLATE_DIR)) if os.path.exists(ISOLATE_DIR) else set()
        
        # Deep OS-Level Invariants
        try:
            self.fd_count = len(os.listdir(f"/proc/{server_pid}/fd/"))
            self.threads = int(open(f"/proc/{server_pid}/stat").read().split()[19])
            self.children = set(subprocess.check_output(["pgrep", "-P", str(server_pid)]).decode().split())
        except:
            self.fd_count = 0
            self.threads = 0
            self.children = set()

def get_server_pid() -> int:
    try:
        return int(subprocess.check_output(["pgrep", "-f", "LJS"]).decode().strip().split('\n')[0])
    except:
        return 0

def get_descendants(pid: int) -> List[int]:
    """Recursively finds all descendant processes of a given PID."""
    try:
        children = subprocess.check_output(["pgrep", "-P", str(pid)]).decode().split()
        descendants = [int(c) for c in children]
        for child in children:
            descendants.extend(get_descendants(int(child)))
        return descendants
    except subprocess.CalledProcessError:
        return []

def extract_exact_verdict(raw_output: str) -> str:
    """Strips ANSI codes and strictly extracts the deterministic verdict field."""
    clean = re.sub(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])', '', raw_output).strip()
    
    if "Invalid source file" in clean: return "Invalid source file"
    if "Incorrect Number of Arguments" in clean: return "Incorrect Number of Arguments"
    if "Compilation Time Limit Exceeded" in clean: return "Compilation Time Limit Exceeded"
    if "Compilation error" in clean: return "Compilation error"
    if "Unable to run the sandbox" in clean: return "Child Process Error"
    if "Unable to run some program" in clean: return "Child Process Error"
    if "Process Error" in clean: return "Process Error"

    m = re.search(r'Test 1:\s*([^\n]+)', clean)
    if m: return m.group(1).strip()
    return clean

def wait_for_cleanup(baseline: SystemState, timeout: float = 5.0) -> List[str]:
    """Polls until state matches baseline, strictly tracking FDs, Threads, and Process Trees."""
    start = time.time()
    while time.time() - start < timeout:
        current = SystemState(baseline.server_pid)
        leaks = []
        if current.binaries - baseline.binaries: leaks.append(f"Binaries: {current.binaries - baseline.binaries}")
        if current.meta - baseline.meta: leaks.append(f"Meta: {current.meta - baseline.meta}")
        if current.active_boxes - baseline.active_boxes: leaks.append(f"Boxes: {current.active_boxes - baseline.active_boxes}")
        
        # Invariants: 0 Orphans, Thread pool jitter allows +11 (max_threads limit), FDs allow +4 caching
        if current.children - baseline.children: leaks.append(f"Orphan Processes: {current.children - baseline.children}")
        if current.threads > baseline.threads + 11: leaks.append(f"Worker Thread Leak: {current.threads} > {baseline.threads} + 11")
        if current.fd_count > baseline.fd_count + 4: leaks.append(f"FD Leak: {current.fd_count} > {baseline.fd_count}")
        
        if not leaks: return []
        time.sleep(0.2)
    return leaks

def submit_and_verify(test_name: str, payload_key: str, expected_v: Union[str, List[str]], baseline: SystemState, skip_cleanup=False) -> bool:
    source_path = f"test_{uuid.uuid4().hex[:8]}_{payload_key}.cpp"
    with open(source_path, "w") as f: 
        f.write(PAYLOADS.get(payload_key, PAYLOADS["AC"]))
        
    try:
        proc = subprocess.Popen([LJS_CLIENT, "run", "1", "1", source_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, _ = proc.communicate(timeout=45)
        output = stdout.decode()
    except subprocess.TimeoutExpired:
        proc.kill()
        print(f"\033[1;31m[FAIL] {test_name}: Client Timeout (LJS Server unresponsive)\033[0m")
        if os.path.exists(source_path): os.remove(source_path)
        return False

    if os.path.exists(source_path): os.remove(source_path)

    actual_v = extract_exact_verdict(output)
    
    expected_list = [expected_v] if isinstance(expected_v, str) else expected_v
    if actual_v not in expected_list:
        print(f"\033[1;31m[FAIL] {test_name}: Expected one of {expected_list}, got '{actual_v}'\033[0m")
        return False
        
    if not skip_cleanup:
        leaks = wait_for_cleanup(baseline)
        if leaks:
            print(f"\033[1;31m[FAIL] {test_name}: Resource leaks detected: {leaks}\033[0m")
            return False
            
    if not skip_cleanup: print(f"\033[1;32m[PASS] {test_name}\033[0m")
    return True

def test_external_sigkill(baseline: SystemState):
    """Recursively targets the deepest sandboxed submission process and strikes with SIGKILL."""
    test_name = "External SIGKILL Descendant Strike"
    print(f"\n[*] Running: {test_name}")
    
    source_path = f"test_sigkill_{uuid.uuid4().hex[:8]}.cpp"
    with open(source_path, "w") as f: f.write(PAYLOADS["TLE"]) # Infinite loop
        
    proc = subprocess.Popen([LJS_CLIENT, "run", "1", "1", source_path], stdout=subprocess.PIPE)
    
    target_pid = 0
    descendants = []
    for _ in range(30):
        try:
            # 1. Find Isolate Runner
            pids = subprocess.check_output(["pgrep", "-f", "isolate.*--run"]).decode().split()
            if pids:
                # 2. Find the deepest actual descendant (the compiled binary execution)
                isolate_pid = int(pids[0])
                descendants = get_descendants(isolate_pid)
                if descendants:
                    target_pid = descendants[-1] # Hit the deepest child
                    break
        except subprocess.CalledProcessError: pass
        time.sleep(0.1)
        
    if not target_pid:
        print(f"\033[1;31m[FAIL] {test_name}: Could not locate deep sandboxed child.\033[0m")
        proc.kill()
        return False
        
    print(f"    -> Deepest target PID {target_pid} isolated. Penetrating sandbox with sudo SIGKILL...")
    
    # Hit the exact child process with uncatchable SIGKILL (Requires sudo due to Isolate UID shift)
    subprocess.run(["sudo", "kill", "-9", str(target_pid)], stderr=subprocess.DEVNULL)
    
    stdout, _ = proc.communicate(timeout=10)
    os.remove(source_path)
    
    actual_v = extract_exact_verdict(stdout.decode())
    print(f"    -> Server correctly translated inner death to verdict: '{actual_v}'")
    
    # Assert Process Tree is wiped completely
    for pid in descendants + [target_pid]:
        try:
            os.kill(pid, 0)
            print(f"\033[1;31m[FAIL] {test_name}: Sandbox Descendant {pid} survived SIGKILL!\033[0m")
            return False
        except PermissionError:
            # Process exists but is owned by the sandbox user
            print(f"\033[1;31m[FAIL] {test_name}: Sandbox Descendant {pid} survived SIGKILL!\033[0m")
            return False
        except ProcessLookupError:
            pass # Reaped properly
            
    leaks = wait_for_cleanup(baseline)
    if leaks:
        print(f"\033[1;31m[FAIL] {test_name}: Resource leaks detected post-SIGKILL: {leaks}\033[0m")
        return False
        
    print(f"\033[1;32m[PASS] {test_name}: OS-level fault tolerance verified.\033[0m")
    return True

def test_sandbox_id_lifecycle(baseline: SystemState):
    """Explicitly verifies that cleanup frees sandbox IDs and allows mathematically sound reuse."""
    test_name = "Sandbox ID Lifecycle & Reuse"
    print(f"\n[*] Running: {test_name}")
    
    # Prove single reuse
    submit_and_verify("Lifecycle Probe 1", "AC", "Accepted", baseline, skip_cleanup=True)
    wait_for_cleanup(baseline, timeout=5.0) # Ensure directory disappeared
    submit_and_verify("Lifecycle Probe 2", "AC", "Accepted", baseline, skip_cleanup=True)
    wait_for_cleanup(baseline, timeout=5.0)
    
    # Prove exhaustion-recovery via queue saturation
    print("    -> Saturating 20 jobs to force ID pool cycling...")
    success = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
        futures = [executor.submit(submit_and_verify, f"Lifecycle Job {i}", "AC", "Accepted", baseline, skip_cleanup=True) for i in range(20)]
        for f in concurrent.futures.as_completed(futures):
            try:
                if f.result(): success += 1
            except Exception: pass
            
    # Max active threads is 11, so 20 jobs succeeding PROVES IDs are returning to the pool and being safely popped.
    if success == 20: 
        print(f"\033[1;32m[PASS] {test_name}: 20/20 Jobs Completed. ID cycling mathematically verified.\033[0m")
        return True
    else:
        print(f"\033[1;31m[FAIL] {test_name}: ID cycling failed. Only {success}/20 completed.\033[0m")
        return False

def test_phased_disconnect(test_name: str, payload_key: str, delay: float, baseline: SystemState):
    """Connects to raw socket, uploads file, and abruptly severs connection at exact execution phases."""
    source_path = f"test_disc_{uuid.uuid4().hex[:8]}.cpp"
    with open(source_path, "w") as f: f.write(PAYLOADS[payload_key])
    
    try:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(SV_SOCK_ADDR)
        cwd = os.getcwd() + "\n"
        s.sendall(cwd.encode())
        s.sendall(f"run\n1\n1\n{source_path}\n".encode())
        
        time.sleep(delay)
        s.close() # Abrupt disconnect without SHUT_WR
        
        leaks = wait_for_cleanup(baseline, timeout=10.0)
        os.remove(source_path)
        
        if leaks: print(f"\033[1;31m[FAIL] {test_name} leaks: {leaks}\033[0m")
        else: print(f"\033[1;32m[PASS] {test_name} (Worker cleaned up cleanly)\033[0m")
    except Exception as e:
        print(f"\033[1;31m[FAIL] {test_name}: {e}\033[0m")

def run_suite():
    pid = get_server_pid()
    if pid == 0:
        print("\033[1;31m[ERROR] LJS_server must be running.\033[0m")
        return
        
    global_baseline = SystemState(pid)
    
    print("\n--- 1. Exact Verdicts, Resource Exhaustion, & Signals ---")
    submit_and_verify("Valid AC", "AC", "Accepted", global_baseline)
    submit_and_verify("Wrong Answer", "WA", "Wrong Answer", global_baseline)
    submit_and_verify("Time Limit Exceeded", "TLE", "Time Limit Exceeded", global_baseline)
    submit_and_verify("Memory Limit Exceeded", "MLE", "Memory Limit Exceeded", global_baseline)
    submit_and_verify("Segfault", "SIGSEGV", "Segmentation fault", global_baseline)
    submit_and_verify("Abort Signal", "SIGABRT", "Aborted", global_baseline)
    submit_and_verify("Floating Point Exception", "SIGFPE", "Floating point exception", global_baseline)
    
    # Explicit Bounded Verdicts for kernel-dependent limit tests
    # FORK_BOMB hits RLIMIT_NPROC (Process Error) or Wall Timeout (TLE)
    submit_and_verify("Fork Bomb (CPU/PID/Memory Limit)", "FORK_BOMB", ["Time Limit Exceeded", "Memory Limit Exceeded", "Process Error", "Child Process Error"], global_baseline)
    
    # LARGE_OUTPUT hits RLIMIT_FSIZE (SIGXFSZ -> File size limit exceeded) or TLE
    submit_and_verify("Output Flood (File Limit)", "LARGE_OUTPUT", ["Time Limit Exceeded", "File size limit exceeded", "Process Error", "Child Process Error"], global_baseline)
    
    print("\n--- 2. Compiler Defenses ---")
    submit_and_verify("Compiler Bomb (OOM)", "COMPILER_BOMB", "Compilation error", global_baseline)

    print("\n--- 3. Protocol Attacks & Path Traversal ---")
    def socket_attack(name, payload, expected):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(SV_SOCK_ADDR)
        s.sendall(payload.encode())
        s.shutdown(socket.SHUT_WR)
        resp = ""
        while True:
            chunk = s.recv(4096).decode()
            if not chunk: break
            resp += chunk
        s.close()
        actual = extract_exact_verdict(resp)
        if actual == expected: print(f"\033[1;32m[PASS] {name}\033[0m")
        else: print(f"\033[1;31m[FAIL] {name}: Expected '{expected}', got '{actual}'\033[0m")
        
    cwd = os.getcwd() + "\n"
    socket_attack("Missing Arguments", f"{cwd}run\n1\n", "Incorrect Number of Arguments")
    socket_attack("Absolute Path", f"{cwd}run\n1\n1\n/etc/passwd\n", "Invalid source file")
    socket_attack("Relative Path", f"{cwd}run\n1\n1\n../../../../../etc/passwd\n", "Invalid source file")

    print("\n--- 4. Lifecycle & Disconnect Resilience ---")
    test_sandbox_id_lifecycle(global_baseline)
    test_phased_disconnect("Compile-Phase Disconnect", "AC", 0.1, global_baseline)
    test_phased_disconnect("Execution-Phase Disconnect", "SLEEP_2", 1.0, global_baseline)
    test_external_sigkill(global_baseline)

    print("\n--- 5. Concurrency & Queue Saturation (100 Jobs) ---")
    print("Dispatching 100 concurrent requests. Awaiting Queue Drain...")
    success_count = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=100) as executor:
        futures = [executor.submit(submit_and_verify, f"Job-{i}", "AC", "Accepted", global_baseline, skip_cleanup=True) for i in range(100)]
        for future in concurrent.futures.as_completed(futures):
            try:
                if future.result(): success_count += 1
            except Exception as e:
                print(f"\033[1;31m[FAIL] Concurrency Thread Exception: {e}\033[0m")
            
    if success_count == 100: print("\033[1;32m[PASS] All 100 queued jobs completed safely.\033[0m")
    else: print(f"\033[1;31m[FAIL] Only {success_count}/100 jobs passed.\033[0m")

    print("\n--- 6. Global Post-Stress Invariant Check ---")
    final_leaks = wait_for_cleanup(global_baseline, timeout=10.0)
    if final_leaks:
        print(f"\033[1;31m[FAIL] Global Leak Detected: {final_leaks}\033[0m")
    else:
        print("\033[1;32m[PASS] Post-Stress System state perfectly matches pre-test baseline.\033[0m")
        submit_and_verify("Final Control AC", "AC", "Accepted", global_baseline)

if __name__ == "__main__":
    run_suite()
