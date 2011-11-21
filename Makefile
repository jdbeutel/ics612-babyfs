DEPS = babyfs.h p6.h cache.h
OBJ = block.o cache.o p6.o tree.o

%.o: %.c $(DEPS)
	cc -c -o $@ $<

runtest%: test%
	rm -f simulated_device $<.od-X $<.od-c
	dd if=/dev/zero of=simulated_device bs=1024 count=99
	./$< 2>&1 | tee $<.out
	od -Ax -c simulated_device > $<.od-c
	od -Ax -X simulated_device > $<.od-X
	git diff $<.od-X

.SECONDEXPANSION:

testp6 test1: $$@.o $(OBJ)
	cc -o $@ $^
