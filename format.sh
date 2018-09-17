find "src" \( -name '*.h' -or -name '*.cpp' \) -print0 | xargs -0 "clang-format" -i -style=file

filelist=`git ls-files | awk '!/extern/' | grep 'src/*' | grep -E '*\.(cpp|h)'`

for f in $filelist; do
echo "${f}"
	clang-format -i -style=file "${f}"
done