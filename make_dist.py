import sys
import os

DIST="dist"

if not os.path.isdir(DIST):
    os.mkdir(DIST)

# make header file
header = ""

with open("bloom.h") as f:
    header += f.read()

header += "\n"

with open("murmur2/murmurhash2.h") as f:
    header += f.read()


with open(os.path.join(DIST, "bloom.h"), "w+") as f:
    f.write(header)

implementation = ""

with open("bloom.c") as f:
    implementation += f.read()

implementation += "\n"

with open("murmur2/MurmurHash2.c") as f:
    implementation += f.read()

with open(os.path.join(DIST, "bloom.c"), "w+") as f:
    f.write(implementation)


