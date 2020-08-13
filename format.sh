if [ "$1" = "pre-commit" ]; then
	git diff --name-only --cached -- | egrep '\.h$|\.cpp$' | awk '/src/' | awk '!/extern/' | xargs "clang-format" -style=file -i
	git diff --name-only --cached -- | egrep '\.h$|\.cpp$' | awk '/src/' | awk '!/extern/' | xargs git add 
else
	find "src" \( -name '*.h' -or -name '*.cpp' \) \( -type d -o -type f \) -print0 | awk '/src/' | awk '!/extern/' | xargs -0 "clang-format" -style=file -i
fi

filelist=`git ls-files -z --full-name | awk '/src/' | grep '^src' | grep -E '.*\.(cpp|h)'`
for f in $filelist; do
	if [ "$1" = "pre-commit" ]; then
		clang-format -i -style=file "${f}"
	else
		clang-format -i -style=file "${f}" && git add "${f}"
	fi
done
