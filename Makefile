all: engine


engine: force_check
	cd engine && make
	cd game && make
	cd server && make

clean:
	cd engine && make clean
	cd game && make clean
	cd server && make clean

force_check:
	@true
