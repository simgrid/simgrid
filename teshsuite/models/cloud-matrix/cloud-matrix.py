#!/usr/bin/env python3

# Copyright (c) 2026. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""Run the cloud-matrix test program, and check each line it prints against an expected outcome computed on the fly.

Usage: cloud-matrix.py [--filter TEXT] [--verbose] <cluster_multi.xml> <command...>
  <command...> runs the cloud-matrix program, possibly through a wrapper such as valgrind. The platform file and the
  filter are appended to it.
  --filter only runs the scenarios whose label contains TEXT, to debug them one at a time.
  --verbose shows every line of the program, with the outcome of its check.

This replaces tesh for this test: instead of comparing the output with a serialized expected output, this runner
recomputes the expected outcome of each scenario from its label. It fails if a scenario does not match, if the program
prints anything unexpected, if it does not print as many scenarios as it announces, or if it fails.

This script does not use SimGrid. It encodes the sharing rules of the VM model, as documented in VirtualMachineImpl:

- On a PM, each task running directly on the PM competes with a weight equal to its amount of threads, and can use at
  most one core per thread (or its bound, if lower, which applies to all its threads).
- A VM competes on its PM with a weight equal to the amount of threads it contains, but at most its amount of cores.
  It can use at most what its tasks need (one core per thread, or the bound of the bounded tasks), and never more than
  its amount of cores.
- Within a VM, the tasks share what the VM got on its PM, with the same weights and limits as on a PM.
- Sharing is max-min fair, in proportion of the weights: everybody gets the same amount per unit of weight, except
  the ones that are limited, whose share is distributed to the others.

