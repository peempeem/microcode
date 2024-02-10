import os
import subprocess

def update(dir):
    subprocess.call(f'robocopy .\\microcode .\\{dir}\\src\\microcode /MIR /XD .*')

if __name__ == '__main__':
    dirs = os.listdir('./programs')

    for dir in dirs:
        update(f'./programs/{dir}')