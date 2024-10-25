import sys
import math
import subprocess
import resource

CSV_FILE = ""
GRADING_TESTCASES = "grading_testcases"
TESTS = {1: 12, 2: 19, 3: 26, 4: 33}
TIMEOUT = 4  # timeout in sec
MAX_MEM = round(2.5 * (1024**3))

GRADING_PARAMS = {
    "undel_a": 20.0,
    "undel_b": 0.4,
    "wrongdel": 4.0,
    "nr": 1.0,
    "dist": 1.0,
}

MAKE_COMMAND = "make -j CXXFLAGS='-MMD -MP -O3'"


def shifted(f):
    at0 = f(0)
    at1 = f(1)
    return lambda x: ((f(x) - at1) / (at0 - at1))


def logistic(x, a, b):
    return 1.0 / math.exp(a * (x - b))


def nexp(x, c):
    return math.exp(-c * x)


def grade_for_a_run(output):
    packets_transmitted = float(output[0])
    ideal_packets_transmitted = float(output[1])
    packets_distance = float(output[2])
    ideal_packets_distance = float(output[3])
    nr_undelivered_segments = float(output[4])
    nr_wrongly_delivered_segments = float(output[5])
    nr_segments = float(output[6])

    undel = nr_undelivered_segments / nr_segments
    wrongdel = nr_wrongly_delivered_segments / nr_segments
    trans = 1.0 - packets_transmitted / ideal_packets_transmitted
    dist = 1.0 - packets_distance / ideal_packets_distance

    l = lambda x: logistic(x, GRADING_PARAMS["undel_a"], GRADING_PARAMS["undel_b"])
    undel_score = shifted(l)(undel)

    l = lambda x: nexp(x, GRADING_PARAMS["wrongdel"])
    wrongdel_score = shifted(l)(wrongdel)

    efficiency_score = nexp(math.fabs(trans), GRADING_PARAMS["nr"]) * nexp(
        math.fabs(dist), GRADING_PARAMS["dist"]
    )

    return undel_score * wrongdel_score * efficiency_score


# run 'make -j' and get the error code and output
# if the error code is 0, then the compilation is successful
def compile():
    allowed_headers_list = {
        "rp.h": set(["src/node_impl/../node.h"]),
        "rp.cc": set(["src/node_impl/../node.h", "src/node_impl/rp.h"]),
    }
    for file, allowed_headers in allowed_headers_list.items():
        out = (
            subprocess.check_output(
                "g++ -H src/node_impl/%s -o /dev/null 2>&1 | grep -e '\.\.* [^/]' | awk '{print $2}'"
                % (file),
                shell=True,
            )
            .decode("utf-8")
            .strip()
        )
        out = set(out.split()) - allowed_headers
        if out != set():
            return -1, "Compilation error: Disallowed headers: " + str(out)

    result = subprocess.run(
        MAKE_COMMAND, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    return result.returncode, "Compilation error: " + result.stderr.decode("utf-8")


def limit_virtual_memory():
    resource.setrlimit(resource.RLIMIT_AS, (MAX_MEM, resource.RLIM_INFINITY))


def grade_testcase(i):
    try:
        p = subprocess.run(
            f"./bin/main rp {GRADING_TESTCASES}/{i}.netspec {GRADING_TESTCASES}/{i}.msgs -g",
            shell=True,
            timeout=TIMEOUT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            preexec_fn=limit_virtual_memory,
        )
    except subprocess.TimeoutExpired:
        return (0.0, "Runtime error: Timed out")
    if p.returncode != 0:
        return (0.0, "Runtime error: " + p.stderr.decode("utf-8"))
    out = [
        x.split()
        for x in p.stdout.decode("utf-8").strip().split(("=" * 50 + "\n") * 2)[1:]
    ]
    marks = [grade_for_a_run(x) for x in out]
    return (sum(marks) / len(marks), None)


def grade(roll):
    grade = dict([(i, (0.0, None)) for i in TESTS.keys()])

    error_code, reason = compile()
    if error_code != 0:
        return (grade, reason)

    for i, weight in TESTS.items():
        g, r = grade_testcase(i)
        grade[i] = (weight * g, r)

    return (grade, None)


if __name__ == "__main__":
    g = grade("lmao")
    if g[1] is not None:
        print(g[1])
    total = 0.0
    for _, m in g[0].items():
        total += m[0]
    g[0]["total"] = round(total * 10) / 10.0
    print(g[0])
