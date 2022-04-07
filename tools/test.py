import serial
import sys

try:
    filename = sys.argv[1]
    port = sys.argv[2]
except(IndexError):
    exit(1)

try:
    with open(filename) as f:
        ser = serial.Serial(port, 115200)
        for line in f:
            ser.write(str.encode(line))
except(FileNotFoundError):
    print("File Not Found!")
    exit(1)
