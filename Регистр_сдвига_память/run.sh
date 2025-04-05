#!/bin/bash

# Параметры
DEBUG_MODE=false
LOG_FILE="redis_commands.log"

# Парсинг аргументов
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug) DEBUG_MODE=true; shift ;;
        *) break ;;
    esac
done

# Инициализация лога
if $DEBUG_MODE; then
    > "$LOG_FILE"
    echo "🔍 Режим отладки: логирование команд в $LOG_FILE"
fi

# Запуск Redis с мониторингом (если debug)
start_redis() {
    docker-compose up --build -d redis
    
    if $DEBUG_MODE; then
        docker-compose exec -T redis redis-cli MONITOR > "$LOG_FILE" &
        MONITOR_PID=$!
    fi
}

start_redis

# Ожидание готовности Redis
timeout=30
while ! docker-compose exec -T redis redis-cli ping | grep -q "PONG"; do
    if (( timeout-- <= 0 )); then
        echo "⛔ Redis не запустился за 30 секунд" >&2
        docker-compose logs redis >&2
        exit 1
    fi
    sleep 1
done

# Всегда очищаем БД перед запуском программы
echo "🧹 Очищаем БД перед запуском..."
if ! docker-compose exec -T redis redis-cli FLUSHALL >/dev/null; then
    echo "⛔ Ошибка очистки БД" >&2
    exit 1
fi

# Подготавливаем аргументы для программы
PROGRAM_ARGS=("$@")
if $DEBUG_MODE; then
    PROGRAM_ARGS+=("--no-clean")  # Добавляем флаг, если debug
    echo "⚙️  Передаем программе параметры: ${PROGRAM_ARGS[*]}"
fi

# Функция для создания отладочного дампа
create_debug_dump() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local prefix="redis_dump_${timestamp}"
    
    echo "💾 Создаем отладочный дамп..."
    docker-compose exec -T redis redis-cli SAVE
    docker cp $(docker-compose ps -q redis):/data/dump.rdb "${prefix}.rdb"

    rdb -c json -o "${prefix}.json" "${prefix}.rdb" && \
    python3 analyze_dump.py "${prefix}.json" && \
    rm -f "${prefix}.json" "${prefix}.rdb"
}

# Очистка
cleanup() {

    if [[ $CLEANUP_DONE -eq 1 ]]; then
        return
    fi
    CLEANUP_DONE=1

    # Посылаем SIGINT в run.exe (если он еще работает)
    if kill -0 $RUN_PID 2>/dev/null; then
        echo "🛑 Остановка run.exe..."
        
        # Сначала мягкий SIGINT
        kill -SIGINT $RUN_PID 2>/dev/null
        
        # Ждём нормального завершения (таймаут 3 секунды)
        for i in {1..30}; do
            kill -0 $RUN_PID 2>/dev/null || break
            sleep 0.1
        done
        
        # Если процесс ещё жив, принудительно завершаем
        if kill -0 $RUN_PID 2>/dev/null; then
            echo "⚠️ Принудительное завершение run.exe..."
            kill -9 $RUN_PID 2>/dev/null
            wait $RUN_PID 2>/dev/null
        fi
    fi

    if $DEBUG_MODE; then
        kill $MONITOR_PID 2>/dev/null
        create_debug_dump
        echo "📌 Лог команд сохранён в $LOG_FILE"
    fi

    echo "🧽 Выполняем финальную очистку..."
    docker-compose down -v
}

# Устанавливаем обработчики сигналов
trap 'cleanup; exit 130' SIGINT  # 130 = Script terminated by Ctrl+C
trap 'cleanup; exit' EXIT

# Запуск программы
echo "🔧 Запуск программы..."
./run.exe "${PROGRAM_ARGS[@]}" &
RUN_PID=$!

# Ждём завершения с проверкой статуса
if wait $RUN_PID; then
    echo "✅ Программа завершилась успешно"
else
    EXIT_CODE=$?
    echo "❌ Программа завершилась с кодом $EXIT_CODE" >&2
    exit $EXIT_CODE
fi