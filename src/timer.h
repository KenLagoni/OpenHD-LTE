  
/*
	timer.h
	Copyright (c) 2021 Lagoni
	Not for commercial use
 */ 

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h> 
#include <cstdio>
#include <sys/time.h>

class Timer {
private:

    timeval startTime;
	double duration;
	double duration_ms;
	double totalDuration_ms;

public:

    void start(){
        gettimeofday(&startTime, NULL);
    }

    double stop(){
        timeval endTime;
        long seconds, useconds;


        gettimeofday(&endTime, NULL);

        seconds  = endTime.tv_sec  - startTime.tv_sec;
        useconds = endTime.tv_usec - startTime.tv_usec;

        this->duration = seconds + useconds/1000000.0;
		this->duration_ms = this->duration*1000.0;
		this->totalDuration_ms += this->duration_ms;

        return this->duration_ms;
    }
		
	void reset(){
		this->totalDuration_ms=0;
	}
	
	void stop(std::string text){
		fprintf(stderr, "%s - (%.0lf)ms \n",text.c_str(),this->duration_ms);
	}

	double getTotalDuration(void){
		return this->totalDuration_ms;
	}

    static void printTime(double duration){
        printf("%5.6f seconds\n", duration);
    }
};

#endif /* TIMER_H_ */