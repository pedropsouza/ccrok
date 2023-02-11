
MKDIR   := mkdir
RMDIR   := rm -r
RM      := rm
CC      := gcc
BIN     := ./bin
SRCDIRS := ./src ./app ./src/solr
SRCS    := $(foreach fd, $(SRCDIRS), $(wildcard $(fd)/*.c))
OBJS    := $(patsubst %.c,%.o,$(SRCS))
DEPS    := $(patsubst %.o,%.d,$(OBJS))
SOLR    := $(BIN)/solre
#CCROK   := $(BIN)/ccrok
#TCHESS := $(BIN)/tchess
EXES    := $(SOLR)
CFLAGS  := $(addprefix -I,$(SRCDIRS)) $(shell sdl2-config --cflags) $(XTRAFLAGS)
LDLIBS  := -lm $(shell sdl2-config --libs)

COMMON  := $(addprefix ./src/, draw.o ivec.o memutil.o net.o phys.o sched.o serde.o)

all: $(EXES)
.PHONY: all

include makefile.defs
include $(DEPS)

$(SOLR): $(COMMON) ./app/solr.o ./src/solr/solr.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BIN):
	$(MKDIR) $@

clean:
	-$(RM) $(OBJS)
	-$(RM) $(OBJS)
.PHONY: clean
