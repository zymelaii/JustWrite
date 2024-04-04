def build_type_completer [] -> string {
    ["Debug", "Release"]
}

# Build JustWrite by CMake
def cmake-build [build_type: string@build_type_completer] {
    cmake -B build -S . -G Ninja -DCMAKE_INSTALL_PREFIX=build/JustWrite $'-DCMAKE_BUILD_TYPE=($build_type)'
    cmake --build build --target install

}

# Run JustWrite
def cmake-run [] {
    ./build/JustWrite/JustWrite.exe
}
