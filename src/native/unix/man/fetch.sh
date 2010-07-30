# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
