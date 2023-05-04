#include <stdio.h>
#include <stdlib.h>

int ugo_builtin_getchar() {
    return getchar();
}

int ugo_builtin_putint(int x) {
	return printf("%d", x);
}

int ugo_builtin_println(int x) {
    return printf("%d\n", x);
}

int ugo_builtin_putchar(int x) {
    return putchar(x);
}

int ugo_builtin_exit(int x) {
	exit(x);
	return 0;
}
