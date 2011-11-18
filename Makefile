DEPS = babyfs.h p6.h
OBJ = block.o cache.o p6.o

%.o: %.c $(DEPS)
	cc -c -o $@ $<

runtest%: test%
	rm simulated_device
	dd if=/dev/zero of=simulated_device bs=1024 count=99
	./$< 2>&1 | tee $<.out
	od -c simulated_device > $<.dump

.SECONDEXPANSION:

testp6 test1: $$@.o $(OBJ)
	cc -o $@ $^
