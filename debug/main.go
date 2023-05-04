package main

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
			// arr[i], arr[j] = arr[j], arr[i]
			temp := arr[i]
			arr[i] = arr[j]
			arr[j] = temp
			i++
			j--
		}
	}
	// swap
	temp := arr[left]
	arr[left] = arr[j]
	arr[j] = temp
	return j
}

func main() {
	var arr []int
	var n int
	n = getint()
	for i := 0; i < n; i++ {
		var tmp int
		tmp = getint()
		arr = append(arr, tmp)
	}
	quicksort(arr, 0, n-1)
	for i := 0; i < n; i++ {
		putint(arr[i])
	}
}

// cat sort.temp.in | go run ./sort.go ./runtime.go
