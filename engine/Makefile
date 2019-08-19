# We don't overwrite the env CPP var if set
ifeq ($(origin CPP),default)
CPP = clang++
endif

CFLAGS ?= -Wall -std=c++11 -O2
LFLAGS += -lSDL2 -lm -lSDL2_gfx -lSDL2_image

OBJS = camera.o colour.o object.o ray.o renderer.o util.o

ALL: $(OBJS)
	ar rvs ../libengine.a $(OBJS)

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
