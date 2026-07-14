#!/bin/bash
export PATH=/ucrt64/bin:/usr/bin:$PATH
export MSYSTEM=UCRT64
export GDK_BACKEND=win32

LOG="/c/Users/Prakhyath L/.gemini/antigravity/scratch/cnewel_debug.log"
echo "" > "$LOG"

cd "/c/Users/Prakhyath L/.gemini/antigravity/scratch"
./cnewel.exe > "$LOG" 2>&1
echo "Exit code: $?" >> "$LOG"
cat "$LOG"
