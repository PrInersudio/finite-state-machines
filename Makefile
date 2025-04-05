# Компиляторы
CC = gcc
CXX = g++

# Флаги компиляции по умолчанию
CFLAGS = -Icommon -Wall -O3 -lm
CXXFLAGS = -Icommon -Wall -O3

# Папки с исходными файлами
COMMON_DIR = common
SR_DIR = Регистр_сдвига

# Исходные файлы из common
COMMON_SRCS_C = $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS_C = $(COMMON_SRCS_C:.c=.o)
COMMON_SRCS_CPP = $(wildcard $(COMMON_DIR)/*.cpp)
COMMON_OBJS_CPP = $(COMMON_SRCS_CPP:.cpp=.o)

SR_SRCS = $(SR_DIR)/ShiftRegister.c
SR_OBJ = $(SR_SRCS:.c=.o)

# Исходные файлы для каждой задачи
SR_TASK1_SRC = $(SR_DIR)/task1.c
SR_TASK2_SRC = $(SR_DIR)/task2.c
SR_TASK3_SRC = $(SR_DIR)/task3.c
SR_TASK4_SRC = $(SR_DIR)/*.cpp

TARGETS = shift_register_task1.exe shift_register_task2.exe shift_register_task3.exe shift_register_task4.exe

# Правило для сборки всех задач
all: clean $(TARGETS)

# Правило для сборки с отладочными флагами
debug: CFLAGS += -fsanitize=address -g
debug: CXXFLAGS += -fsanitize=address -g -DDEBUG
debug: clean all

shift_register_task1.exe: $(SR_TASK1_SRC) $(COMMON_OBJS_C) $(SR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

shift_register_task2.exe: $(SR_TASK2_SRC) $(COMMON_OBJS_C) $(SR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

shift_register_task3.exe: $(SR_TASK3_SRC) $(COMMON_OBJS_C) $(SR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

shift_register_task4.exe: $(SR_TASK4_SRC) $(COMMON_OBJS_CPP) $(COMMON_OBJS_C) $(SR_OBJ)
	$(CXX) $(CXXFLAGS) -lsqlite3 -o $@ $^

# Правила для компиляции объектных файлов из common
$(COMMON_OBJS_C): $(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(COMMON_OBJS_CPP): $(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SR_OBJ): $(SR_SRCS)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(TARGETS) $(COMMON_OBJS_C) $(COMMON_OBJS_CPP) $(SR_OBJ) *.db* *.cvc
	find . -type f -name "*.log" -delete