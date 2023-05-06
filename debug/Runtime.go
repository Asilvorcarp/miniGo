package main

import "fmt"

func getchar() int {
	var n byte
	fmt.Scanf("%c", &n)
	return int(n)
}

func putchar(n int) {
	fmt.Printf("%c", n)
}
