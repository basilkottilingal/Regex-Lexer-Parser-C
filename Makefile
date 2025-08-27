CC     = gcc
CFLAGS = -Iinclude

RGX    = src/debug.c src/regex.c src/nfa.c src/dfa.c src/allocator.c src/stack.c 
OBJ    = $(patsubst src/%.c, obj/%.o, $(RGX))
TST    = test/nfa.c test/dfa.c test/bits.c
RUN    = $(patsubst test/%.c, obj/%.tst, $(TST))
LRGTST = test/min-dfa.c test/hopcroft.c
LRGRUN = $(patsubst test/%.c, obj/%.tst, $(LRGTST))

$(OBJ): | obj
$(RUN): | obj
$(LRGRUN): | obj

obj:
	mkdir -p obj

obj/allocator.o: src/allocator.c include/allocator.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/nfa.o: src/nfa.c include/nfa.h include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/rgx.a: $(OBJ) Makefile
	ar -cr $@ $(OBJ)

obj/%.s: test/%.c obj/rgx.a
	$(CC) -Iinclude -o obj/$* $^

obj/%.tst: obj/%.s obj/rgx.a
	cd obj/ && ./$*

clean:
	rm -f ./obj/*

all:
	$(MAKE) clean
	$(MAKE) CFLAGS="-DRGXLRG $(CFLAGS)" obj/rgx.a
	$(MAKE) obj/min-dfa.tst
	$(MAKE) obj/hopcroft.tst
	$(MAKE) clean
	$(MAKE) obj/rgx.a
	$(MAKE) obj/dfa.tst
	$(MAKE) obj/nfa.tst
	$(MAKE) obj/bits.tst
