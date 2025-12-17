# Variant: 21

# Build and run
```bash
mkdir ./build
cd ./build

gcc -o liblibrary_1.so -shared -fPIC ../src/library_1.c
gcc -o liblibrary_2.so -shared -fPIC ../src/library_2.c
gcc -o main_runtime ../main_runtime_link.c -I./include -ldl
gcc -o main_compile ../main_compile_link.c -I./include -L. -llibrary_1
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```

```bash
./main_runtime
./main_compile
```
