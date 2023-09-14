TARGET=http_server
OBJECTS=http_util.o main.o http_server_ans.o

CC=gcc

OBJDIR=./obj/
VPATH=.
DEPS = $(wildcard *.h)

OPTS=-O0 -g
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