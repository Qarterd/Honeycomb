self=`basename $0`
self_dir=`dirname $0`
home=$USERPROFILE
build=$self_dir/../../build
doc=$self_dir/../../doc
src=$self_dir/../../src
log=$build/tools.log

check_error()
{
    if [ ${1} -eq 0 ]; then return; fi
    echo Build failed. Error: ${1}                  2>&1 | tee -a $log
    echo "Press any key to continue..."
    read -s -n 1 any_key
}

mkdir -p $build