#!/bin/sh
g++ -c -m32 -arch i386 -g -arch x86_64 -msse3 -O2 -DNDEBUG -I../../Include -I/usr/include/ni -I/usr/X11/include -I/Users/toku1938/Documents/freeglut-2.8.0/include/  -o Release/main.o main.cpp
g++ -c -m32 -arch i386 -g -arch x86_64 -msse3 -O2 -DNDEBUG -I../../Include -I/usr/include/ni -I/usr/X11/include -I/Users/toku1938/Documents/freeglut-2.8.0/include/  -o Release/SceneDrawer.o SceneDrawer.cpp
/bin/sh /Users/toku1938/Documents/freeglut-2.8.0/libtool --tag=CC   --mode=link g++ -I/Users/toku1938/Documents/freeglut-2.8.0/include -I/usr/X11R6/include -g -O2 -o ../Bin/Release/aom-ORF ./Release/main.o ./Release/SceneDrawer.o /Users/toku1938/Documents/freeglut-2.8.0/src/libfreeglut.la -framework OpenGL -framework GLUT -lGL -lXext -lX11 -lXrandr -lXxf86vm -lSM -lICE -lfreeglut -arch x86_64 -L../../Lib -L/usr/X11R6/lib -L/usr/local/lib /Users/toku1938/Documents/freeglut-2.8.0/src/.libs/libfreeglut.dylib -L../Bin/Release -lOpenNI -lGL -lXext -lX11 -lXrandr -lXxf86vm -lSM -lICE -lfreeglut


