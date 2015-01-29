#
#Jonathan Flessner
#flessnej@onid.oregonstate.edu
#CS311-400
#Homework 4 Makefile
#28 April 2014
#
CC = gcc
CFLAGS = -Wall -g

progs = sig_demo myar

tests12345 = ar12345.ar myar12345.ar 
tests135 = ar135.ar myar135.ar 
tests24 = ar24.ar myar24.ar
texts = ar-ctoc.txt ar-vtoc.txt myar-ctoc.txt myar-vtoc.txt

all: $(progs)

sig_demo: sig_demo.o
	$(CC) $(CFLAGS) sig_demo.o -o sig_demo

sig_demo.o: sig_demo.c
	$(CC) $(CFLAGS) -c sig_demo.c

myar: myar.o
	$(CC) $(CFLAGS) myar.o -o myar

myar.o: myar.c
	$(CC) $(CFLAGS) -c myar.c

clean:
	rm -f $(progs) *.o *~ sig_demo.exe myar.exe $(tests12345) $(tests135) $(tests24) $(texts) myar12345A.ar myar12345B.ar

clean_tests:
	rm -f $(tests12345) $(tests135) $(tests24) $(texts) myar12345A.ar myar12345B.ar

testq12345:
	rm -f $(tests12345)
	ar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	./myar -q myar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	diff ar12345.ar myar12345.ar

testq135:
	rm -f $(tests135)
	ar q ar135.ar 1-s.txt 3-s.txt 5-s.txt
	./myar -q myar135.ar 1-s.txt 3-s.txt 5-s.txt
	diff ar135.ar myar135.ar

testq24:
	rm -f $(tests24)
	ar q ar24.ar 2-s.txt 4-s.txt
	./myar -q myar24.ar 2-s.txt 4-s.txt
	diff ar24.ar myar24.ar

testqNoAdd:
	rm -f $(tests12345)
	./myar -q myar12345A.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	./myar -q myar12345B.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	diff myar12345A.ar myar12345B.ar
	./myar q myar12345B.ar 2-s.txt 4-s.txt
	# nothing should be added
	diff myar12345A.ar myar12345B.ar 

testq: testq12345 testq135 testq24 testqNoAdd

testt12345:
	rm -f $(tests12345)
	ar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	ar t ar12345.ar > ar-ctoc.txt
	./myar -t ar12345.ar > myar-ctoc.txt
	diff ar-ctoc.txt myar-ctoc.txt

testt135:
	rm -f $(tests135)
	ar q ar135.ar 1-s.txt 3-s.txt 5-s.txt
	ar t ar135.ar > ar-ctoc.txt
	./myar -t ar135.ar > myar-ctoc.txt
	diff ar-ctoc.txt myar-ctoc.txt

testt24:
	rm -f $(tests24)
	ar q ar24.ar 2-s.txt 4-s.txt
	ar t ar24.ar > ar-ctoc.txt
	./myar -t ar24.ar > myar-ctoc.txt
	diff ar-ctoc.txt myar-ctoc.txt

testt: testt12345 testt135 testt24

testv12345:
	rm -f $(tests12345)
	ar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	ar tv ar12345.ar > ar-vtoc.txt
	./myar -v ar12345.ar > myar-vtoc.txt
	diff ar-vtoc.txt myar-vtoc.txt

testv135:
	rm -f $(tests135)
	ar q ar135.ar 1-s.txt 3-s.txt 5-s.txt
	ar tv ar135.ar > ar-vtoc.txt
	./myar -v ar135.ar > myar-vtoc.txt
	diff ar-vtoc.txt myar-vtoc.txt

testv24:
	rm -f $(tests24)
	ar q ar24.ar 2-s.txt 4-s.txt
	ar tv ar24.ar > ar-vtoc.txt
	./myar -v ar24.ar > myar-vtoc.txt
	diff ar-vtoc.txt myar-vtoc.txt

testv: testv12345 testv135 testv24

testd1:
	rm -f $(tests12345)
	ar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	ar d ar12345.ar 1-s.txt 4-s.txt 3-s.txt
	ar t ar12345.ar > ar-ctoc.txt
	rm -f $(tests12345)
	ar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	./myar -d ar12345.ar 1-s.txt 4-s.txt 3-s.txt
	./myar -t ar12345.ar > myar-ctoc.txt
	diff ar-ctoc.txt myar-ctoc.txt

testd2:
	rm -f $(tests12345)
	ar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	ar d ar12345.ar 4-s.txt 1-s.txt
	ar d ar12345.ar 3-s.txt
	ar t ar12345.ar > ar-ctoc.txt
	ar tv ar12345.ar > ar-vtoc.txt
	rm -f $(tests12345)
	./myar q ar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	./myar -d ar12345.ar 4-s.txt 1-s.txt
	./myar -d ar12345.ar 3-s.txt
	./myar -t ar12345.ar > myar-ctoc.txt
	./myar -v ar12345.ar > myar-vtoc.txt
	diff ar-ctoc.txt myar-ctoc.txt
	diff ar-vtoc.txt myar-vtoc.txt

testd: testd1 testd2

testx:
	rm -f $(tests12345)
	./myar q myar12345.ar 1-s.txt 2-s.txt 3-s.txt 4-s.txt 5-s.txt
	mv 4-s.txt 4-sX.txt
	mv 1-s.txt 1-sX.txt
	./myar x myar12345.ar 4-s.txt 1-s.txt
	diff 4-s.txt 4-sX.txt
	diff 1-s.txt 1-sX.txt
	rm -f 4-sX.txt 1-sX.txt

tests: testq testt testv testd testx
	

