#include <stdio.h>

#define DOUBLE(x) ((x) * 2)

/* Simple helper function */
int compute_value(int a, int b) {
    int result = a + b;
    return DOUBLE(result);
}

int main(void) {
    int x = 3;
    int y = 4;

    int value = compute_value(x, y);
    printf("Computed value: %d\n", value);

    return 0;
}
