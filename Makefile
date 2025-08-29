CC     = gcc
CFLAGS = -Iinclude

RGX    = src/debug.c src/regex.c src/nfa.c src/dfa.c src/allocator.c src/stack.c 
OBJ    = $(patsubst src/%.c, obj/%.o, $(RGX))
LOBJ = $(patsubst src/%.c, obj/%.l.o, $(RGX))
TST    = test/nfa.c test/dfa.c test/bits.c test/tokens-nfa.c
RUN    = $(patsubst test/%.c, obj/%.tst, $(TST))
LTST   = test/min-dfa.c test/hopcroft.c test/stack.c
LRUN   = $(patsubst test/%.c, obj/%.ltst, $(LTST))

$(RUN) $(OBJ) $(LOBJ): | obj
$(LOBJ) : CFLAGS += -DRGXLRG
obj:
	mkdir -p obj

obj/allocator.o: src/allocator.c include/allocator.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/nfa.o: src/nfa.c include/nfa.h include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.l.o: src/%.c include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/allocator.l.o: src/allocator.c include/allocator.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/nfa.l.o: src/nfa.c include/nfa.h include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/rgx.a: $(OBJ) Makefile
	ar -cr $@ $(OBJ)

obj/lrgx.a: $(LOBJ) Makefile
	ar -cr $@ $(LOBJ)

obj/%.s: test/%.c obj/rgx.a
	$(CC) $(CFLAGS) -o obj/$* $^

obj/%.ls: test/%.c obj/lrgx.a
	$(CC) $(CFLAGS) -o obj/$* $^

obj/%.tst: obj/%.s
	cd obj/ && ./$*

obj/%.ltst: obj/%.ls
	cd obj/ && ./$*

clean:
	rm -f ./obj/*

all: obj/rgx.a obj/lrgx.a
	$(MAKE) obj/dfa.tst
	$(MAKE) obj/nfa.tst
	$(MAKE) obj/bits.tst
	$(MAKE) obj/min-dfa.ltst
	$(MAKE) obj/hopcroft.ltst
