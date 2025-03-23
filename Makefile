# Компилятор
CC = gcc

# Флаги компиляции по умолчанию
CFLAGS = -Icommon -Wall -O2 -lm

# Папки с исходными файлами
COMMON_DIR = common
SR_DIR = Регистр_сдвига

# Исходные файлы из common
COMMON_SRCS = $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

SR_SRCS = $(SR_DIR)/ShiftRegister.c $(SR_DIR)/Memory.c

# Исходные файлы для каждой задачи
SR_TASK1_SRCS = $(SR_DIR)/task1.c $(SR_SRCS)
SR_TASK2_SRCS = $(SR_DIR)/task2.c $(SR_SRCS)
SR_TASK3_SRCS = $(SR_DIR)/task3.c $(SR_SRCS)
SR_TASK4_SRCS = $(SR_DIR)/task4.c $(SR_SRCS)

TARGETS = shift_register_task1.exe shift_register_task2.exe shift_register_task3.exe shift_register_task4.exe

# Правило для сборки всех задач
all: $(TARGETS)

# Правило для сборки с отладочными флагами
debug: CFLAGS += -fsanitize=address -g
debug: clean all

shift_register_task1.exe: $(SR_TASK1_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

shift_register_task2.exe: $(SR_TASK2_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

shift_register_task3.exe: $(SR_TASK3_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

shift_register_task4.exe: $(SR_TASK4_SRCS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Правило для компиляции объектных файлов из common
$(COMMON_OBJS): $(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(TARGETS) $(COMMON_OBJS)