CFLAGS   = -O2 -std=c++11 -Wall -Wextra -Wuninitialized -Wunused -Wfloat-equal -Wshadow -Wunreachable-code -Wconversion -Winline -finline-functions -Wl,--retain-symbols-file=di.symbol -Wl,--version-script=di.export
DEBUG	 =
PICFLAGS = -fPIC
SHFLAGS  = -shared
COMPILE	 = -c
OUTFLAGS = -o
CC       = clang++
LIBS.a 	 =
LIBS.so  = -ldl
LIBDIR   = -L./
INCDIR   = -I./
TARGETDIR= ./
SOURCE   = $(wildcard *.cpp)
DIR      = $(notdir $(SOURCE))
OBJS     = $(DIR:%.cpp=%.o)

TARGET   = libv4l2Agent.so

all:$(TARGET)
	@echo "------\nSuccessfully Compiled!\n------"

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(SHFLAGS) $(INCDIR) $(LIBDIR) $^ $(LIBS.a) $(LIBS.so) $(OUTFLAGS) $@

$(OBJS) : $(SOURCE)
	$(CC) $(CFLAGS) $(PICFLAGS) $(INCDIR) $(COMPILE) $^

.PHONY : clean
clean:
	@rm -rf *.o
