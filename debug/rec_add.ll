target triple = "x86_64-pc-linux-gnu"

declare i32 @ugo_builtin_putint(i32)

define i32 @rec_add(i32 %a, i32 %b) {
entry:
    %tmp1 = icmp eq i32 %a, 0
    br i1 %tmp1, label %done, label %recurse
    recurse:
        %tmp2 = sub i32 %a, 1
        %tmp3 = add i32 %b, 1
        %called = call i32 @rec_add(i32 %tmp2, i32 %tmp3)
        ret i32 %called
    done:
        ret i32 %b
}

define i32 @main() {
    %tmp4 = add i32 0, 4;
    %tmp5 = add i32 0, 99;
    %cast = call i32 @rec_add(i32 %tmp4, i32 %tmp5)
    call i32 @ugo_builtin_putint(i32 %cast)
    ret i32 0
}