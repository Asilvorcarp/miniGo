// basic functions
// built on top of getchar and putchar

package main

import (
	"fmt"
)

func getchar() int {
	var n byte
	fmt.Scanf("%c", &n)
	return int(n)
}

func putchar(n int) {
	fmt.Printf("%c", n)
}

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

func getIntFromStrStart(s []int) int {
	sign := 1
	n := 0
	i := 0
	c := s[i]
	for c < '0' || c > '9' {
		if c == '-' {
			sign = -1
		}
		i++
		c = s[i]
	}
	for c >= '0' && c <= '9' {
		n = n*10 + c - '0'
		i++
		c = s[i]
	}
	n *= sign
	return n
}

// get int from string, start from index x
// return [num, end], end is the index of the last digit + 1
// return [-1, -1] if not found
func getIntFromString(s []int, x int) []int {
	ret := []int{-1, -1}
	total := length(s)
	found := 0
	sign := 1
	n := 0
	i := x
	c := s[i]
	for c < '0' || c > '9' {
		if c == '-' {
			sign = -1
		}
		i++
		if i >= total {
			return ret
		}
		// fmt.Println("going to ", i)
		c = s[i]
	}
	for c >= '0' && c <= '9' {
		found = 1
		n = n*10 + c - '0'
		i++
		c = s[i]
	}
	n *= sign
	if found == 0 {
		return ret
	}
	ret[0] = n
	ret[1] = i
	// fmt.Println("returning ", ret)
	return ret
}

// whether char c is in string s
func inString(c int, s []int) int {
	idx := 0
	cur := s[idx]
	for cur != 0 {
		if cur == c {
			return 1
		}
		idx++
		cur = s[idx]
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

func putIntC(n int, c int) {
	putInt(n)
	putchar(c)
}

func putIntCC(n int, c1 int, c2 int) {
	putInt(n)
	putchar(c1)
	putchar(c2)
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

// get string until '\n'
func getLine(maxLen int) []int {
	stop := []int{'\n', 0}
	return getString(maxLen, stop)
}

// find char in string, start from x; return -1 if not found
func findChar(s []int, c int, x int) int {
	i := x
	for s[i] != 0 {
		if s[i] == c {
			return i
		}
		i++
	}
	return -1
}

// get length of string
func length(s []int) int {
	i := 0
	for s[i] != 0 {
		i++
	}
	return i
}

// substring
func substring(s []int, start int, end int) []int {
	sub := make([]int, end-start+1)
	for i := start; i < end; i++ {
		sub[i-start] = s[i]
	}
	sub[end-start] = 0
	return sub
}

// end line
func endl() {
	putchar('\n')
}

// put space n times
func putSpace(n int) {
	for i := 0; i < n; i++ {
		putchar(' ')
	}
}

func putIndent(indent int) {
	for i := 0; i < indent; i++ {
		putSpace(4)
	}
}

// for debug info output
func putIndentCC(ind int, x int, c1 int, c2 int) {
	putIndent(ind)
	if c1 == '}' {
		putchar(c1)
		putInt(x)
		putchar(c2)
		return
	}
	putIntCC(x, c1, c2)
}
