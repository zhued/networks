EXE=dfc dfs

all: $(EXE)

CLEAN=rm -f $(EXE) *.o *.a

dfs: dfs.c
	gcc -Wall -g -o dfs dfs.c

dfc: dfc.c
	gcc -Wall -g -o dfc dfc.c

clean:
	$(CLEAN)