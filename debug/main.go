package main

func main() {
	if 1 < 2 {
		putchar('a')
	} else {
		putchar('b')
	}
	if 1 > 2 {
		putchar('c')
	} else {
		putchar('d')
	}
	if 1 == 1 {
		putchar('e')
	} else if 1 == 1 {
		putchar('f')
	} else {
		putchar('g')
	}
	if 1 != 1 {
		putchar('h')
	} else if 1 == 1 {
		putchar('i')
	} else {
		putchar('j')
	}
	if 1 != 1 {
		putchar('k')
	} else if 1 != 1 {
		putchar('l')
	} else {
		putchar('m')
	}
	if 1 == 1 {
		putchar('n')
	} else if 1 != 1 {
		putchar('o')
	} else {
		putchar('p')
	}
}
