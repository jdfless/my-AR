This repository contains my implementation of the unix ar utility as well as a very simple demonstartion of signal handling.

The myar.c implementation has much of the functionality of the traditional ar utility (specifically the -q, -x, -t, -v, and -d flags) while also including two new features, -A which appends all 'regular' files in the current directory to the archive and -w which for a given time-out will add all files that have been changed to the archive.

This myar implementation was a great exercise in learning about reads and writes as well as how to program from documentation without instruction.  It is also over 1k lines of code written and tested in a week.
