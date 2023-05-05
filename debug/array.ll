; package main

; To run this:
; clang runtime.o.ll ../debug/array.ll -o ../debug/main.out
; ../debug/main.out

target triple = "x86_64-pc-linux-gnu"

declare i32 @runtime_getchar()
declare i32 @runtime_getint()
declare i32 @runtime_putchar(i32)
declare i32 @runtime_putint(i32)
declare i32 @runtime_println(i32)
declare i32 @runtime_exit(i32)

define void @main_main() {
	%arr = alloca i32, i32 10004, align 4
    store i32 0, ptr %arr, align 4
    %elem1 = getelementptr i32, ptr %arr, i32 1
    store i32 1, ptr %elem1, align 4
    %elem2 = getelementptr i32, ptr %arr, i32 2
    store i32 2, ptr %elem2, align 4
    %elem3 = getelementptr i32, ptr %arr, i32 3
    store i32 3, ptr %elem3, align 4
    ; print arr[1]
    %e1 = getelementptr i32, ptr %arr, i32 1
    %elem1_val = load i32, ptr %e1, align 4
    call i32 @runtime_putint(i32 %elem1_val)

    ; ------ begin of solution ------

    ; array of [4 x [5 x i32]] as ptr
    %spaa = alloca ptr, align 4 ; done by assign
    ; template: pT1 = alloca T1, i32 numOfElements, align 4
    %paa = alloca ptr, i32 4, align 4 ; done by make
    store ptr %paa, ptr %spaa, align 4

    %pa0 = alloca i32, i32 5, align 4
        %paa_0 = load ptr, ptr %spaa, align 4
        %spa0 = getelementptr inbounds ptr, ptr %paa_0, i32 0 ; index arr[0]
    store ptr %pa0, ptr %spa0, align 4

    %pa1 = alloca i32, i32 5, align 4
    %paa_1 = load ptr, ptr %spaa, align 4
    %spa1 = getelementptr inbounds ptr, ptr %paa_1, i32 1 ; index arr[1]
    store ptr %pa1, ptr %spa1, align 4
    ; init done

    ; set 110 to arr[1][1]
    %sp11 = getelementptr inbounds ptr, ptr %pa1, i32 1 ; index arr[1][1]
    store i32 110, ptr %sp11, align 4 ; set to 110

    ; now get the 110 from %spaa
    %paa_ = load ptr, ptr %spaa, align 4
    %spa1_ = getelementptr inbounds ptr, ptr %paa_, i32 1 ; index arr[1]
    %pa1_ = load ptr, ptr %spa1_, align 4
    %sp11_ = getelementptr inbounds ptr, ptr %pa1_, i32 1 ; index arr[1][1]
    %sp11_val = load i32, ptr %sp11_, align 4
    call i32 @runtime_putint(i32 %sp11_val)

    ; ------ end of solution ------

    ; array of [4 x [5 x i32]]
    %arr2 = alloca [4 x [5 x i32]], align 4
    %el0 = getelementptr [4 x [5 x i32]], [4 x [5 x i32]]* %arr2, i32 0, i32 0, i32 0
    ; get the pointer to the first element of the array !
    store i32 53, i32* %el0, align 4
    %el1 = getelementptr [4 x [5 x i32]], [4 x [5 x i32]]* %arr2, i32 0, i32 0, i32 1
    store i32 233, i32* %el1, align 4
    %el2 = getelementptr [4 x [5 x i32]], [4 x [5 x i32]]* %arr2, i32 0, i32 0, i32 2
    store i32 666, i32* %el2, align 4
    %f1 = getelementptr [4 x [5 x i32]], [4 x [5 x i32]]* %arr2, i32 0, i32 1, i32 0
    store i32 555, i32* %f1, align 4
    ; print arr2[1]
    %e2 = getelementptr [4 x [5 x i32]], [4 x [5 x i32]]* %arr2, i32 0, i32 0, i32 1
    %elem2_val = load i32, i32* %e2, align 4
    call i32 @runtime_putint(i32 %elem2_val)

    ; get el2 from el0
    %e3 = getelementptr i32, i32* %el0, i32 1
    %elem3_val = load i32, i32* %e3, align 4
    call i32 @runtime_putint(i32 %elem3_val)

    ; get f1 from el0 ; second sub array
    %f1hh = getelementptr i32, i32* %el0, i32 5
    %f1hh_val = load i32, i32* %f1hh, align 4
    call i32 @runtime_putint(i32 %f1hh_val)

    ; thus to get arr[i][j]:
    ; %t = getelementptr i32, i32* %el0, i32 (i * 5 + j)

    ; ; get i32** of arr2
    ; %arr2_ptr = alloca [4 x [5 x i32]]*, align 4
    ; store [4 x [5 x i32]]* %arr2, [4 x [5 x i32]]** %arr2_ptr, align 4
    ; %arr2_ptr_val = load [4 x [5 x i32]]*, [4 x [5 x i32]]** %arr2_ptr, align 4
    ; ; get i32* of arr2[1]
    ; %arr2_1_ptr = getelementptr [4 x [5 x i32]], [4 x [5 x i32]]* %arr2_ptr_val, i32 0, i32 1
    ; %arr2_1_ptr_val = load [5 x i32]*, [5 x i32]** %arr2_1_ptr, align 4
    ; ; get i32 of arr2[1][2]
    ; %arr2_1_2_ptr = getelementptr [5 x i32], [5 x i32]* %arr2_1_ptr_val, i32 0, i32 2
    ; %arr2_1_2_ptr_val = load i32*, i32** %arr2_1_2_ptr, align 4
    ; %arr2_1_2_val = load i32, i32* %arr2_1_2_ptr_val, align 4
    ; call i32 @runtime_putint(i32 %arr2_1_2_val)



	ret void
}

define void @main_init() {
	ret void
}

define i32 @main() {
	call void() @main_init()
	call void() @main_main()
	ret i32 0
}
