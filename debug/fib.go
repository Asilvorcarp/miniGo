package main

func putInt(n int) {
	if n < 0 {
		putchar('-')
		n = -n
	}
	if n/10 != 0 {
		putInt(n / 10)
	}
	putchar(n%10 + '0')
}

func fib(n int) int {
	if n >= 2 {
		return fib(n-1) + fib(n-2)
	}
	return 1
}

func main() {
	for i := 0; i < 20; i = i + 1 {
		if n := fib(i); n <= 100 {
			putInt(n)
			putchar('\n')
		}
	}
}
