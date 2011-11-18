DEPS = babyfs.h p6.h
OBJ = block.o cache.o p6.o testp6.o

%.o: %.c $(DEPS)
	cc -c -o $@ $<

testp6: $(OBJ)
	cc -o $@ $^
