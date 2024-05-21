# Display timeline of JustWrite version.
export def "jwrite versions" [] {
    let sep = char -u '3000'
    let commits = git log --date=short $'--pretty=format:%cd($sep)%h($sep)%s' -- VERSION | lines | each {|e|
        let data = $e | split row $sep
        {date: $data.0, hash: $data.1, msg: $data.2}
    }
    let versions = $commits | each {|e| git show $'($e.hash):VERSION' | split row " " | get 0 }
    let max_width = $versions | each {|e| $e | str length} | math max
    let prefix = $versions | zip $commits | each {|e|
        $'($e.1.date) ($e.0 | fill -w $max_width) ($e.1.hash) ($e.1.msg)'
    }
    let first_commit = git log --reverse --pretty=format:%h | lines | first
    let commit_list = $commits.hash | append $first_commit
    let diffs = 0..(($commits.hash | length) - 1) | each {|i|
        git diff --shortstat $'($commit_list | get $i)..($commit_list | get ($i + 1))'
    }
    let row_width = $prefix | each {|e| $e | str length} | math max
    $prefix | zip $diffs | each {|e|
        let diff_row = $"('' | fill -w ($max_width + 11))($e.1)"
        print $"\e[34m($e.0 | fill -w $row_width)\e[0m"
        print $"\e[30m($diff_row | fill -w $row_width)\e[0m"
    } | ignore
}
