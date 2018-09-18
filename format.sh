find "src" \( -name '*.h' -or -name '*.cpp' \) \( -type d -o -type f \) -print0 | 
	awk '/src/' | awk '!/extern/' | 
	grep '^src' | grep -E '*\.(cpp|h)' | 
	xargs -0 "clang-format" -i -style=file

filelist=`git ls-files -z --full-name | awk '/src/' | awk '!/extern/' | grep '^src' | grep -E '*\.(cpp|h)'`

for f in $filelist; do
	clang-format -i -style=file "${f}"
done