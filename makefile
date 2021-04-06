test : test.o bt.o
	g++ -o test test.o bt.o

test.o : test.cc db.h bplusTree.h
	g++ -c test.cc

bt.o : bplusTree.cc bplusTree.h db.h
	g++ -c bplusTree.cc -o bt.o

clean:
	rm *.o