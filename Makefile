
MKDIR   := mkdir
RMDIR   := rm -r
RM      := rm
CC      := gcc
BIN     := ./bin
SRCDIRS := ./src ./app ./src/solr
SRCS    := $(foreach fd, $(SRCDIRS), $(wildcard $(fd)/*.c))
OBJS    := $(patsubst %.c,%.o,$(SRCS))
SO_OBJS := $(patsubst %.c,%.dyn.o,$(SRCS))
SOLR    := $(BIN)/solre
CROK    := $(BIN)/crok
#TCHESS  := $(BIN)/tchess
EXES    := $(SOLR) $(CROK)
CFLAGS  := $(addprefix -I,$(SRCDIRS)) $(shell sdl2-config --cflags) $(XTRAFLAGS)
LDLIBS  := -lm $(shell sdl2-config --libs) -ldl

COMMON_O  := $(addprefix ./src/, draw.o ivec.o net.o phys.o sched.o serde.o)
COMMON_SO_O := $(patsubst %.o,%.dyn.o,$(COMMON_O))
SOLR_O  := ./app/solr.o $(addprefix ./src/solr/, solr.o cards_n_stacks.o graphics.o move.o autosolve.o gui.o handlers.o)
SOLR_SO_O := $(patsubst %.o,%.dyn.o,$(SOLR_O))
SOLR_SO := $(BIN)/solr.so
CROK_O := ./app/crok.o $(addprefix ./src/crok/, crok.o board.o data.o gloop.o state.o unit.o)

all: $(EXES) $(SOLR_SO)
.PHONY: all

%.dyn.o: %.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

$(SOLR): src/solr/hotreload.c app/solr.c src/sched.c | $(BIN)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -o $@ $^ $(LDLIBS)

$(BIN):
	$(MKDIR) $@

$(SOLR_SO): $(SOLR_SO_O) $(COMMON_SO_O)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LDLIBS)

debug:
	$(MAKE) XTRAFLAGS:="-g -fsanitize=undefined -fsanitize-undefined-trap-on-error -fno-omit-frame-pointer -O0 -DDEBUG -Wuninitialized"
.PHONY: debug

release:
	$(MAKE) XTRAFLAGS:="-O3"
.PHONY: release

clean:
	-$(RM) $(OBJS)
	-$(RM) $(SO_OBJS)
	-$(RM) $(EXES)
.PHONY: clean
