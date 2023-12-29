#!/bin/sh

export DEST="/mnt/c/Tea/"

mkdir -p "$DEST"

./build.sh || exit $?

mkdir -p "$HOME/bin"

echo '#!/bin/sh
/mnt/c/Tea/tea.exe $* &' > "$HOME/bin/tea"

chmod +x "$HOME/bin/tea"

echo 'Installation successful. Please make sure that ~/bin is in PATH.'
