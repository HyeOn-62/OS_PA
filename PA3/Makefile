all:
	gcc -shared -fPIC -o ddetector.so ddetector.c -ldl
	gcc -shared -fPIC -o dmonitor.so dmonitor.c -ldl
	gcc -pthread -o dpredictor dpredictor.c
	gcc -pthread -o example/abba -g example/abba.c
	gcc -pthread -o example/dinning_deadlock -g example/dinning_deadlock.c
	gcc -pthread -o example/gateLock -g example/gateLock.c
	gcc -pthread -o example/singleThreadCycle -g example/singleThreadCycle.c
	gcc -pthread -o example/threadCreation -g example/threadCreation.c

clean:
	rm -rf example/abba ddetector.so example/dinning_deadlock dmonitor.so dpredictor dmonitor.trace example/gateLock example/singleThreadCycle example/threadCreation
