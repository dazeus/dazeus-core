#!/bin/bash
[ ! -e Makefile ] && cp Makefile.in Makefile

echo "!!! If you get build errors, try looking at the following file !!!"
echo "!!!    plugins/perl/Makefile                                   !!!"
echo "!!! Changes to this file will be preserved.                    !!!"

make
