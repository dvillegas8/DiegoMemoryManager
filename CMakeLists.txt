cmake_minimum_required(VERSION 3.31)
project(DiegoMemoryManager C)

set(CMAKE_C_STANDARD 11)

add_executable(DiegoMemoryManager src/main.c
        macros.h
        include/util.h
        include/pfn.h
        include/pte.h
        include/trimmer.h
        include/writer.h
        include/reader.h
        src/util.c
        src/trimmer.c
        src/reader.c
        src/pte.c
        src/pfn.c
        src/initializer.c
        include/initializer.h
        src/writer.c
        include/lists.h
        src/lists.c
        src/threads.c
        include/pagefaulthandler.h
        src/pagefaulthandler.c
)
