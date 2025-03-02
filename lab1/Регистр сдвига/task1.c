#include "ShiftRegister.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование: %s <файл_настроек>\n", argv[0]);
        return 0;
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1])) return -1;
    int rc = 0;
    if (readState(&reg)) {
        rc = -2;
        goto end;
    }
    int8_t x;
    printf("Введите x: ");
    while(1) {
        switch (fgetc(stdin)) {
            case '1':
                printf("y = %u\n", useShiftRegister(&reg, 1));
                printf("Введите x: ");
                break;
            case '0':
                printf("y = %u\n", useShiftRegister(&reg, 0));
                printf("Введите x: ");
                break;
            case ' ':
            case '\t':
            case '\n':
                break;
            default:
                goto end;
        }
    }
end:
    freeShiftRegister(&reg);
    return rc;
}