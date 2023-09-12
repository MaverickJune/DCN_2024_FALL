TARGET=http_server
OBJECTS=socket_util.o http_util.o

CC=gcc

OBJDIR=./obj/
VPATH=.
DEPS = $(wildcard *.h)

OPTS=-O0 -g
LDFLAGS=
COMMON= 
CFLAGS= -Wall 

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