# NXC Data Communications Makefile for HTTP server
# Written by Jongseok Park (cakeng@snu.ac.kr)
# 2023. 9. 19


#### DO NOT MODIFY THIS FILE!! ####

TARGET=torrent
OBJECTS=torrent.o torrent_engine.o torrent_engine_ans.o torrent_ui.o torrent_utils.o
SAVE_DIR=./torrents

CC=gcc

OBJDIR=./obj/
VPATH=./src/
DEPS = $(wildcard *.h)

OPTS=-O3
LDFLAGS= 
COMMON= -D SAVE_DIR=\"$(SAVE_DIR)\" -Iinclude/
CFLAGS= -Wall -Wno-unused-variable

OBJS= $(addprefix $(OBJDIR), $(OBJECTS))

all: obj $(TARGET) 
 
$(TARGET): $(OBJS)
	$(CC) $(COMMON) $(CFLAGS) $(OPTS) $^ -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.c $(DEPS)
	$(CC) $(COMMON) $(CFLAGS) $(OPTS) -c $< -o $@

obj:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(TARGET) $(OBJS) $(OBJDIR) $(SAVE_DIR)