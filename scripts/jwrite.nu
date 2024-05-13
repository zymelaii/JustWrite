def find-process [
    name: string,
] -> int {
    ps | where name =~ 'JustWrite' | get 0.pid --ignore-errors | default '-1' | into int
}

def build_type_completer [] -> string {
    ["Debug", "Release", "Default"]
}

def rollback-build-number [] {
    let version = open VERSION | parse -r '(?<version>\d+\.\d+\.\d+)\.(?<build_number>\d+)' | get 0
    let build_number = [0, (($version.build_number | into int) - 1)] | math max
    $'($version.version).($build_number)' | save -f VERSION
}

def wait-target [] {
    while ('build/.lock' | path exists) {}
}

def lock-target [] {
    wait-target
    touch build/.lock
}

def unlock-target [] {
    rm -f build/.lock
}

def test-and-update-md5 [
    file: string,
] {
    let md5_list = 'build/.md5-list';
    if not ($md5_list | path exists) {
        mkdir ($md5_list | path dirname)
        '{}' | save -f $md5_list
    }
    mut cached_hash = open $md5_list | from json
    let hash_value = open $file | hash md5
    let old_value = $cached_hash | get $file --ignore-errors | default ''
    if $old_value == '' {
        $cached_hash = ($cached_hash | insert $file $old_value)
    }
    if $hash_value != $old_value {
        $cached_hash | update $file $hash_value | to json | save -f $md5_list
        true
    } else {
        false
    }
}

def "jwrite config" [
    build_type: string@build_type_completer, # Build type
] {
    do --ignore-errors {
        if $build_type == 'Default' {
            cmake -B build -S . -G Ninja -DCMAKE_INSTALL_PREFIX=build/JustWrite
        } else {
            cmake -B build -S . -G Ninja -DCMAKE_INSTALL_PREFIX=build/JustWrite $'-DCMAKE_BUILD_TYPE=($build_type)'
        }
    } | complete | get exit_code
}

# Build JustWrite.
export def "jwrite build" [
    build_type: string@build_type_completer = 'Default', # Build type
] {
    lock-target
    let date = date now | format date "%Y-%m-%d %H:%M:%S"
    print $"\e[37;1m($date) [INFO]\e[0m build JustWrite"
    mut success = true;
    let exit_code = jwrite config $build_type
    if $exit_code == 0 {
        let exit_code = do --ignore-errors {
            cmake --build build --target install
        } | complete | get exit_code
        if $exit_code != 0 {
            rollback-build-number
            $success = false
        }
    } else {
        $success = false
    }
    let date = date now | format date "%Y-%m-%d %H:%M:%S"
    if $success {
        print $"\e[32;1m($date) [PASS]\e[0m build JustWrite done"
    } else {
        print $"\e[31;1m($date) [FAIL]\e[0m failed to build JustWrite"
    }
    unlock-target
}

# Run JustWrite.
export def "jwrite run" [] {
    wait-target
    ./build/JustWrite/JustWrite.exe
}

# Run JustWrite unittests.
export def "jwrite test" [] {
    let target = './build/JustWrite/jwrite-test.exe'
    if not ($target | path exists) { return }
    wait-target
    ^$target
}

# Run JustWrite auto-build server.
export def "jwrite watch" [
    build_type: string@build_type_completer = 'Default', # Build type
    --build-once (-b), # Build once immediately after setup
] {
    let dirs = [src, test]
    for dir in $dirs {
        ls -f $dir | each {|e| test-and-update-md5 $e.name }
    }
    if $build_once {
        jwrite build $build_type
    }
    unlock-target
    watch -r true . { |op, path|
        for dir in $dirs {
            if not ($path | path relative-to (pwd)| str starts-with $'($dir)\') { continue }
            if not (test-and-update-md5 $path) { break }
            let pid = find-process JustWrite
            if $pid != -1 {
                kill -f -q $pid
            }
            jwrite build $build_type
            break
        }
    }
    unlock-target
}

# Display timeline of JustWrite VERSION.
export def "jwrite ls-versions" [] {
    let commits = git log --pretty=format:'%h %s' -- VERSION | lines
    let versions = $commits | each {|e| git show $'($e | split row " " | get 0):VERSION' | split row " " | get 0 }
    let max_width = $versions | each {|e| $e | str length} | math max
    $versions | zip $commits | each {|e|
        print $'($e.0 | fill -w $max_width) ($e.1)'
    } | ignore
}
