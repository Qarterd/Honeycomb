. tools-common.sh

echo "Copying visualizer to Visual Studio user dir"     2>&1 | tee -a $log
dst_dir="$home/Documents/Visual Studio 2012/Visualizers"
mkdir -p "$dst_dir"
cp "$src/win/Honeycomb.natvis" "$dst_dir"

check_error ${PIPESTATUS[0]}