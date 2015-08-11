. tools-common.sh
doxygen=$doc/Doxygen

echo "Setting PATH env (requires restart)"     2>&1 | tee -a $log
sudo sh -c 'echo "setenv PATH $PATH" >> /etc/launchd.conf'

echo "Copying Honeycomb tex package to MacTeX user repository"  2>&1 | tee -a $log
dst_dir="$home/Library/texmf/tex/latex/"
mkdir -p "$dst_dir"
cp "$doxygen/Honeycomb.sty" "$dst_dir"

check_error ${PIPESTATUS[0]}