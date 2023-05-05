package main

import "fmt"

func getchar() int {
	var n byte
	fmt.Scanf("%c", &n)
	return int(n)
}

func getint() int {
	var n int
	fmt.Scanf("%d", &n)
	return n
}

func putchar(n int) {
	fmt.Printf("%c", n)
}

func putint(n int) {
	fmt.Printf("%d", n)
}

func println(n int) {
	fmt.Printf("%d\n", n)
}
