#include "ShiftRegister.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование: %s <файл_настроек>\n", argv[0]);
        return 0;
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1])) return -1;
    struct MinimizedShiftRegister minimized;
    if (minimizeShiftRegister(&minimized, &reg))    {
        freeShiftRegister(&reg);
        return -2;
    }
    printMinimizedShiftRegister(&minimized);
    freeShiftRegister(&reg);
    freeMinimizedShiftRegister(&minimized);
    return 0;
}