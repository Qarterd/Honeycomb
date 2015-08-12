selfDir=$(cd $(dirname $0) && pwd)
. $selfDir/../../sh-util/win/core.sh

echo "Copying visualizer to Visual Studio user dir"
dstDir="$home/Documents/Visual Studio 2012/Visualizers"
mkdir -p "$dstDir"
cp "$src/win/Honeycomb.natvis" "$dstDir"
