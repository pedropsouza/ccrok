
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
CFLAGS  := $(addprefix -I,$(SRCDIRS)) $(XTRAFLAGS)
LDLIBS  := -lm -lraylib

COMMON  := $(addprefix ./src/, draw.o ivec.o memutil.o net.o phys.o sched.o serde.o)
SOLR_O  := ./app/solr.o ./src/solr/solr.o ./src/solr/cards_n_stacks.o ./src/solr/graphics.o ./src/solr/move.o ./src/solr/autosolve.o

all: $(EXES)
.PHONY: all

include makefile.defs
include $(DEPS)

$(SOLR): $(COMMON) $(SOLR_O) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BIN):
	$(MKDIR) $@

debug:
	$(MAKE) XTRAFLAGS:="-g -fsanitize=undefined -fsanitize-undefined-trap-on-error -fno-omit-frame-pointer -O0 -DDEBUG -Wuninitialized"
.PHONY: debug

release:
	$(MAKE) XTRAFLAGS:="-O3"
.PHONY: release

clean:
	-$(RM) $(OBJS)
	-$(RM) $(EXES)
.PHONY: clean
