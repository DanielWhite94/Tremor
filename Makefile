all: engine


engine: force_check
	cd engine && make
	cd game && make

clean:
	cd engine && make clean
	cd game && make clean

force_check:
	@true
