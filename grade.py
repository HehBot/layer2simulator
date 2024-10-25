import sys
import math
import subprocess

NUM_TESTS = 2
CSV_FILE = ''
GRADING_TESTCASES = 'example_testcases'
TIMEOUT = 4

GRADING_PARAMS = {
        "undelivered": 1.0,
        "wrongly_delivered": 1.0,
        "nr_packets": 1.0,
        "packets_distance": 1.0,
}

def grade_for_a_run(output):
    nr_segments = float(output[6])
    nr_undelivered_segments = float(output[4])
    nr_wrongly_delivered_segments = float(output[5])
    packets_transmitted = float(output[0])
    ideal_packets_transmitted = float(output[1])
    packets_distance = float(output[2])
    ideal_packets_distance = float(output[3])

    accuracy_score = math.exp(-GRADING_PARAMS["undelivered"]*nr_undelivered_segments / nr_segments) * math.exp(-GRADING_PARAMS["wrongly_delivered"] * nr_wrongly_delivered_segments / nr_segments)
    efficiency_score = math.exp(-GRADING_PARAMS["nr_packets"] * math.fabs(packets_transmitted - ideal_packets_transmitted) / ideal_packets_transmitted) * math.exp(-GRADING_PARAMS["packets_distance"] * math.fabs(packets_distance - ideal_packets_distance) / ideal_packets_distance)

    return accuracy_score * efficiency_score

# run 'make -j > compilation.txt' and get the error code
# if the error code is 0, then the compilation is successful
# if the error code is not 0, then the compilation is not successful
def compile():
    # run 'g++ -H rp.h -o /dev/null 2>&1 | grep -e '\.\.* [^/]' | awk '{print $2}'' and get the output in a variable
    # and check out is equal to '../node.h'

    out = subprocess.check_output("g++ -H src/node_impl/rp.h -o /dev/null 2>&1 | grep -e '\.\.* [^/]' | awk '{print $2}'", shell=True).decode('utf-8')

    if out.strip() != 'src/node_impl/../node.h':
        return -1

    out = subprocess.check_output("g++ -H src/node_impl/rp.cc -o /dev/null 2>&1 | grep -e '\.\.* [^/]' | awk '{print $2}'", shell=True).decode('utf-8')

    out = sorted(out.strip().split('\n'))
    
    if out != ['src/node_impl/../node.h', 'src/node_impl/rp.h']:
        return -1

    error_code = subprocess.call(['make', '-j'], stdout=open('compilation.txt', 'w'))
    return error_code

def grade_testcase(i):
    try:
        out = subprocess.check_output(f'./bin/main rp {GRADING_TESTCASES}/{i}.netspec {GRADING_TESTCASES}/{i}.msgs -g', shell=True, timeout=TIMEOUT, stderr=subprocess.PIPE).decode('utf-8')
    except subprocess.TimeoutExpired:
        return 0.0, "Timed out"
    except subprocess.CalledProcessError as e:
        return 0.0, str(e)
    out = [x.split() for x in out.strip().split(('='*50+'\n')*2)[1:]]
    marks = [grade_for_a_run(x) for x in out]
    return sum(marks)/len(marks), "Ran successfully"

def grade(roll):
    grade = [0]*NUM_TESTS
    
    error_code = compile()
    if error_code != 0:
        return grade

    for i in range(NUM_TESTS):
        grade[i] = grade_testcase(i+1)

    return grade

if __name__ == '__main__':
    print(grade('lmao'))
