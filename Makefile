all: engine


engine: force_check
	cd engine && make
	cd client && make
	cd server && make

clean:
	cd engine && make clean
	cd client && make clean
	cd server && make clean

force_check:
	@true
