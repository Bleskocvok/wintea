## Wintea

Simple countdown program displaying time remaining until your tea is ready.

[Download latest release](https://github.com/Bleskocvok/wintea/releases/latest)

You can download the executable file (`tea.exe`), place it somewhere and run it to begin
countdown.

It can be run from powershell as follows to begin countdown for 5 minutes:

```ps
.\tea.exe 5:00
```

It can also be run by double-clicking in which case the program prompts for
time which is entered in the same format.

### WSL

Originally intended to be used inside WSL. In that case the `install.sh` script
should be followed as it places files in the appropriate places. The script
compiles the source code, so it expects that you have setup development
environment (either MinGW or MSVC and Python3 and ImageMagick).

In WSL (using MinGW):
```sh
git clone https://github.com/Bleskocvok/wintea.git
cd wintea
sh ./install.sh
```

Using MSVC:
```sh
git clone https://github.com/Bleskocvok/wintea.git
cd wintea
USING_MINGW=0 sh ./install.sh
```

After installation succeeds, you can run the program from the command line in
WSL (assuming you have made sure to have `~/bin` in the `PATH` environment
variable).
```sh
tea 5:00
tea
```
