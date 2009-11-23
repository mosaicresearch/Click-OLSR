#!/bin/bash

if (( $# == 0 ))
then
	echo "No interfacenames specified..."
	exit 1;
fi

if (( $# % 2 != 0 ))
then
	echo "Usage: olsr.sh interface_name IP_Address [interface_name IP_Address]"
	exit 2;
fi

INTERFACES="";
ADDRESSES="";

j=1;
let "nr = $# / 2";

for i in $*
do
	if (( j % 2 == 1 ))
	then
		INTERFACES="$INTERFACES $i";
	else
		ADDRESSES="$ADDRESSES $i";
	fi

	let "j = $j + 1"	
done

perl make-olsr-config.pl -u -n $nr -i $INTERFACES -a $ADDRESSES > script.click
./click-align -f script.click -o olsr.click 2> /dev/null

echo "To start click, run \"click olsr.click\""
