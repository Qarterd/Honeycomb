selfDir=$(cd $(dirname $0) && pwd)
. $selfDir/../../sh-util/win/core.sh
doxygen=$doc/Doxygen

echo "Copying Honeycomb tex package to MiKTeX user repository"
dstDir="$home/AppData/Local/MiKTeX/2.9/tex/latex/Honeycomb"
mkdir -p "$dstDir"
cp "$doxygen/Honeycomb.sty" "$dstDir"
