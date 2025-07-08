BUILD_FLAGS=-std=c17 -O3 -Wall -Wextra -Wno-unused-result

all: ./build/snake
	strip ./build/snake

./build/snake: ./build/snake.o
	cc ./build/snake.o -o ./build/snake

./build/snake.o: ./snake.c ./tgui.h
	cc -c ./snake.c -o ./build/snake.o -D _DEFAULT_SOURCE $(BUILD_FLAGS)

clean:
	rm ./build/snake ./build/snake.o