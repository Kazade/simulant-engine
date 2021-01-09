#!/usr/bin/env python3

import sys
import re

if __name__ == '__main__':
    output_file = sys.argv[1]
    input_files = sys.argv[2:]
    
    found = []
    
    for f in input_files:
        content = open(f).read()
        
        matches = re.findall("(e?gl[A-Za-z0-9]+(ARB|EXT|PSP))", content)
        found.extend([x[0] for x in matches])
        
    found = list(set(found))
    found = sorted(found)
    
    print("\n".join(found))
    
    with open(output_file, "w") as f:
        for item in found:
            f.write('{"%s", (void (*)())%s},' % (item, item))

        
