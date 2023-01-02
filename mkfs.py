import fs as fat

fs = fat.open_fs('fat://bin/os.bin')

f = fs.open('blank.elf', 'wb')
f2 = open('./programs/blank/blank.elf', 'rb')
f.write(f2.read())
f.close()
f2.close()
