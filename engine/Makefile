CPP = clang++

CFLAGS = -Wall -std=c++11 -O2
LFLAGS = -lSDL2 -lm -lSDL2_gfx

OBJS = camera.o colour.o ray.o renderer.o ../demo/main.o

ALL: $(OBJS)
	$(CPP) $(CFLAGS) $(LFLAGS) $(OBJS) -o ../main

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
