import serial
import sys
import numpy as np
import argparse
from time import sleep

parser = argparse.ArgumentParser()
parser.add_argument("--dev", help="serial device")
parser.add_argument("--rate", help="baudrate, default: 115200")
parser.add_argument("--kernel", help="kernel image")
parser.add_argument("--addr", help="load addressm default: 80000")
args = parser.parse_args()

def main():
    if args.dev and args.kernel:
        dev = args.dev
        kernel = args.kernel
        rate = 115200
        addr = "80000"
        if args.rate:
            rate=int(args.rate)
        if args.addr:
            addr = args.addr
        print(f"Serial device: {dev}")
        print(f"Baudrate: {rate}")
        print(f"Kernel image: {kernel}")
        print(f"Load address: {addr}")
    else:
        print("Insufficient arguments!")
        exit(1)    

    try:
        ser = serial.Serial(dev, rate)
    except:
        print("Cannot open device")
        exit(1)
    
    try:
        with open(kernel, 'rb') as k:
            kernel_data = k.read()
        kernel_size = len(kernel_data)
    except(FileNotFoundError):
        print("File not found")
        exit(1)

    print(f"Image size: {kernel_size} bytes, load address: 0x{addr}")
    ser.write(str.encode("loadimg\n"))
    ser.write(str.encode(addr + "\n"))
    ser.write(kernel_size.to_bytes(4, byteorder = 'little'))

    chunk_size = 16
    chunk_count = (kernel_size + chunk_size - 1) // chunk_size
    for i in range(chunk_count):
        sys.stdout.write('\r')
        sys.stdout.write("%d/%d" % (i+1, chunk_count))
        sys.stdout.flush()
        ser.write(kernel_data[i*chunk_size: (i+1)*chunk_size])
    print("...done!")

if __name__ == "__main__":
    main()