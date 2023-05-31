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

// cat debug/course_selection.temp.in | go run debug/course_selection.go

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
	// fmt.Println("putString: ", s)
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

// compare two strings
func strcmp(s1 []int, s2 []int) int {
	i := 0
	for s1[i] != 0 && s2[i] != 0 && s1[i] == s2[i] {
		i++
	}
	return s1[i] - s2[i]
}

// encode string to int, string is only made of [0-9a-zA-Z]
// max value: 62**4 = 14776336
func enc(s []int) int {
	// fmt.Println("enc", s)
	res := 0
	i := 0
	for s[i] != 0 {
		res *= 62
		if s[i] >= '0' && s[i] <= '9' {
			res += s[i] - '0'
		}
		if s[i] >= 'a' && s[i] <= 'z' {
			res += s[i] - 'a' + 10
		}
		if s[i] >= 'A' && s[i] <= 'Z' {
			res += s[i] - 'A' + 36
		}
		i++
	}
	// fmt.Println("enc res", res)
	return res
}

// decode int to string
func dec(code int) []int {
	res := make([]int, 5)
	if code == 0 {
		res[0] = '0'
		res[1] = 0
		return res
	}
	len := 0
	tmp := code
	for tmp > 0 {
		tmp /= 62
		len++
	}
	res[len] = 0
	for i := len - 1; i >= 0; i-- {
		mod := code % 62
		if mod < 10 {
			res[i] = '0' + mod
		}
		if 10 <= mod && mod < 36 {
			res[i] = 'a' + (mod - 10)
		}
		if mod >= 36 {
			res[i] = 'A' + (mod - 36)
		}
		code /= 62
	}
	return res
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

// print n/100 as float with 1 decimal and round up
func putGpa(n int) {
	// integer part
	integer := n / 100
	// one decimal
	d := n % 100 / 10
	// round up
	if n%10 >= 5 {
		d++
	}
	// carry
	if d == 10 {
		integer++
		d = 0
	}
	putInt(integer)
	putchar('.')
	putInt(d)
}

// get a conjunction from string, like "c1,a2" to [enc("c1"), enc("a2"), -1]
// get all int in string actually
func getConjunction(s []int) []int {
	ret := make([]int, 10)

	// empty string
	if s[0] == 0 {
		ret[0] = -1
		return ret
	}
	idx := 0
	total := length(s)
	start := 0
	// do (one or more)
	end := findChar(s, ',', start)
	if end == -1 {
		end = total
	}
	sub := substring(s, start, end)
	res := enc(sub)
	ret[idx] = res
	idx++
	start = end + 1
	// while
	for start != total+1 {
		end := findChar(s, ',', start)
		if end == -1 {
			end = total
		}
		sub := substring(s, start, end)
		res := enc(sub)
		ret[idx] = res
		idx++
		start = end + 1
	}
	ret[idx] = -1

	return ret
}

// last ptr is nil
func getDisjunction(s []int) [][]int {
	ret := make([][]int, 10)
	for i := 0; i < 10; i++ {
		ret[i] = nil
	}
	// empty string
	if s[0] == 0 {
		return ret
	}
	idx := 0
	total := length(s)
	start := 0
	// do (one or more)
	end := findChar(s, ';', start)
	if end == -1 {
		end = total
	}
	sub := substring(s, start, end)
	res := getConjunction(sub)
	ret[idx] = res
	idx++
	start = end + 1
	// while
	for start != total+1 {
		end := findChar(s, ';', start)
		if end == -1 {
			end = total
		}
		sub := substring(s, start, end)
		res := getConjunction(sub)
		ret[idx] = res
		idx++
		start = end + 1
	}
	// fmt.Println("getDisjunction returning ", ret)
	return ret
}

func putIndent(indent int) {
	for i := 0; i < indent; i++ {
		putSpace(4)
	}
}

// for debug info output
func putIndentCC(ind int, x int, c1 int, c2 int) {
	var debug int = 0
	if debug == 1 {
		putIndent(ind)
		if c1 == '}' {
			putchar(c1)
			putInt(x)
			putchar(c2)
			return
		}
		putIntCC(x, c1, c2)
	}
}

// type cou struct {
// 	logic [][]int
// 	cred  int
//  // NotSet, Empty, F, D, C, B, A : -2, -1, 0, 1, 2, 3, 4
// 	score int
// }

// assert all course is alphabet and digit
// 512 for line number
// 512 for line length
// 5 for course name
// 8 for conjunction/disjunction
// ! always a new blank line at the end
func main() {
	// final result
	gpa := 0
	ha := 0
	hc := 0
	cr := 0
	possible := make([]int, 512)
	possibleNum := 0

	// input
	lines := make([][]int, 512)
	lineNum := 0
	line := getLine(512)
	for line[0] != 0 {
		lines[lineNum] = line
		line = getLine(512)
		lineNum++
	}
	// { // print line number
	// 	putInt(lineNum)
	// 	endl()
	// }
	// { // print lines
	// 	for i := 0; i < lineNum; i++ {
	// 		putString(lines[i])
	// 		endl()
	// 	}
	// 	putInt(11111)
	// 	endl()
	// }
	// id of input order
	ids := make([]int, lineNum)
	// buckets
	logics := make([][][]int, 14776340)
	creds := make([]int, 14776340)
	scores := make([]int, 14776340)
	for i := 0; i < 512; i++ {
		scores[i] = -2 // -2 for not set
	}
	// parse
	for i := 0; i < lineNum; i++ {
		div1 := findChar(lines[i], '|', 0)
		div2 := findChar(lines[i], '|', div1+1)
		div3 := findChar(lines[i], '|', div2+1)
		len := length(lines[i])
		// { // print divs
		// 	putInt(div1)
		// 	putchar(' ')
		// 	putInt(div2)
		// 	putchar(' ')
		// 	putInt(div3)
		// 	putchar(' ')
		// 	putInt(len)
		// 	endl()
		// }
		// get substrings
		part1 := substring(lines[i], 0, div1)
		part2 := substring(lines[i], div1+1, div2)
		part3 := substring(lines[i], div2+1, div3)
		part4 := substring(lines[i], div3+1, len)
		// { // print substrings
		// 	putString(part1)
		// 	putchar('!')
		// 	putString(part2)
		// 	putchar('!')
		// 	putString(part3)
		// 	putchar('!')
		// 	putString(part4)
		// 	putchar('~')
		// 	endl()
		// }
		// get id and cred
		id := enc(part1)
		ids[i] = id
		cred := getIntFromStrStart(part2)
		creds[id] = cred
		// get score
		if part4[0] == 'A' {
			scores[id] = 4
		}
		if part4[0] == 'B' {
			scores[id] = 3
		}
		if part4[0] == 'C' {
			scores[id] = 2
		}
		if part4[0] == 'D' {
			scores[id] = 1
		}
		if part4[0] == 'F' {
			scores[id] = 0
		}
		if part4[0] == 0 {
			scores[id] = -1 // for empty string
		}
		// { // print id, cred, score
		// 	putIntCC(id, 'I', '\n')
		// 	putIntCC(cred, 'C', '\n')
		// 	putIntCC(scores[id], 'S', '\n')
		// }
		// get logic
		logic := getDisjunction(part3)
		logics[id] = logic
		// { // print logic
		// 	putSpace(5)
		// 	putchar('-')
		// 	putchar(' ')
		// 	putInt(id)
		// 	putchar(' ')
		// 	putchar('-')
		// 	endl()
		// 	ii := 0
		// 	for logic[ii] != nil {
		// 		jj := 0
		// 		for logic[ii][jj] != -1 {
		// 			putInt(logic[ii][jj])
		// 			putchar(' ')
		// 			jj++
		// 		}
		// 		endl()
		// 		ii++
		// 	}
		// }
	}
	// calculate
	for idx := 0; idx < lineNum; idx++ {
		putIndentCC(1, 430, '{', '\n')
		curId := ids[idx]
		curCred := creds[curId]
		curScore := scores[curId]
		curLogic := logics[curId]
		if curScore == -1 { // empty curScore
			putIndentCC(2, 454, '{', '\n')
			cr = cr + curCred
			putIndentCC(2, 454, '{', '\n')
		}
		if curScore == 0 { // F
			putIndentCC(2, 458, '}', '\n')
			ha = ha + curCred
			cr = cr + curCred
			putIndentCC(2, 458, '{', '\n')
		}
		if curScore > 0 { // ABCD
			putIndentCC(2, 463, '}', '\n')
			ha = ha + curCred
			hc = hc + curCred
			putIndentCC(2, 463, '}', '\n')
		}
		// check prerequisites
		ii := 0
		// disjunction
		boolDisj := 0
		boolConj := 1
		for curLogic[ii] != nil {
			putIndentCC(2, 489, '{', '\n')
			jj := 0
			// conjunction
			boolConj = 1
			for curLogic[ii][jj] != -1 {
				putIndentCC(3, 454, '{', '\n')
				id2 := curLogic[ii][jj]
				score2 := scores[id2]
				if score2 <= 0 {
					putIndentCC(4, 465, '{', '\n')
					// not set or empty or F
					boolConj = 0
					// break // TODO
					putIndentCC(4, 465, '}', '\n')
				}
				jj++
				putIndentCC(3, 454, '}', '\n')
			}
			if boolConj == 1 {
				putIndentCC(3, 488, '{', '\n')
				boolDisj = 1
				// break // TODO
				putIndentCC(3, 488, '}', '\n')
			}
			ii++
			putIndentCC(2, 489, '}', '\n')
		}
		if curLogic[0] == nil {
			putIndentCC(2, 491, '{', '\n')
			// no prerequisite
			boolDisj = 1
			putIndentCC(2, 491, '}', '\n')
		}
		if boolDisj == 1 && curScore <= 0 {
			putIndentCC(2, 497, '{', '\n')
			possible[possibleNum] = curId
			possibleNum++
			putIndentCC(2, 497, '}', '\n')
		}
		// calculate the numerator of GPA * 100
		if curScore > 0 {
			putIndentCC(2, 502, '{', '\n')
			gpa = gpa + 100*curScore*curCred
			// { // print gpa now
			// 	putIndent(2)
			// 	putchar('G')
			// 	putInt(gpa)
			// 	endl()
			// }
			putIndentCC(2, 502, '}', '\n')
		}
		putIndentCC(1, 430, '}', '\n')
	}
	if gpa != 0 {
		gpa = gpa / ha
	}
	// output
	result(gpa, ha, hc, cr, possible, possibleNum)
	return
}

// final output
func result(gpa int, ha int, hc int, cr int, possible []int, possibleNum int) {
	// parameters:
	// gpa - GPA * 100

	// format:
	// GPA: 3.5
	// Hours Attempted: 129
	// Hours Completed: 129
	// Credits Remaining: 0
	// Possible Courses to Take Next
	// None - Congratulations!
	// - Now all above string stored in char array -
	sGpa := []int{'G', 'P', 'A', ':', ' ', 0}
	sHoursAttempted := []int{'H', 'o', 'u', 'r', 's', ' ', 'A', 't', 't', 'e', 'm', 'p', 't', 'e', 'd', ':', ' ', 0}
	sHoursCompleted := []int{'H', 'o', 'u', 'r', 's', ' ', 'C', 'o', 'm', 'p', 'l', 'e', 't', 'e', 'd', ':', ' ', 0}
	sCreditsRemaining := []int{'C', 'r', 'e', 'd', 'i', 't', 's', ' ', 'R', 'e', 'm', 'a', 'i', 'n', 'i', 'n', 'g', ':', ' ', 0}
	sPossibleCourses := []int{'P', 'o', 's', 's', 'i', 'b', 'l', 'e', ' ', 'C', 'o', 'u', 'r', 's', 'e', 's', ' ', 't', 'o', ' ', 'T', 'a', 'k', 'e', ' ', 'N', 'e', 'x', 't', 0}
	sNone := []int{'N', 'o', 'n', 'e', ' ', '-', ' ', 'C', 'o', 'n', 'g', 'r', 'a', 't', 'u', 'l', 'a', 't', 'i', 'o', 'n', 's', '!', 0}
	putString(sGpa)
	putGpa(gpa)
	endl()
	putString(sHoursAttempted)
	putInt(ha)
	endl()
	putString(sHoursCompleted)
	putInt(hc)
	endl()
	putString(sCreditsRemaining)
	putInt(cr)
	endl()
	endl()
	putString(sPossibleCourses)
	endl()
	if possibleNum == 0 && cr == 0 {
		putSpace(2)
		putString(sNone)
		endl()
		return
	}
	for i := 0; i < possibleNum; i++ {
		putSpace(2)
		// fmt.Println("possible[i] = ", possible[i])
		putString(dec(possible[i]))
		endl()
	}
	return
}
