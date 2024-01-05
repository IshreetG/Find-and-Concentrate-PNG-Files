# Makefile, ECE252  
# Yiqing Huang

CC = gcc       # compiler
CFLAGS = -Wall -g # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o
SRCS   = pnginfo.c catpng.c findpng.c crc.c zutil.c
TARGETS= pnginfo catpng findpng
OBJS   = main.o $(LIB_UTIL) 

# TARGETS= main.out 

all: ${TARGETS}

pnginfo: pnginfo.o zutil.o crc.o
	gcc -o $@ $^ -lz -g

catpng: catpng.o zutil.o crc.o
	gcc -o $@ $^ -lz -g

findpng: findpng.o zutil.o crc.o
	gcc -o $@ $^ -lz -g

# main.out: $(OBJS) 
# 	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
