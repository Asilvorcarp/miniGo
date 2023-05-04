package main

import "fmt"

func getchar() int {
	var n rune
	fmt.Scanln(&n)
	return int(n)
}

func getint() int {
	var n int
	fmt.Scanln(&n)
	return n
}

func putchar(n int) {
	fmt.Println(rune(n))
}

func putint(n int) {
	fmt.Println(n)
}

func println(n int) {
	fmt.Println(n)
}
