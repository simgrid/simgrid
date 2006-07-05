include ./Makedefs

default: pre executable 

pre:
	@rm -f ./make.log
	@echo "Make log for Mixtesim" > ./make.log

executable: 
	cd src; make all

wc:
	@ wc -l `find . -name "*.[ch]"` | grep total

clean:
	@ cd src;        $(MAKE) clean
	@ /bin/rm -f ./bin/mixtesim
