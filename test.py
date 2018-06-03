from subprocess import check_output

def run_suite(wat_exec, suite_file):
    with open(suite_file, 'r') as f:
        for filename in f.readlines():
            check_output(wat_exec + " 
