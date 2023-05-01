# miniGo

The compiler for a subset of Go, implemented in C++.

## grammar

// TODO

keywords:
```
"package"
"import"
"var"
"func"
"return"
"if"
"else"
"for"
"break"
"continue"
"defer"
"goto"
```

## build

```bash
cmake -S "repo目录" -B "build目录" -DLIB_DIR="libkoopa目录" -DINC_DIR="libkoopa头文件目录"
cmake --build "build目录" -j `nproc`
```
