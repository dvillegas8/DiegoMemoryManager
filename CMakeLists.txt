cmake_minimum_required(VERSION 3.31)
project(DiegoMemoryManager C)

set(CMAKE_C_STANDARD 11)

# Add include directories
include_directories(include)

# Define source files
set(SOURCES
        src/main.c
        src/util.c
        src/trimmer.c
        src/reader.c
        src/pte.c
        src/pfn.c
        src/initializer.c
        src/writer.c
        src/lists.c
        src/threads.c
        src/pagefaulthandler.c
        src/dthreads.c
)

# Define header files (optional but good for clarity)
set(HEADERS
        macros.h
        include/util.h
        include/pfn.h
        include/pte.h
        include/trimmer.h
        include/writer.h
        include/reader.h
        include/initializer.h
        include/lists.h
        include/pagefaulthandler.h
        include/main.h
        include/dthreads.h
)

# Add the executable
add_executable(DiegoMemoryManager ${SOURCES} ${HEADERS})
