#!/bin/bash
export PATH=/ucrt64/bin:$PATH
cd "/c/Users/Prakhyath L/.gemini/antigravity/scratch"
echo "=== Compiler: $(which gcc) ==="
echo "=== pkg-config: $(which pkg-config) ==="
echo "=== GTK flags: $(pkg-config --cflags gtk+-3.0) ==="
gcc cnewel.c -o cnewel $(pkg-config --cflags --libs gtk+-3.0) -lm 2>&1
if [ $? -eq 0 ]; then
    echo "BUILD_SUCCESS"
else
    echo "BUILD_FAILED"
fi
