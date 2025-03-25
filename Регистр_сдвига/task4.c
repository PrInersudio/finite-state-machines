#include <stdio.h>
#include <inttypes.h>
#include "ShiftRegister.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование: %s <файл_настроек>\n", argv[0]);
        return 0;
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1])) return -1;
    uint64_t memory_size = 0;
    int rc = getMemoryShiftRegister(&reg, &memory_size);
    if (rc == 0)
        printf("Память автомата больше %" PRIu64 ".\n", MAX_MEMORY_TRY);
    else if (rc == 1)
        printf("Память автомата равна %" PRIu64 "\n", memory_size);
    freeShiftRegister(&reg);
    return rc >=0 ? 0 : -2;
}