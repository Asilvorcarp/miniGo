package main

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

func quicksort(arr []int, left int, right int) {
	if left < right {
		pivotIndex := partition(arr, left, right)
		quicksort(arr, left, pivotIndex-1)
		quicksort(arr, pivotIndex+1, right)
	}
}

func partition(arr []int, left int, right int) int {
	pivot := arr[left]
	i := left + 1
	j := right
	for i <= j {
		for i <= j && arr[i] < pivot {
			i++
		}
		for i <= j && arr[j] > pivot {
			j--
		}
		if i <= j {
			// swap
			arr[i], arr[j] = arr[j], arr[i]
			i++
			j--
		}
	}
	// swap
	arr[left], arr[j] = arr[j], arr[left]
	return j
}

func main() {
	arr := make([]int, 1024)
	var n int
	n = getInt()
	for i := 0; i < n; i++ {
		arr[i] = getInt()
	}
	quicksort(arr, 0, n-1)
	for i := 0; i < n; i++ {
		println(arr[i])
	}
}

// cat sort.temp.in | go run ./sort.go ./runtime.go
