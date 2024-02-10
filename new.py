import os
import sys

from update import update


allowed_chars = 'abcdefghijklmnopqrstuvwxyz'
allowed_chars += allowed_chars.upper()
allowed_chars += '0123456789_'

sketch_default = """void setup()
{

}

void loop()
{

}
"""

if __name__ == '__main__':
    args = sys.argv

    if len(args) == 1:
        print('E: need sketch name')
        exit()
    elif len(args) > 2:
        print('E: too many arguments')
        exit()
        
    sketch_name = args[1]
    for c in sketch_name:
        if c not in allowed_chars:
            print(f'E: character \"{c}\" is not allowed')
        
    folder_path = f'./programs/{sketch_name}'
    if os.path.exists(folder_path):
        print('E: sketch already exists')
        exit()
    
    os.mkdir(folder_path)
    
    with open(f'{folder_path}/{sketch_name}.ino', 'w') as f:
        f.write(sketch_default)

    src_path = f'{folder_path}/src'
    os.mkdir(src_path)
    
    with open(f'{src_path}/.gitignore', 'w') as f:
        f.write('microcode')
    
    update(folder_path)
    