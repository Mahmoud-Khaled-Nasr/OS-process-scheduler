build: 
	g++ clk.cpp -o clk.out
	g++ scheduler.cpp -o scheduler.out
	g++ FilesGenerator.cpp -o FilesGenerator.out
	g++ processGenerator.cpp -o processGenerator.out
	g++ process.cpp -o process.out
clean:
	rm -f *.out  processes.txt
	ipcrm -M 300
	ipcrm -Q 777
	@echo "All Items Cleared"

all: clean build

run:
	./processGenerator.out
