from sys import argv
from subprocess import check_output

def run_suite(wat_exec, suite_file):
    test_count = 0
    failed = []

    with open(suite_file, 'r') as f:
        for filename in f.readlines():
            filename = filename.rstrip()

            print("========================================")
            
            input=None
            
            try:
                input = open("tests/" + filename + ".in").read().encode()
            except: pass

            result = "\n".join(check_output([wat_exec, "tests/" + filename], input=input).decode().splitlines())

            with open("tests/" + filename + ".out", 'r') as ex:
                expected_output = ex.read().rstrip()
                if result == expected_output:
                    print(filename + " passed")
                else:
                    print("Failed " + filename)
                    print("Expected:")
                    print(expected_output)
                    print("Actual:")
                    print(result)

if __name__ == "__main__":
    run_suite(argv[1], argv[2])
