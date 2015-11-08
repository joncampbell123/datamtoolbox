@echo off
cd Apple
call cleanup.bat
cd ..

cd common
call cleanup.bat
cd ..

cd Demoscene
call cleanup.bat
cd ..

cd Macromedia
call cleanup.bat
cd ..

cd Microsoft
call cleanup.bat
cd ..

cd Other
call cleanup.bat
cd ..

del *.ncb
del *.opt
