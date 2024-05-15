# Display timeline of JustWrite version.
export def "jwrite versions" [] {
    let commits = git log --pretty=format:'%h %s' -- VERSION | lines
    let versions = $commits | each {|e| git show $'($e | split row " " | get 0):VERSION' | split row " " | get 0 }
    let max_width = $versions | each {|e| $e | str length} | math max
    $versions | zip $commits | each {|e|
        print $'($e.0 | fill -w $max_width) ($e.1)'
    } | ignore
}
