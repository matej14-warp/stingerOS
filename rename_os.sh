#!/bin/bash
# Rename the OS to stinger
echo "Renaming stinger to stinger..."
grep -rl "stinger" . | xargs sed -i 's/stinger/stinger/g'
grep -rl "stinger" . | xargs sed -i 's/stinger/stinger/g'
grep -rl "stinger" . | xargs sed -i 's/stinger/stinger/g'
echo "Done."


