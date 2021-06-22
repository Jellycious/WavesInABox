CPP = g++
SRC_DIR = ./src
TARGET_DIR = ./build

CFLAGS = -I./include -I/usr/local/include -Wall -Wextra -Wno-missing-braces -g
LDFLAGS = -L/usr/local/lib -lglfw3 -lrt -lm -ldl -lX11 -lpthread -lxcb -lXau -lXdmcp

OBJS = glad.o util.o
OBJS_TARGETS = $(addprefix ${TARGET_DIR}/,${OBJS})


all: target ${OBJS}
	${CPP} ${CFLAGS} ${OBJS_TARGETS} ${SRC_DIR}/main.cpp -o ./main ${LDFLAGS}

%.o: ${SRC_DIR}/%.cpp target
	${CPP} ${CFLAGS} -c $< -o ${TARGET_DIR}/$@

target:
	mkdir -p ${TARGET_DIR}

glad.o: 
	${CPP} -c ${CFLAGS} ${SRC_DIR}/glad.c -o ${TARGET_DIR}/glad.o  ${LDFLAGS}

clean:
	rm -rf ${TARGET_DIR}
