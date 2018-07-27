.PHONY: clean

server: server.o
	g++ -o server server.cpp webServer.cpp task.cpp -lpthread

clean:
	rm *.o server