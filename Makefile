all: engine


engine: force_check
	cd engine && make


clean:
	cd engine && make clean

force_check:
	@true
