#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Find all the Python files ending with .test.py
for file in $SCRIPT_DIR/*.test.py; do
    echo "Test > $(basename $file)"
    
    # run the test
    python "$file"

    echo ""
    echo ""
done
