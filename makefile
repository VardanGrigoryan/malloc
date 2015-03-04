malloc : malloc.c
	gcc -o $@ $< -ggdb

.PHONY : clean
clean:
	rm -rf malloc *.swp
