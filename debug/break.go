// file to test break/continue

package main

func main() {
	n := 10
	for i := 0; i < n; i++ {
		if i > 5 {
			break
		}
		putchar('0' + i)
		putchar('\n')
	}
	putchar('\n')
	for i := 0; i < n; i++ {
		if i%2 == 0 {
			continue
		}
		putchar('0' + i)
		putchar('\n')
	}
}
