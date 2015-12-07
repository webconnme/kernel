#!/bin/sh

ARG1=$1
ARG2=$2
FILE_NAME=$3

fix_all() {
	for FILE_LIST in `find ./ -type f -maxdepth 1 -name 'cfg_gpio*' | sed 's/.\///g' `

	do
		echo "$SRC to $DST in $FILE_LIST"
		sed -i 's/'$SRC'/'$DST'/g' $FILE_LIST
	done
}

fix_file() {
	echo "$ARG1 to $ARG2 in $FILE_NAME"
	sed -i 's/'$ARG1'/'$ARG2'/g' $FILE_NAME
}

if [ "$1" = "" ]; then
	echo -n "파일 전체 수정?(y/n)"
	read INPUT

	if [ $INPUT = "y" ]; then
		echo -n "원본 내용: "
		read SRC
		echo -n "바꿀 내용: "
		read DST 

		echo -n "진행합니까?(y/n): "
		read INPUT
		if [ $INPUT = "y" ]; then
			fix_all
		fi
	fi
else
	fix_file
fi
