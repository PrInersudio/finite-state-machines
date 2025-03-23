#include <stdio.h>
#include "ShiftRegister.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование: %s <файл_настроек>\n", argv[0]);
        return 0;
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1])) return -1;
    struct Memory memory;
    if (getMemoryShiftRegister(&memory, &reg)) {
        freeShiftRegister(&reg);
        return -1;
    }
    printMemory(&memory);
    freeMemory(&memory);
    freeShiftRegister(&reg);
    return 0;
}