# Display timeline of JustWrite version.
export def "jwrite versions" [] {
    let commits = git log --pretty=format:'%h %s' -- VERSION | lines | each {|e|
        let commit = $e | split row " " | get 0
        let message = $e | str substring (($commit | str length) + 1)..
        {commit: $commit, message: $message}
    }
    let versions = $commits | each {|e| git show $'($e.commit):VERSION' | split row " " | get 0 }
    let max_width = $versions | each {|e| $e | str length} | math max
    let prefix = $versions | zip $commits | each {|e|
        $'($e.0 | fill -w $max_width) ($e.1.commit) ($e.1.message)'
    }
    let max_width = $prefix | each {|e| $e | str length} | math max
    let first_commit = git log --reverse --pretty=format:%h | lines | first
    let commit_list = $commits.commit | append $first_commit
    let diffs = 0..(($commits.commit | length) - 1) | each {|i|
        git diff --shortstat $'($commit_list | get $i)..($commit_list | get ($i + 1))'
    }
    $prefix | zip $diffs | each {|e|
        print $'($e.0 | fill -w $max_width) ($e.1)'
    } | ignore
}
