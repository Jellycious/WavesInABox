CPP = g++
SRC_DIR = ./src
TARGET_DIR = ./build

CFLAGS = -I./include -I/usr/local/include -Wall -Wextra -Wno-missing-braces -g
LDFLAGS = -L/usr/local/lib -lglfw -lrt -lm -ldl -lX11 -lpthread -lxcb -lXau -lXdmcp

SOURCES = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI    _DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
IMGUI_DIR = ./src/imgui
IMGUI_OBJS = imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o
IMGUI_TARGETS = $(addprefix ${TARGET_DIR}/imgui/, $(IMGUI_OBJS))

OBJS = glad.o util.o
OBJS_TARGETS = $(addprefix ${TARGET_DIR}/,${OBJS})


all: target ${OBJS} imgui
	${CPP} ${CFLAGS} ${OBJS_TARGETS} $(IMGUI_TARGETS) ${SRC_DIR}/main.cpp -o ./main ${LDFLAGS}

imgui:$(IMGUI_TARGETS)

%.o: ${SRC_DIR}/%.cpp target
	${CPP} ${CFLAGS} -c $< -o ${TARGET_DIR}/$@

$(TARGET_DIR)/imgui/%.o:
	$(CPP) $(CFLAGS) -c -o $@ $(SRC_DIR)/imgui/$(basename $(notdir $@)).cpp

target:
	mkdir -p ${TARGET_DIR}
	mkdir -p ${TARGET_DIR}/imgui

glad.o: 
	${CPP} -c ${CFLAGS} ${SRC_DIR}/glad.c -o ${TARGET_DIR}/glad.o  ${LDFLAGS}

clean:
	rm -rf ${TARGET_DIR}
