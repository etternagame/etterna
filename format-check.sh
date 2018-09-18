cleanstate=`git status | grep "modified"`
if ! [[ -z $cleanstate ]]; then
  echo "Script must be applied on a clean git state"
  exit 1
fi

filelist=`git ls-files -z --full-name | awk '/src/' | awk '!/extern/' | grep '^src' | grep -E '*\.(cpp|h)'`

for f in $filelist; do
	clang-format -i -style=file "${f}"
done

notcorrectlist=`git status | grep "modified"`

if [[ -z $notcorrectlist ]]; then
  # send a negative message to gitlab
  echo "Excellent. **VERY GOOD FORMATTING!** :thumbsup:"
  exit 0;
else
  echo "The following files have clang-format problems (showing patches)";
  for f in $notcorrectlist; do
      echo $f
      git diff $f
  done
fi

exit 1