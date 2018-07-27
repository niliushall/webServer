webServer: webServer.o locker.h threadPool.h task.h webServer.h
	g++ -o webServer locker.h threadPool.h task.h webServer.h webServer.cpp -lpthread