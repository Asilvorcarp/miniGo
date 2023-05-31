package main

func put8args(a int, b int, c int, d int, e int, f int, g int, h int) {
	putchar(a)
	putchar(b)
	putchar(c)
	putchar(d)
	putchar(e)
	putchar(f)
	putchar(g)
	putchar(h)
}

func main() {
	put8args('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h')
}
