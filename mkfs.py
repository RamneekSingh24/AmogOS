import fs as fat

def put_file(home_os_path, fs_path, fs):
    f = fs.open(fs_path, 'wb')
    f2 = open(home_os_path, 'rb')
    f.write(f2.read())
    f.close()
    f2.close()


fs = fat.open_fs('fat://bin/os.bin')

put_file('./programs/blank/blank.elf', 'blank', fs)
put_file('./programs/shell/shell.elf', 'shell', fs)

