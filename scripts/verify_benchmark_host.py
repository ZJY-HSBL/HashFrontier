#!/usr/bin/env python3
"""Fail-closed standard host admission. A local check is not remote attestation."""
import argparse,json,os,platform,re,subprocess
from pathlib import Path

def command_output(cmd):
    try: return subprocess.check_output(cmd,text=True,stderr=subprocess.DEVNULL,timeout=5).strip()
    except (OSError,subprocess.CalledProcessError,subprocess.TimeoutExpired):return ""
def read_text(p):
    try:return Path(p).read_text(encoding="utf-8").strip()
    except OSError:return ""
def parse_os_release(raw):
    out={}
    for line in raw.splitlines():
        if "=" in line and not line.startswith("#"):
            k,v=line.split("=",1);out[k]=v.strip().strip('"')
    return out
def current_host(m):
    cpu=read_text("/proc/cpuinfo")
    cpu_match=re.search(r"^model name\s*:\s*(.*?)\s*$",cpu,re.MULTILINE)
    mem_match=re.search(r"^MemTotal:\s*(\d+)\s+kB",read_text("/proc/meminfo"),re.MULTILINE)
    pin=m.get("cpu_affinity_logical_id")
    governor=read_text(f"/sys/devices/system/cpu/cpu{pin}/cpufreq/scaling_governor") if pin is not None else ""
    nodes=sorted(Path(f"/sys/devices/system/cpu/cpu{pin}").glob("node[0-9]*")) if pin is not None else []
    return {"cpu_model":cpu_match.group(1) if cpu_match else "",
       "memory_bytes":int(mem_match.group(1))*1024 if mem_match else 0,
       "os_release":parse_os_release(read_text("/etc/os-release")),
       "compiler_version":command_output([m["compiler_command"],"-dumpfullversion","-dumpversion"]),
       "kernel":platform.release(),"machine_arch":platform.machine(),
       "cpu_affinity_allowed":sorted(os.sched_getaffinity(0)) if hasattr(os,"sched_getaffinity") else [],
       "observed_governor":governor,
       "observed_numa_node":int(nodes[0].name[4:]) if nodes else -1}
def evaluate(m,h):
    checks={}
    def add(name,ok,detail):checks[name]={"passed":bool(ok),"detail":str(detail)}
    pin=m.get("cpu_affinity_logical_id")
    add("host_bound",m.get("stage")=="physically_bound_and_frozen" and bool(m.get("machine_binding_id")),m.get("stage"))
    add("cpu",m["cpu_model_required"].lower() in h["cpu_model"].lower(),h["cpu_model"])
    add("memory",h["memory_bytes"]>=m["minimum_memory_bytes"],h["memory_bytes"])
    add("os",all(h["os_release"].get(k)==v for k,v in m["os_release"].items()),h["os_release"])
    add("compiler",h["compiler_version"]==m["compiler_version_required"],h["compiler_version"])
    add("cpu_affinity_bound",isinstance(pin,int) and pin in h["cpu_affinity_allowed"],pin)
    add("governor",pin is not None and h["observed_governor"]==m["cpu_governor_required"],h["observed_governor"])
    add("numa",m.get("numa_node_id") is not None and h["observed_numa_node"]==m.get("numa_node_id"),h["observed_numa_node"])
    add("allocator_bound",bool(m.get("allocator_id")),m.get("allocator_id"))
    return {"profile":m["hardware_profile"],"formal_host_eligible":all(x["passed"] for x in checks.values()),"checks":checks,"host":h,"warning":"This is local eligibility only, not remote attestation. No official cross-paper performance numbers until all gates pass."}
def main():
    p=argparse.ArgumentParser();p.add_argument("--manifest",default="benchmarks/frozen-platform-v1.0.json")
    p.add_argument("--out");p.add_argument("--require-formal",action="store_true");a=p.parse_args()
    m=json.loads(Path(a.manifest).read_text());r=evaluate(m,current_host(m))
    data=json.dumps(r,indent=2,ensure_ascii=False)+"\n"
    if a.out:Path(a.out).write_text(data)
    print(data,end="")
    if a.require_formal and not r["formal_host_eligible"]:raise SystemExit(2)
if __name__=="__main__":main()
