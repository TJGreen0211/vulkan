CC = cl.exe
CFLAGS = ""
#Wall - W0 - W4
PROG = cube.exe

MAKE_DIR = %cd%

SRCS = Main.c \
	linearAlg.c \
	arcballCamera.c


#SRCS := $(shell echo dir1/*.c)
INCLUDE_PATHS = -I "..\dependencies\lib" -I "..\dependencies\include" -I "..\dependencies\include\vulkan"
INCLUDE_PATHS = $(INCLUDE_PATHS) -I $(MAKE_DIR)
LIBS = C:\Users\TJ\Documents\programming\vulkan\dependencies\lib\glfw3dll.lib C:\Users\TJ\Documents\programming\vulkan\dependencies\lib\SOIL.lib C:\Users\TJ\Documents\programming\vulkan\dependencies\lib\vulkan-1.lib

all: $(PROG)
	@echo "Building"

$(PROG):$(SRCS)
	$(CC) /nologo /MDd /W3 /DNDEBUG  /D_CRT_SECURE_NO_DEPRECATE $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SRCS) /link $(LIBS) /NODEFAULTLIB:libcmt.lib

clean:
	del main.obj
