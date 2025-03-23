#include "ShiftRegister.h"
#include <inttypes.h>

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
    printf("Введите x: ");
    while(1) {
        switch (fgetc(stdin)) {
            case '1':
                printf("y = %" PRIu8 " ", useShiftRegister(&reg, 1));
                printf("state = %" PRIu32 "\n", getState(&reg));
                printf("Введите x: ");
                break;
            case '0':
                printf("y = %" PRIu8 " ", useShiftRegister(&reg, 0));
                printf("state = %" PRIu32 "\n", getState(&reg));
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