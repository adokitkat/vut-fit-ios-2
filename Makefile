NAME=proj2
FLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -lrt -lpthread -g

.PHONY: all build run

all: build

build:
	gcc $(NAME).c $(FLAGS) -o $(NAME)
	chmod +x $(NAME)

run:
	./$(NAME)