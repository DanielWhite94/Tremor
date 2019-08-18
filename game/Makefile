CPP = clang++

CFLAGS = -Wall -std=c++11 -O2 -I../engine
LFLAGS = -lSDL2 -lm -lSDL2_gfx -lSDL2_image

OBJS = main.o

ALL: $(OBJS)
	$(CPP) $(CFLAGS) $(LFLAGS) $(OBJS) ../libengine.a -o ../main

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
