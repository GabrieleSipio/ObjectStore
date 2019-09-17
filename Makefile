# _*_ MakeFile _*_
CC       = gcc
CFLAGS   = -std=c99 -Wall
LPATH    = ./Utilities
OPTFLAGS = -g

TARGETS = mainClient  \
			mainServer  

TESTTARGETS = ./test.sh \
				./testsum.sh		

all: $(TARGETS)

mainClient: mainClient.c libobj.a
	$(CC) $(CFLAGS) $(OPTFLAGS) mainClient.c -o $@ -L $(LPATH) -lobj -lpthread

mainServer: mainServer.c libcheck.a
	$(CC) $(CFLAGS) $(OPTFLAGS) mainServer.c -o $@ -L $(LPATH) -lcheck -lpthread	

libobj.a: objstore.o check.o
	ar rvs $(LPATH)/$@ $(LPATH)/objstore.o $(LPATH)/check.o

libcheck.a: check.o
	ar rvs $(LPATH)/$@ $(LPATH)/$<

objstore.o: $(LPATH)/objstore.c
	$(CC) $(CFLAGS) -g $< -c -o $(LPATH)/$@	

check.o: $(LPATH)/check.c
	$(CC) $(CFLAGS) -g $< -c -o $(LPATH)/$@ 





clean		: 
	rm -f $(TARGETS)
	rm -rf data/*
test:
	@./mainServer &
	@./test.sh
	@./testsum.sh
