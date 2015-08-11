self=`basename $0`
self_dir=`dirname $0`
home=$HOME
project_dir=$self_dir/../../..
build=$project_dir/build
src=$project_dir/src
doc=$project_dir/doc
log=$build/tools.log

function check_error()
{
    if [ ${1} -eq 0 ]; then return; fi
    echo Build failed. Error: ${1}                  2>&1 | tee -a $log
    echo "Press any key to continue..."
    read -s -n 1 any_key
}

mkdir -p $build