The power of a PM follows the energy plugin: its idle power when nothing runs on it, and otherwise its epsilon power
plus its load (between 0 and 1) times the difference between its full power and its epsilon power. The speed, cores
and power profiles of the PMs are read from the platform file.
"""

import re
import subprocess
import sys
import xml.etree.ElementTree as ET

# The PMs used by the test, as in cloud-matrix.cpp
PM_HOSTS = {"pm1": "node-0.1core.org", "pm2": "node-0.2cores.org", "pm4": "node-0.4cores.org",
            "fast": "node-1.4cores.org"}
BOUND = {"o": float("inf"), "b": 0.5e9, "q": 0.25e9, "t": 0.75e9, "B": 2e9,  # total, for all the threads
         "d": float("inf"), "h": 1.5e9, "l": 0.5e9}
THREADS = {"d": 2, "h": 2, "l": 2}  # 1 for the other kinds
TOLERANCE = 1e-3

PMS = {}  # name -> {"speeds": [per pstate], "cores": n, "wattage": [(idle, epsilon, full) per pstate]}


def parse_speed(text):
    m = re.fullmatch(r"\s*([\d.eE+-]+)\s*([kMGT]?)f\s*", text)
    return float(m.group(1)) * {"": 1, "k": 1e3, "M": 1e6, "G": 1e9, "T": 1e12}[m.group(2)]


def load_platform(path):
    """Read the speeds, cores and power profiles of the hosts used by the test, from <host> and <cluster> tags"""
    hosts = {}
    for elem in ET.parse(path).iter():
        if elem.tag not in ("host", "cluster"):
            continue
        props = {p.get("id"): p.get("value") for p in elem.findall("prop")}
        spec = {"speeds": [parse_speed(s) for s in elem.get("speed").split(",")], "cores": int(elem.get("core", "1")),
                "wattage": [tuple(float(w) for w in state.split(":"))
                            for state in props.get("wattage_per_state", "0:0:0").split(",")]}
        if elem.tag == "host":
            hosts[elem.get("id")] = spec
        else:
            for part in elem.get("radical").split(","):
                first, _, last = part.partition("-")
                for i in range(int(first), int(last or first) + 1):
                    hosts[f"{elem.get('prefix')}{i}{elem.get('suffix')}"] = spec
    for name, host in PM_HOSTS.items():
        PMS[name] = hosts[host]


def share(capacity, items):
    """Weighted max-min sharing of 'capacity' between items given as (weight, limit). Returns the allocations."""
    alloc = [0.0] * len(items)
    active = [i for i, (weight, _) in enumerate(items) if weight > 0]
    while active:
        rate = capacity / sum(items[i][0] for i in active)  # per unit of weight
        limited = [i for i in active if items[i][1] <= items[i][0] * rate]
        if not limited:
            for i in active:
                alloc[i] = items[i][0] * rate
            break
        for i in limited:  # they get their limit, and the others share the rest
            alloc[i] = items[i][1]
            capacity -= items[i][1]
            active.remove(i)
    return alloc


def threads(kind):
    return THREADS.get(kind, 1)


def task_limit(kind, speed):
    return min(threads(kind) * speed, BOUND[kind])


def solve(placement, vm_pm, pstates):
    """Speed of each task (in flop/s), and power of each PM (in W).

    placement: list of (kind, location), where location is a PM name or a VM name ('A' or 'B')
    vm_pm: dict giving the PM and the amount of cores of each VM: {'A': (pm, cores), ...}
    pstates: dict giving the pstate of each PM"""
    speeds = [0.0] * len(placement)
    power = {}
    for pm, spec in PMS.items():
        speed = spec["speeds"][pstates[pm]]
        competitors = []  # (weight, limit, list of the indices of the tasks, or the name of the VM)
        for i, (kind, location) in enumerate(placement):
            if location == pm:
                competitors.append((threads(kind), task_limit(kind, speed), i))
        for vm, (host, cores) in vm_pm.items():
            if host != pm:
                continue
            tasks = [i for i, (_, location) in enumerate(placement) if location == vm]
            load = sum(task_limit(placement[i][0], speed) / speed for i in tasks)
            weight = sum(threads(placement[i][0]) for i in tasks)
            competitors.append((min(weight, cores), min(load, cores) * speed, vm))
        alloc = share(spec["cores"] * speed, [(weight, limit) for weight, limit, _ in competitors])
        for (_, _, what), amount in zip(competitors, alloc):
            if isinstance(what, int):
                speeds[what] = amount
            else:  # Share what the VM got between its tasks
                tasks = [i for i, (_, location) in enumerate(placement) if location == what]
                items = [(threads(placement[i][0]), task_limit(placement[i][0], speed)) for i in tasks]
                for i, rate in zip(tasks, share(amount, items)):
                    speeds[i] = rate
        load = sum(alloc) / (spec["cores"] * speed)
        idle, epsilon, full = spec["wattage"][pstates[pm]]
        power[pm] = epsilon + load * (full - epsilon) if load > 1e-12 else idle
    return speeds, power


def parse_label(label):
    m = re.fullmatch(r"(pm\d) A(\d)\[(\w*)\] B\[(\w*)\] C\[(\w*)\] P\[(\w*)\] F\[(o*)\] ev=(\w+) end=(\w+)", label)
    if not m:
        raise ValueError(f"Cannot parse the scenario label '{label}'")
    pm, a_cores, a_tasks, b_tasks, c_tasks, p_tasks, f_tasks, event, _ = m.groups()
    placement = ([(k, "A") for k in a_tasks] + [(k, "B") for k in b_tasks] + [(k, "C") for k in c_tasks] +
                 [(k, pm) for k in p_tasks] + [("o", "fast") for _ in f_tasks])
    vm_pm = {"A": (pm, int(a_cores)), "B": (pm, 1), "C": (pm, 1)}
    return placement, vm_pm, pm, event, len(a_tasks) + len(b_tasks) + len(c_tasks)


def apply_event(placement, vm_pm, pstates, pm, event, first_p):
    placement, vm_pm, pstates = list(placement), dict(vm_pm), dict(pstates)
    if event in ("exec_vm2pm", "actor_vm2pm"):
        placement[0] = (placement[0][0], pm)
    elif event in ("exec_pm2vm", "actor_pm2vm"):
        placement[first_p] = (placement[first_p][0], "A")
    elif event == "exec_vm2vm":
        placement[0] = (placement[0][0], "B")
    elif event in ("set_pm", "live_migration"):
        vm_pm["A"] = ("fast", vm_pm["A"][1])
    elif event == "dvfs":
        pstates[pm] = 1
    elif event == "dvfs_down":
        pstates[pm] = 0
    else:
        raise ValueError(f"Unknown event {event}")
    return placement, vm_pm, pstates


def format_phase(label, speeds, power):
    """Format a phase like the C++ program does"""
    m = re.fullmatch(r".*A\d\[(\w*)\] B\[(\w*)\] C\[(\w*)\] P\[(\w*)\] F\[(o*)\].*", label)
    sizes = [len(group) for group in m.groups()]
    groups, i = [], 0
    for size in sizes:
        groups.append(",".join(f"{s / 1e9:.4f}" for s in speeds[i:i + size]))
        i += size
    watts = "".join(f" {pm}={p:.4f}" for pm, p in power.items() if abs(p) > 1e-6) or " none"
    return ";".join(groups) + " W" + watts


def expected_line(label):
    placement, vm_pm, pm, event, first_p = parse_label(label)
    pstates = {name: 0 for name in PMS}
    pstates["fast"] = 1  # The test switches it to its fast pstate
    if event == "dvfs_down":
        pstates[pm] = 1
    line = label + " | P1 " + format_phase(label, *solve(placement, vm_pm, pstates))
    if event != "none":
        line += " | P2 " + format_phase(label, *solve(*apply_event(placement, vm_pm, pstates, pm, event, first_p)))
    return line + " | idle W none"


def numbers(text):
    """Speeds and powers of a phase, as a list of floats and a dict"""
    speeds_part, watts_part = re.split(r"(?:^| )W ", text, maxsplit=1)
    speeds = [float(x) for x in re.split(r"[;,]", speeds_part) if x]
    watts = {pm: float(p) for pm, p in re.findall(r"(\w+)=([\d.-]+)", watts_part)}
    return speeds, watts


def compare(obtained, expected):
    """Compare two lines with a tolerance, return a list of differences"""
    diffs = []
    obtained_phases = obtained.split(" | ")[1:]
    expected_phases = expected.split(" | ")[1:]
    if len(obtained_phases) != len(expected_phases):
        return ["different amount of phases"]
    for got, exp in zip(obtained_phases, expected_phases):
        name = exp.split(" ")[0]
        got_speeds, got_watts = numbers(got.split(" ", 1)[1])
        exp_speeds, exp_watts = numbers(exp.split(" ", 1)[1])
        if len(got_speeds) != len(exp_speeds) or any(abs(g - e) > TOLERANCE for g, e in zip(got_speeds, exp_speeds)):
            diffs.append(f"{name} speeds {got_speeds} instead of {exp_speeds}")
        for pm in set(got_watts) | set(exp_watts):
            if abs(got_watts.get(pm, 0) - exp_watts.get(pm, 0)) > TOLERANCE:
                diffs.append(f"{name} power of {pm} is {got_watts.get(pm, 0)} instead of {exp_watts.get(pm, 0)}")
    return diffs


def main():
    args = sys.argv[1:]
    filter_text, verbose = None, False
    while args and args[0].startswith("--"):
        option = args.pop(0)
        if option == "--filter":
            filter_text = args.pop(0)
        elif option == "--verbose":
            verbose = True
        else:
            sys.exit(f"Unknown option {option}\n\n{__doc__}")
    if len(args) < 2:
        sys.exit(__doc__)
    platform, command = args[0], args[1:]
    load_platform(platform)

    command = command + [platform, "--log=root.fmt:%m%n", "--log=host_energy.thres:warning"]
    if filter_text is not None:
        command.append(filter_text)

    checked, failures, unexpected, announced = 0, 0, [], None
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as program:
        for line in program.stdout:
            line = line.rstrip("\n")
            if m := re.fullmatch(r"(\d+) scenarios", line):
                announced = int(m.group(1))
                continue
            if not re.match(r"pm\d ", line):
                if not line.startswith("=="):  # Allow the output of valgrind
                    unexpected.append(line)
                    print(f"UNEXPECTED OUTPUT: {line}")
                continue
            checked += 1
            label = line.split(" | ")[0]
            diffs = compare(line, expected_line(label))
            if verbose:
                print(f"{'OK  ' if not diffs else 'FAIL'} {line}")
            if diffs:
                failures += 1
                print(f"MISMATCH: {label}")
                for diff in diffs:
                    print(f"    {diff}")
    status = program.returncode

    print(f"{checked} scenarios checked: {checked - failures} passed, {failures} failed")
    problems = []
    if status != 0:
        problems.append(f"the program returned {status}")
    if announced != checked:
        problems.append(f"the program announced {announced} scenarios, but printed {checked}")
    if unexpected:
        problems.append(f"the program printed {len(unexpected)} unexpected lines")
    if failures:
        problems.append(f"{failures} scenarios do not match")
    for problem in problems:
        print(f"FAILED: {problem}")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
