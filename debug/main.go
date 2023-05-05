package main

// import (
// 	"fmt"
// )

// func getchar() int {
// 	var n byte
// 	fmt.Scanf("%c", &n)
// 	return int(n)
// }

// func putchar(n int) {
// 	fmt.Printf("%c", n)
// }

// fmt.Scanf("%d", &n)
func getInt() int {
	sign := 1
	n := 0
	c := getchar()
	for c < '0' || c > '9' {
		if c == '-' {
			sign = -1
		}
		c = getchar()
	}
	for c >= '0' && c <= '9' {
		n = n*10 + c - '0'
		c = getchar()
	}
	n *= sign
	return n
}

// func inArray(n int, arr []int) int {
// 	for i := 0; i < len(arr); i++ {
// 		if arr[i] == n {
// 			return 1
// 		}
// 	}
// 	return 0
// }

// // get string before any stop char
// func getString(maxLen int, stopChars []int) []int {
// 	s := make([]int, maxLen)
// 	i := 0
// 	c := getchar()
// 	for inArray(c, stopChars) == 0 && i < maxLen {
// 		s[i] = c
// 		i++
// 		c = getchar()
// 	}
// 	s[i] = 0
// 	return s
// }

// // fmt.Printf("%d", n)
// func putInt(n int) {
// 	if n < 0 {
// 		putchar('-')
// 		n = -n
// 	}
// 	if n/10 != 0 {
// 		putInt(n / 10)
// 	}
// 	putchar(n%10 + '0')
// }

// // put int with width
// func putIntW(n int, width int) {
// 	// print spaces
// 	digits := 0
// 	goDown := n
// 	goUp := 1
// 	for goDown != 0 {
// 		digits++
// 		goDown /= 10
// 		goUp *= 10
// 	}
// 	spaces := 10 - digits
// 	for i := 0; i < spaces; i++ {
// 		putchar(' ')
// 	}
// 	// print sign
// 	if n < 0 {
// 		putchar('-')
// 		n = -n
// 	}
// 	// print number
// 	goDown = goUp / 10
// 	for i := digits - 1; i >= 0; i-- {
// 		d := n / goDown % 10
// 		putchar(d + '0')
// 		goDown /= 10
// 	}
// }

// // fmt.Printf("%s", s)
// func putString(s []int) {
// 	i := 0
// 	for s[i] != 0 {
// 		putchar(s[i])
// 		i++
// 	}
// }

// // "Incompatible Dimensions" as int array
// var err = []int{73, 110, 99, 111, 109, 112, 97, 116, 105, 98, 108, 101, 32, 68, 105, 109, 101, 110, 115, 105, 111, 110, 115, 0}

// func main() {
// 	aRows, aCols := getInt(), getInt()
// 	matrixA := parseMatrix(aRows, aCols)

// 	bRows, bCols := getInt(), getInt()
// 	matrixB := parseMatrix(bRows, bCols)

// 	// error if dim not match
// 	if aCols != bRows {
// 		putString(err)
// 		putchar('\n')
// 		return
// 	}
// 	result := calculateProduct(matrixA, matrixB)

// 	outputMatrix(result)
// }

// func parseMatrix(rows int, cols int) [][]int {
// 	matrix := make([][]int, rows)
// 	for i := 0; i < rows; i++ {
// 		matrix[i] = make([]int, cols)
// 		for j := 0; j < cols; j++ {
// 			matrix[i][j] = getInt()
// 		}
// 	}
// 	return matrix
// }

// func calculateProduct(matrixA [][]int, matrixB [][]int) [][]int {
// 	m := len(matrixA)
// 	n := len(matrixB[0])
// 	result := make([][]int, m)
// 	for i := 0; i < m; i++ {
// 		row := make([]int, n)
// 		for j := 0; j < n; j++ {
// 			sum := 0
// 			for k := 0; k < len(matrixA[i]); k++ {
// 				sum += matrixA[i][k] * matrixB[k][j]
// 			}
// 			row[j] = sum
// 		}
// 		result[i] = row
// 	}
// 	return result
// }

// func outputMatrix(matrix [][]int) {
// 	for _, row := range matrix {
// 		for _, value := range row {
// 			putIntW(value, 10)
// 		}
// 		putchar('\n')
// 	}
// }
