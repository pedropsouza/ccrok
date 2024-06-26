
MKDIR   := mkdir
RMDIR   := rm -r
RM      := rm
CC      := gcc
BIN     := ./bin
SRCDIRS := ./src ./app ./src/solr ./src/crok ./src/mahjongcell
SRCS    := $(foreach fd, $(SRCDIRS), $(wildcard $(fd)/*.c))
OBJS    := $(patsubst %.c,%.o,$(SRCS))
SO_OBJS := $(patsubst %.c,%.dyn.o,$(SRCS))
SOLR    := $(BIN)/solre
CROK    := $(BIN)/crok
MAHJONGCELL := $(BIN)/mahjcell
#TCHESS  := $(BIN)/tchess
SOLR_HOT := $(SOLR)-hot
CROK_HOT := $(CROK)-hot
MAHJONGCELL_HOT := $(MAHJONGCELL)-hot

EXES    := $(MAHJONGCELL) #$(SOLR) $(CROK) $(MAHJONGCELL)
CFLAGS  := $(addprefix -I,./src ./app) $(shell sdl2-config --cflags) $(XTRAFLAGS)
LDLIBS  := -lm $(shell sdl2-config --libs) -ldl -lSDL2_ttf

COMMON_O  := $(addprefix ./src/, draw.o ivec.o fvec.o net.o phys.o sched.o serde.o) libcrc/lib/libcrc.a
COMMON_SO_O := $(patsubst %.o,%.dyn.o,$(COMMON_O))

SOLR_O  := $(addprefix ./src/solr/, solr.o cards_n_stacks.o graphics.o move.o autosolve.o gui.o handlers.o)
SOLR_SO_O := $(patsubst %.o,%.dyn.o,$(SOLR_O))
SOLR_SO := $(BIN)/solr.so

CROK_O := $(addprefix ./src/crok/, crok.o board.o data.o gloop.o state.o unit.o net.o)
CROK_SO_O := $(patsubst %.o,%.dyn.o,$(CROK_O))
CROK_SO := $(BIN)/crok.so

MAHJONGCELL_O := $(addprefix ./src/mahjongcell/, solr.o cards_n_stacks.o graphics.o move.o autosolve.o gui.o handlers.o)
MAHJONGCELL_SO_O := $(patsubst %.o,%.dyn.o,$(MAHJONGCELL_O))
MAHJONGCELL_SO := $(BIN)/mahjcell.so

all: $(EXES)
hot: $(SOLR_HOT) $(CROK_HOT) $(SOLR_SO) $(CROK_SO) $(MAHJONGCELL_SO)
.PHONY: all

%.dyn.o: %.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

$(SOLR_HOT): src/solr/hotreload.c app/solr.c src/sched.c | $(BIN)
	$(CC) $(CFLAGS) -DENABLE_HOTRELOAD -Wl,-rpath=./ -o $@ $^ $(LDLIBS)

$(CROK_HOT): src/crok/hotreload.c app/crok.c src/sched.c | $(BIN)
	$(CC) $(CFLAGS) -DENABLE_HOTRELOAD -Wl,-rpath=./ -o $@ $^ $(LDLIBS)

$(MAHJONGCELL_HOT): src/mahjongcell/hotreload.c app/mahjongcell.c src/sched.c | $(BIN)
	$(CC) $(CFLAGS) -DENABLE_HOTRELOAD -Wl,-rpath=./ -o $@ $^ $(LDLIBS)

$(SOLR): app/solr.c $(SOLR_O) $(COMMON_O) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(CROK): app/crok.c $(CROK_O) $(COMMON_O) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(MAHJONGCELL): app/mahjongcell.c $(MAHJONGCELL_O) $(COMMON_O) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BIN):
	$(MKDIR) $@

$(SOLR_SO): $(SOLR_SO_O) $(COMMON_SO_O)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LDLIBS)

$(CROK_SO): $(CROK_SO_O) $(COMMON_SO_O)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LDLIBS)

$(MAHJONGCELL_SO): $(MAHJONGCELL_SO_O) $(COMMON_SO_O)
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
