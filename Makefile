CC=gcc
SRC=${wildcard *.c}
OBJ=${SRC:.c=.o}
FILENAME=nano

all: build clean

build: ${OBJ}
	${CC} ${OBJ} -o ${FILENAME}

clean:
	rm -rf ${OBJ}
	rm -rf ${FILENAME}

%.o:%.c
	${CC} -c $< -o $@