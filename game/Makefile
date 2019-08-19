# We don't overwrite the env CPP var if set
ifeq ($(origin CPP),default)
CPP = clang++
endif

CFLAGS ?= -Wall -std=c++11 -O2 -I../engine/src
LFLAGS += -lSDL2 -lm -lSDL2_gfx -lSDL2_image

SRCDIR = src
BUILDDIR = build

OUTFILE = ../main
ENGINELIB = ../libengine.a

SRCS = $(wildcard $(SRCDIR)/*.cpp)

OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))

ALL: $(OBJS)
	$(CPP) $(CFLAGS) $(LFLAGS) $(OBJS) $(ENGINELIB) -o $(OUTFILE)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
