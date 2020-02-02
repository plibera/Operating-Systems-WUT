#!/bin/sh

cc main.c -o vfs

errors=0

test_file()
{
  ./vfs testDisk d $1
  DIFF=$(diff $1 $2)
  if [ "$DIFF" != "" ]
  then
    echo $1 and $2 are not the same
    errors=$(($errors + 1))
  fi
  rm $1
}

echo Internal fragmentation demonstration:
./vfs testDisk c 20
echo Upload 2kB file
./vfs testDisk u 2kb.txt 2kb_file
./vfs testDisk l
echo Upload 1,5kB file
./vfs testDisk u 1,5kb.txt 1,5kb_file
./vfs testDisk l
./vfs testDisk m

echo Smaller 1,5kB file used 2kB, wasting 0,5kB of memory
./vfs testDisk R y

echo
echo Free space control test:
./vfs testDisk c 17
./vfs testDisk m
echo Try to upload 10kB file:
./vfs testDisk u 10kb.txt file
./vfs testDisk m
echo Upload 2kB until full:
./vfs testDisk u 2kb.txt file1
./vfs testDisk u 2kb.txt file2
./vfs testDisk u 2kb.txt file3
./vfs testDisk u 2kb.txt file4
./vfs testDisk u 2kb.txt file5
./vfs testDisk m
echo Upload 1kB file:
./vfs testDisk u 1kb.txt file5
./vfs testDisk m
./vfs testDisk l
./vfs testDisk R y

echo
echo External fragmentation and defragmentation:
./vfs testDisk c 24
echo Fill the disk with 2kB files:
./vfs testDisk u 2kb.txt file1
./vfs testDisk u 2kb.txt file2
./vfs testDisk u 2kb.txt file3
./vfs testDisk u 2kb.txt file4
./vfs testDisk u 2kb.txt file5
./vfs testDisk u 2kb.txt file6
./vfs testDisk u 2kb.txt file7
./vfs testDisk u 2kb.txt file8
./vfs testDisk l
./vfs testDisk m
echo Remove some of them leaving the disk fragmented:
./vfs testDisk r file2
./vfs testDisk r file3
./vfs testDisk r file4
./vfs testDisk r file6
./vfs testDisk r file7
./vfs testDisk l
./vfs testDisk m
echo Upload 10kB file that requires defragmentation:
./vfs testDisk u 10kb.txt bigFile
./vfs testDisk l
./vfs testDisk m
echo Test files after defragmentation:
test_file file1 2kb.txt
test_file file5 2kb.txt
test_file file8 2kb.txt
test_file bigFile 10kb.txt
echo Errors: $errors
./vfs testDisk R y

echo
echo Try to create a disk in a file smaller than 8kB:
./vfs testDisk c 5
echo Try to create a disk larger than 8 MB:
./vfs testDisk c 10000

echo
echo Try to upload a file with the same name twice:
./vfs testDisk c 20
./vfs testDisk u 2kb.txt
./vfs testDisk u 2kb.txt
./vfs testDisk l
./vfs testDisk R y

echo
echo Test uploading and downloading:
echo Vfs output will be saved to copyTest.txt
./vfs testDisk c 104 >copyTest.txt
i=70
while [ $i -gt 0 ]
do
  ./vfs testDisk u 10kb.txt file1 >>copyTest.txt
  ./vfs testDisk u 2kb.txt file2 >>copyTest.txt
  ./vfs testDisk u 10kb.txt file3 >>copyTest.txt
  ./vfs testDisk u 2kb.txt file4 >>copyTest.txt
  test_file file1 10kb.txt >>copyTest.txt
  ./vfs testDisk r file1 >>copyTest.txt
  ./vfs testDisk u 2kb.txt file5 >>copyTest.txt
  ./vfs testDisk u 10kb.txt file6 >>copyTest.txt
  test_file file2 2kb.txt >>copyTest.txt
  ./vfs testDisk r file2 >>copyTest.txt
  test_file file3 10kb.txt >>copyTest.txt
  ./vfs testDisk r file3 >>copyTest.txt
  test_file file4 2kb.txt >>copyTest.txt
  ./vfs testDisk r file4 >>copyTest.txt
  test_file file5 2kb.txt >>copyTest.txt
  ./vfs testDisk r file5 >>copyTest.txt
  test_file file6 10kb.txt >>copyTest.txt
  ./vfs testDisk r file6 >>copyTest.txt
  ./vfs testDisk u 1kb.txt $i >>copyTest.txt
  i=`expr $i - 1`
done
i=70
while [ $i -gt 0 ]
do
  test_file $i 1kb.txt >>copyTest.txt
  ./vfs testDisk r $i >>copyTest.txt
  i=`expr $i - 1`
done
echo Errors: $errors
./vfs testDisk R y >>copyTest.txt
