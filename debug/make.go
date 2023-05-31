package main

func main() {
	arr := make([][]int, 2)
	// arr[0] = make([]int, 2)
	arr[1] = make([]int, 2)
	arr[1][1] = 'A'
	putchar(arr[1][1])
}
