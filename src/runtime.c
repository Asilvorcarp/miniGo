#include <stdio.h>
#include <stdlib.h>

int runtime_getchar() {
    return getchar();
}

int runtime_putchar(int x) {
    return putchar(x);
}

int runtime_getint() {
    int x;
    scanf("%d", &x);
    return x;
}

int runtime_putint(int x) {
	return printf("%d", x);
}

int runtime_println(int x) {
    return printf("%d\n", x);
}

int runtime_exit(int x) {
	exit(x);
	return 0;
}
