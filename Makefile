CC     = gcc
CFLAGS = -Iinclude -Wall
C99    = -std=c99 -pedantic

RGX    = src/debug.c src/regex.c src/nfa.c src/dfa.c src/allocator.c \
         src/stack.c src/bits.c src/error.c src/class.c              \
         src/compression.c
LXR    = src/lexer.c src/lex.c
OBJ    = $(patsubst src/%.c, obj/%.o, $(RGX))
LOBJ   = $(patsubst src/%.c, obj/%.o, $(LXR))
TST    = test/nfa.c test/dfa.c test/bits.c test/tokens-nfa.c         \
         test/min-dfa.c test/hopcroft.c test/stack.c test/class.c    \
         test/charclass.c test/json.c test/tbl-json.c                \
         test/quantifier.c
RUN    = $(patsubst test/%.c, obj/%.tst, $(TST))

$(RUN) $(OBJ): | obj
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

obj/lexer.o: src/lexer.c include/regex.h include/lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/lxr.a: $(OBJ) $(LOBJ) Makefile
	ar -cr $@ $(OBJ) $(LOBJ)

obj/%.s: test/%.c obj/rgx.a
	$(CC) $(CFLAGS) -o obj/$* $^

obj/%.ls: test/%.c obj/lxr.a
	$(CC) $(CFLAGS) -o obj/$* $^

obj/%.tst: obj/%.s
	cd obj/ && ./$*

obj/%.ltst: obj/%.ls
	cd obj/ && ./$*

clean:
	rm -f ./obj/*

all: obj/rgx.a
	$(MAKE) obj/dfa.tst
	$(MAKE) obj/nfa.tst
	$(MAKE) obj/bits.tst
	$(MAKE) obj/class.tst
	$(MAKE) obj/charclass.tst
	$(MAKE) obj/quantifier.tst
	$(MAKE) obj/json.tst
	$(MAKE) obj/tbl-json.tst
	$(MAKE) obj/min-dfa.tst
	$(MAKE) obj/hopcroft.tst
	$(MAKE) obj/tokens-nfa.tst
	$(MAKE) obj/lxrgrammar.ltst
	$(MAKE) obj/c-tokens.ltst
	$(MAKE) obj/read_lex.ltst
