import re
import fileinput
import sys

REGEX = "(\d+) tests ([failed|crashed])"


if __name__ == '__main__':
    for line in sys.stdin:
        print(line, end="")
        
        if re.search(REGEX, line):
            print("DETECTED FAILURES")
            sys.exit(1)
    else:
        print("TESTS PASSED!")
        sys.exit(0)

