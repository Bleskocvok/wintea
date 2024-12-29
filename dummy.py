#!/usr/bin/env python3

import shutil
import os


def main(argv):
    filename = argv[1]
    folder = "icons"

    if not os.path.exists(folder):
        os.makedirs(folder)

    for i in range(1, 361):
        dst = f"{folder}/loading_{i:03d}.ico"
        shutil.copy(filename, dst)


if __name__ == "__main__":
    import sys
    main(sys.argv)
