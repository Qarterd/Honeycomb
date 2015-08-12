selfDir=$(cd $(dirname $0) && pwd)
. $selfDir/../../sh-util/mac/core.sh
doxygen=$doc/Doxygen

echo "Setting PATH env (requires restart)"
sudo sh -c 'echo "setenv PATH $PATH" >> /etc/launchd.conf'

echo "Copying Honeycomb tex package to MacTeX user repository"
dstDir="$home/Library/texmf/tex/latex/"
mkdir -p "$dstDir"
cp "$doxygen/Honeycomb.sty" "$dstDir"
