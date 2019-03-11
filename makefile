main: main.c agent.c category.c name.c network.c
	gcc -o simulation main.c agent.c category.c name.c network.c -lm -O3 -std=c99 -Wall -pedantic -Wextra

debug: main.c agent.c category.c name.c network.c
	gcc main.c agent.c category.c name.c network.c -lm -Wall -g
