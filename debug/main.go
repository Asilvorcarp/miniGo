package main

func fib(n int) int {
	if n >= 2 {
		return fib(n-1) + fib(n-2)
	}
	return 1
}

func main() {
	for i := 0; i < 20; i = i + 1 {
		if n := fib(i); n <= 100 {
			println(n)
		}
	}
}
