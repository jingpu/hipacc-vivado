CC = clang++
CC = g++

HIPACC_DIR   ?= /horowitz/users/jingpu/nobackup/hipacc-vivado/build/release

MYFLAGS      ?= -D WIDTH=2048 -D HEIGHT=2048 -D SIZE_X=5 -D SIZE_Y=5
CFLAGS        = $(MYFLAGS) -std=c++11 -lpthread -Wall -Wunused \
                -I$(HIPACC_DIR)/include/dsl
LDFLAGS       = -lm
OFLAGS        = -O3

ifeq ($(CC),clang++)
    # use libc++ for clang++
    CFLAGS   += -stdlib=libc++ \
                -I`/usr/bin/clang -print-file-name=include` \
                -I`/usr/bin/llvm-config --includedir` \
                -I`/usr/bin/llvm-config --includedir`/c++/v1
    LDFLAGS  += -L`/usr/bin/llvm-config --libdir` -lc++ -lc++abi
else
    LDFLAGS  += -lstdc++
endif


BINARY = test
BINDIR = bin
OBJDIR = obj
SOURCES = $(shell echo *.cpp)

OBJS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)
BIN = $(BINDIR)/$(BINARY)


all: $(BINARY)

$(BINARY): $(OBJS) $(BINDIR)
	$(CC) -o $(BINDIR)/$@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp $(OBJDIR)
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ -c $<

$(BINDIR):
	mkdir bin

$(OBJDIR):
	mkdir obj


clean:
	rm -f $(BIN) $(OBJS)
	@echo "all cleaned up!"

distclean: clean
	rm -rf $(BINDIR) $(OBJDIR)

run: $(BINARY)
	$(BIN)

