#script provides env setup and common methods
set -u #exit on uninit var
set -e #exit on error

self=`basename $0`
home=$USERPROFILE
projectDir=$selfDir/../../..
build=$projectDir/build
src=$projectDir/src
doc=$projectDir/doc
log=$build/tools.log

#killTree pid sig
killTree()
{
    local pid=$1
    local sig=${2:-TERM}

    for child in $(pgrep -P $pid)
    do
        killTree $child $sig
    done
    kill -$sig $pid

    #wait a moment for process to die gracefully, otherwise force kill until dead
    timeout=3
    start=$(date +%s)
    while ps -p $pid > /dev/null
    do
        if [ $(($(date +%s)-start)) -le $timeout ]; then continue; fi
        kill -KILL $pid
        start=$(date +%s)
    done
}

stacktrace()
{
    # Hide the stacktrace() call.
    local -i start=$(( ${1:-0} + 1 ))
    local -i end=${#BASH_SOURCE[@]}
    local -i i=0
    local -i j=0
     
    echo "Stacktrace (last called on top):"
    for ((i=$start; i < $end; i++)); do
        j=$(( $i - 1 ))
        local function="$FUNCNAME[$i]"
        local file="$BASH_SOURCE[$i]"
        local line="$BASH_LINENO[$j]"
        echo "    $function() in $file:$line"
    done
}

exit_()
{
    trap - EXIT
    local cmd=$(echo "$1" | xargs) error=$2
    if [ "$error" -eq 0 ]; then return; fi
    echo "Error: command exited with code $error: \"$cmd\""
    stacktrace 1
    exit $error
}

trapExit()
{
    local cmd="$BASH_COMMAND" error=$?
    if [ "$error" -eq 0 ]; then return; fi
    echo "Error: command exited with code $error: \"$cmd\""
    stacktrace 1
}

ignoreError()
{
    set +e
    eval "$@"
    cmd="$@" error=$?
    set -e
}

mkdir -p $build

trap trapExit EXIT
