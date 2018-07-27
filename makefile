.PHONY: clean

webServer: webServer.o
	g++ -o webServer webServer.cpp -lpthread

clean:
	rm *.o