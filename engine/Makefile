# We don't overwrite the env CPP var if set
ifeq ($(origin CPP),default)
CPP = clang++
endif

CFLAGS ?= -Wall -std=c++11 -O2
LFLAGS += -lSDL2 -lm -lSDL2_gfx -lSDL2_image

SRCDIR = src
BUILDDIR = build

SRCS = $(wildcard $(SRCDIR)/*.cpp)

OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))

ALL: $(OBJS)
	ar rvs ../libengine.a $(OBJS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(BUILDDIR)
	$(CPP) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -f $(OBJS)
