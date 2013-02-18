. tools-common.sh
doxygen=$doc/Doxygen

echo "Copying Honeycomb tex package to MiKTeX user repository"  2>&1 | tee -a $log
dst_dir="$home/AppData/Local/MiKTeX/2.9/tex/latex/Honeycomb"
mkdir -p "$dst_dir"
cp "$doxygen/Honeycomb.sty" "$dst_dir"

check_error ${PIPESTATUS[0]}