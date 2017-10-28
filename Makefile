build: 
	g++ -std=c++11 clk.cpp -o clk.out
	g++ -std=c++11 scheduler.cpp -o scheduler.out
	g++ -std=c++11 FilesGenerator.cpp -o FilesGenerator.out
	g++ -std=c++11 processGenerator.cpp -o processGenerator.out
	g++ -std=c++11 process.cpp -o process.out
clean:
	rm -f *.out  processes.txt
	ipcrm -M 300
	ipcrm -Q 777
	@echo "All Items Cleared"

all: clean build

run:
	./processGenerator.out
