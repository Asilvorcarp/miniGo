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

func inArray(n int, arr []int, len int) int {
	for i := 0; i < len; i++ {
		if arr[i] == n {
			return 1
		}
	}
	return 0
}

// get string before any stop char
func getString(maxLen int, stopChars []int) []int {
	s := make([]int, maxLen)
	i := 0
	c := getchar()
	for inArray(c, stopChars) == 0 && i < maxLen {
		s[i] = c
		i++
		c = getchar()
	}
	s[i] = 0
	return s
}

// fmt.Printf("%d", n)
func putInt(n int) {
	if n < 0 {
		putchar('-')
		// n = -n
	}
	if n/10 != 0 {
		putInt(n / 10)
	}
	putchar(n%10 + '0')
}

// put int with width
func putIntW(n int, width int) {
	// print spaces
	digits := 0
	goDown := n
	goUp := 1
	for goDown != 0 {
		digits++
		goDown /= 10
		goUp *= 10
	}
	spaces := 10 - digits
	for i := 0; i < spaces; i++ {
		putchar(' ')
	}
	// print sign
	if n < 0 {
		putchar('-')
		n = -n
	}
	// print number
	goDown = goUp / 10
	for i := digits - 1; i >= 0; i-- {
		d := n / goDown % 10
		putchar(d + '0')
		goDown /= 10
	}
}

// fmt.Printf("%s", s)
func putString(s []int) {
	i := 0
	for s[i] != 0 {
		putchar(s[i])
		i++
	}
}

func main() {
	aRows, aCols := getInt(), getInt()
	matrixA := parseMatrix(aRows, aCols)

	bRows, bCols := getInt(), getInt()
	matrixB := parseMatrix(bRows, bCols)

	result := calculateProduct(matrixA, matrixB, aRows, aCols, bRows, bCols)

	outputMatrix(result)
}

func parseMatrix(rows int, cols int) [][]int {
	matrix := make([][]int, rows)
	for i := 0; i < rows; i++ {
		matrix[i] = make([]int, cols)
		for j := 0; j < cols; j++ {
			matrix[i][j] = getInt()
		}
	}
	return matrix
}

// "Incompatible Dimensions\n" as int array
var err = []int{73, 110, 99, 111, 109, 112, 97, 116, 105, 98, 108, 101, 32, 68, 105, 109, 101, 110, 115, 105, 111, 110, 115, 10, 0}

func calculateProduct(matrixA [][]int, matrixB [][]int, ar int, ac int, br int, bc int) [][]int {
	// error if dim not match a.k.a. ac != br
	if ac != br {
		putString(err)
		return
	}
	result := make([][]int, ar)
	for i := 0; i < ar; i++ {
		row := make([]int, bc)
		for j := 0; j < bc; j++ {
			sum := 0
			for k := 0; k < ac; k++ {
				sum += matrixA[i][k] * matrixB[k][j]
			}
			row[j] = sum
		}
		result[i] = row
	}
	return result
}

func outputMatrix(matrix [][]int, rows int, cols int) {
	for i := 0; i < rows; i++ {
		for j := 0; j < cols; j++ {
			putIntW(matrix[i][j], 10)
		}
		putchar('\n')
	}
}
