package main

// opaque ptr
func main() {
	a := make([][]int, 10)
	b := make([]int, 10)
	b[0] = 'a'
	a = b
	putchar(a[0])
}
