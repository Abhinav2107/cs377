CS377 Project
File System
Abhinav
Anant
Aman
Darsh
Pratham
Amal


By default, our file system is mounted at /x/. It looks for the ide1 drive to mount.

Usage:

cd GeekOS/build
make
qemu-system-i386 diskc.img -drive path-to-fs-disk-image


To write cache back at the end, do a "touch /mountpoint/writecacheback" (Too hacky. I know.)

Use the standard linux commands with full paths
ls
mkdir
rm
cp
mv
touch
cat
chmod (with only one number for permission)
