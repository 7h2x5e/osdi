from pwn import *
import sys

try:
    filename = sys.argv[1]
    port = sys.argv[2]
except(IndexError):
    exit(1)

try:
    with open(filename) as f:
        io = serialtube(port, baudrate=115200, convert_newlines=False)
        for line in f:
            io.send(str.encode(line))

except(FileNotFoundError):
    print("File Not Found!")
    exit(1)
