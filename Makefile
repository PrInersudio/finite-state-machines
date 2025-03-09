# Компилятор
CC = gcc

# Флаги компиляции по умолчанию
CFLAGS = -Icommon -Wall -O2 -lm

# Папки с исходными файлами
COMMON_DIR = common
LAB1_SR_DIR = lab1/Регистр_сдвига

# Исходные файлы из common
COMMON_SRCS = $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# Исходные файлы для каждой задачи
LAB1_SR_TASK1_SRCS = $(LAB1_SR_DIR)/task1.c $(LAB1_SR_DIR)/ShiftRegister.c
LAB1_SR_TASK2_SRCS = $(LAB1_SR_DIR)/task2.c $(LAB1_SR_DIR)/ShiftRegister.c
LAB1_SR_TASK3_SRCS = $(LAB1_SR_DIR)/task3.c $(LAB1_SR_DIR)/ShiftRegister.c

TARGETS = lab1_shift_register_task1.exe lab1_shift_register_task2.exe lab1_shift_register_task3.exe

# Правило для сборки всех задач
all: $(TARGETS)

# Правило для сборки с отладочными флагами
debug: CFLAGS += -fsanitize=address -g
debug: clean all

lab1_shift_register_task1.exe: $(LAB1_SR_TASK1_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Правило для сборки task2
lab1_shift_register_task2.exe: $(LAB1_SR_TASK2_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Правило для сборки task3
lab1_shift_register_task3.exe: $(LAB1_SR_TASK3_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Правило для компиляции объектных файлов из common
$(COMMON_OBJS): $(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(TARGETS) $(COMMON_OBJS)