# NXC Data Communications Network Makefile for HTTP server
# Written by Jongseok Park (cakeng@snu.ac.kr)
# 2023. 9. 11


#### DO NOT MODIFY THIS FILE!! ####

TARGET=http_server
OBJECTS=http_util.o http_engine.o http_engine_ans.o

CC=gcc

OBJDIR=./obj/
VPATH=.
DEPS = $(wildcard *.h)

OPTS=-O3
LDFLAGS=
COMMON= 
CFLAGS= -Wall -Wno-unused-variable

OBJS= $(addprefix $(OBJDIR), $(OBJECTS))
EXEOBJSA= $(addsuffix .o, $(TARGET))
EXEOBJS= $(addprefix $(OBJDIR), $(EXEOBJSA))

all: obj $(TARGET) 
 
$(TARGET): $(EXEOBJS) $(OBJS)
	$(CC) $(COMMON) $(CFLAGS) $(OPTS) $^ -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.c $(DEPS)
	$(CC) $(COMMON) $(CFLAGS) $(OPTS) -c $< -o $@

obj:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(TARGET) $(EXEOBJS) $(OBJS) $(OBJDIR)