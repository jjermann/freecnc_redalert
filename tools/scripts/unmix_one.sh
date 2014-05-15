#!/bin/sh

n=`echo $1 | sed s/\.[Mm][Ii][Xx]//`
if [[ -d $n ]];then
  echo "directory $n exists!"
else
  mkdir $n
fi

cd $n
#cp ../mixfiles.txt .
#../unmix ../$1
pwd
cp /usr/local/bin/mixfiles.txt .
unmix ../$1

while [[ `echo *.MIX` != \*.MIX ]];do
  for j in *.MIX; do
    if [[ -f $j ]];then
      ../unmix $j
      rm $j
    fi
  done
done

rm mixfiles.txt
cd ..
