#!/bin/bash

if [ $# -lt 2 ]; then
	echo "usage: $0 <input_file> <output_file>"
	exit 1
fi

IF="$1"
OF="$2"
SIGSIG='SHA1SUM SIG'
SIG=`grep "$SIGSIG" "$IF" | sed 's/.*'"$SIGSIG"'\s\+\([_a-zA-Z][_a-zA-Z0-9]*\).*/\1/'`
SHA1=`cat settings.c | sed -n '/SHA1SUM BEGIN/,/SHA1SUM END/p' | sha1sum | sed 's/\(.\{8\}\).*/0x\1/'`
cat > "$OF" <<-EOF
	/* GENERATED CONTENT: do not edit by hand */
	#ifndef $SIG
	#define $SIG $SHA1
	#endif
	EOF
exit 0
