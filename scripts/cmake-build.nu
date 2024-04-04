def build_type_completer [] -> string {
    ["Debug", "Release"]
}

# Build JustWrite by CMake
def cmake-build [build_type: string@build_type_completer] {
    while ('build/.lock' | path exists) {}
    touch build/.lock
    do --ignore-errors {
        cmake -B build -S . -G Ninja -DCMAKE_INSTALL_PREFIX=build/JustWrite $'-DCMAKE_BUILD_TYPE=($build_type)'
        cmake --build build --target install
    }
    rm build/.lock
}

# Run JustWrite
def cmake-run [] {
    while ('build/.lock' | path exists) {}
    ./build/JustWrite/JustWrite.exe
}

def cmake-watch [build_type: string@build_type_completer] {
    rm -f build/.lock
    watch -r true src {
        let pid = ps | where name =~ 'JustWrite' | get 0.pid --ignore-errors | default '-1' | into int;
        if $pid != -1 {
            kill -f -q $pid
        }
        cmake-build $build_type
    }
    rm -f build/.lock
}
