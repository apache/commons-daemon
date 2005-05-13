mkdir -p ent
rm -f JSVC.1
while true
do
  FILE=`docbook2man jsvc.1.xml 2>&1 | grep FileNotFoundException | awk -F FileNotFoundException: ' { print $2 } ' | awk ' { print $1 } '`
  if [ -f JSVC.1 ]
  then
    break
  fi
  echo "FILE: $FILE"
  file=`basename $FILE`
  dir=`dirname $FILE`
  man=`basename $dir`
  echo "file: $file dir: $dir man: $man"
  if [ "$man" = "ent" ]
  then
    (cd ent; wget http://www.oasis-open.org/docbook/xml/4.1.2/ent/$file)
  else
    wget http://www.oasis-open.org/docbook/xml/4.1.2/$file
  fi
done
