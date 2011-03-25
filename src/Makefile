CC=gcc 
CFLAGS=-c -Wall
DEFINES=-D GCC -D__MSVCRT_VERSION__=0x0800 -D "_ASSERT(_x)" -D WIN32 -D NDEBUG -D _CONSOLE -D _UNICODE -D UNICODE
LDFLAGS=-dM $(INCLUDEDIRS)
SOURCES=main.cpp stdafx.cpp Win32Clusterd.cpp Win32ClusterdConfig.cpp DebugLog.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=Win32Clusterd
INCLUDEDIRS=-I "C:\devkit\mingw\include"

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	
.cpp.o:
	$(CC) $(CFLAGS) $(DEFINES) $< -o $@
