@echo off
del *.plg
del *.opt
del *.ncb
del debug\*.obj
del debug\*.pch
del debug\*.pdb
del debug\*.idb
del release\*.obj
del release\*.pch
del release\*.pdb
del release\*.idb
rmdir debug
rmdir release

cd zlib
call cleanup.bat
cd ..
