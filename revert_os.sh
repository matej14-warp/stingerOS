#!/bin/bash
# Revert the OS name back to stinger
echo "Renaming stinger back to stinger..."
find . -depth -name "*stinger*" -exec bash -c 'mv "$1" "${1//stinger/stinger}"' _ {} \;
grep -rl "stinger" . | xargs sed -i 's/stinger/stinger/g'
grep -rl "stinger" . | xargs sed -i 's/stinger/stinger/g'
grep -rl "stinger" . | xargs sed -i 's/stinger/stinger/g'
echo "Done."

