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

// // cat debug/matrix.temp.in | go run debug/matrix.go

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

// whether char c is in string s
func inString(c int, s []int) int {
	cur := s[0]
	for cur != 0 {
		if cur == c {
			return 1
		}
	}
	return 0
}

// get string before any char in the stop string
func getString(maxLen int, stop []int) []int {
	s := make([]int, maxLen)
	i := 0
	c := getchar()
	for inString(c, stop) == 0 && i < maxLen {
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
		n = -n
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
	spaces := width - digits
	if n < 0 {
		spaces--
	}
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

	// "Incompatible Dimensions\n" as char literal array
	err := []int{'I', 'n', 'c', 'o', 'm', 'p', 'a', 't', 'i', 'b', 'l', 'e', ' ', 'D', 'i', 'm', 'e', 'n', 's', 'i', 'o', 'n', 's', '\n', 0}

	// error if dim not match
	if aCols != bRows {
		putString(err)
		return
	}

	result := calculateProduct(matrixA, matrixB, aRows, aCols, bRows, bCols)

	outputMatrix(result, aRows, bCols)
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

func calculateProduct(matrixA [][]int, matrixB [][]int, ar int, ac int, br int, bc int) [][]int {
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
