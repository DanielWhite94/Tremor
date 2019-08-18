all: engine


engine: force_check
	cd engine && make
	cd demo && make

clean:
	cd engine && make clean
	cd demo && make clean

force_check:
	@true
