
MKDIR   := mkdir
RMDIR   := rm -r
RM      := rm
ZIP     := zip
CC      := gcc
BIN     := ./bin
SRCDIRS := ./src ./app ./src/solr
SRCS    := $(foreach fd, $(SRCDIRS), $(wildcard $(fd)/*.c))
OBJS    := $(patsubst %.c,%.o,$(SRCS))
DEPS    := $(patsubst %.o,%.d,$(OBJS))
REDBEAN := $(BIN)/redbean.com
SOLR    := $(BIN)/solre
SOLRWEB := $(wildcard ./src/solr/web/*) $(SOLR)
#CCROK   := $(BIN)/ccrok
#TCHESS := $(BIN)/tchess
EXES    := $(SOLR)
CFLAGS  := $(addprefix -I,$(SRCDIRS)) $(XTRAFLAGS)
LDLIBS  := 
COSMOFLAGS := -g -Os -static -fno-pie -no-pie -nostdlib -nostdinc -gdwarf-4 \
  -fno-omit-frame-pointer -pg -mnop-mcount -mno-tls-direct-seg-refs \
  -Wl,--gc-sections -fuse-ld=bfd -Wl,--gc-sections -Wl,-T,cosmo/ape.lds \
  -include cosmo/cosmopolitan.h 

ifeq ($(m),cosmo)
CFLAGS := $(CFLAGS) $(COSMOFLAGS)
EXES := $(addsuffix .com.dbg,$(EXES))
endif
ifeq ($(m),release)
CFLAGS := $(CFLAGS) "-03"
endif
ifeq ($(m),debug)
CFLAGS := $(CFLAGS) "-g -fsanitize=undefined -fsanitize-undefined-trap-on-error \
  -fno-omit-frame-pointer -O0 -DDEBUG -Wuninitialized"
endif

COMMON  := $(addprefix ./src/, ivec.o memutil.o net.o phys.o sched.o serde.o)

.PHONY: all printcflags pack clean
all: $(EXES)
pack: $(REDBEAN)
printcflags:
	@echo $(CFLAGS)

include makefile.defs
include $(DEPS)

$(SOLR): $(COMMON) ./app/solr.o ./src/solr/solr.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BIN):
	$(MKDIR) $@

clean:
	-$(RM) $(OBJS)
	-$(RM) $(EXES)

$(SOLR).com.dbg: $(COMMON) ./app/solr.o ./src/solr/solr.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) \
	cosmo/crt.o cosmo/ape-no-modify-self.o cosmo/cosmopolitan.a

$(REDBEAN): $(SOLRWEB)
	$(ZIP) -j $(REDBEAN) $^