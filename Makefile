OSTYPE := $(shell uname -s)

BIN_DIR = ../Bin

INC_DIRS = ../../Include /usr/include/ni /usr/X11/include /Users/toku1938/Documents/freeglut-2.8.0/include/

SRC_FILES = \
	main.cpp \
	SceneDrawer.cpp	

CFLAGS += -m32 -arch i386 -g

EXE_NAME = Second_wip

ifneq "$(GLES)" "1"
ifeq ("$(OSTYPE)","Darwin")
	LDFLAGS += -framework OpenGL -framework GLUT -lGL -lXext -lX11 -lXrandr -lXxf86vm -lSM -lICE -lfreeglut
else
	USED_LIBS += glut freeglut
endif
else
	DEFINES += USE_GLES
	USED_LIBS += GLES_CM IMGegl srv_um SM ICE
	SRC_FILES += opengles.cpp
endif

USED_LIBS += OpenNI GL Xext X11 Xrandr Xxf86vm SM ICE freeglut

LIB_DIRS += ../../Lib /usr/X11R6/lib /usr/local/lib /Users/toku1938/Documents/freeglut-2.8.0/src/.libs
include ../Build/Common/CommonCppMakefile
