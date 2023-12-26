#!/usr/bin/env python3

import math

thing = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   width="WIDTH"
   height="HEIGHT"
   viewBox="0 0 WIDTH HEIGHT"
   version="1.1"
   id="svg1"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:svg="http://www.w3.org/2000/svg">

  <g id="layer1">
    <path
       id="path1"
       d="PATH_DATA" fill="#ffffff" stroke="#000000" stroke-width="4" />
  </g>
</svg>"""

def part(angle: float, radius: float, mx: int, my: int, swap: bool) -> str:
    angle = math.radians(angle)
    dx = radius * math.sin(angle)
    dy = radius - radius * math.cos(angle)

    if swap:
        dx, dy = dy, dx

    dx *= mx
    dy *= my
    return f"a {radius} {radius} 0 0 0 {dx} {dy}"

def main(argv):
    file   =       argv[1]
    size   = float(argv[2])
    radius = float(argv[3])
    angle  = float(argv[4])

    width = size
    height = size

    mirrors = [ (-1, 1), (1, 1), (1, -1), (-1, -1) ]
    swaps = [False, True, False, True]
    i = 0

    path = f"M {width / 2} {height / 2} L {width / 2} {height / 2 - radius}"

    while angle > 0:
        mx, my = mirrors[i]
        path += " " + part(min(angle, 90), radius, mx, my, swaps[i])
        i += 1
        angle -= 90

    path += " z"

    with open(file, "w") as f:
        result = thing.replace("PATH_DATA", path) \
                      .replace("WIDTH", str(width)) \
                      .replace("HEIGHT", str(height))

        f.write(result)

if __name__ == "__main__":
    import sys
    main(sys.argv)
