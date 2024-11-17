#!/bin/sh

set -e

export DEST="/mnt/c/Tea/"

mkdir -p "$DEST"

./make_icons || exit 1

./build.sh || exit $?

mkdir -p "$HOME/bin"

echo '#!/bin/sh
/mnt/c/Tea/tea.exe $* &' > "$HOME/bin/tea"

chmod +x "$HOME/bin/tea"

echo 'Installation complete.'
printf 'Please make sure that ~/bin is in PATH. Simple check: '

if echo "$PATH" | tr ':' '\n' | grep --color=auto "$HOME/bin" > /dev/null; then
    printf '\033[32mSeems okay\033[0m (~/bin is in PATH)\n'
else
    printf '\033[31mDoes not seem okay (~/bin not in PATH)\033[0m\n'
fi
