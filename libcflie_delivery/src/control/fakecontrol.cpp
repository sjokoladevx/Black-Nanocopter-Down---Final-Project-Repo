#include <stdio.h>
#include <unistd.h>
#include "../leap/leap_c.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
/*EXTENSION*/
#include <GLUT/glut.h>

double batteryLevel;
double batteryPercent;

char batteryLevelString[30]; 
char batteryPercentString[30];  
char accelerationStringX[30]; 
char accelerationStringY[30]; 
char accelerationStringZ[30]; 
char gyroscopeStringX[30]; 
char gyroscopeStringY[30]; 
char gyroscopeStringZ[30];
char finalString[400];

int writeToFile(){
 sprintf(accelerationStringX, "acceleration X: %f", 0);
    sprintf(gyroscopeStringX, "gyroscope X: %f ", 0);
    sprintf(accelerationStringY, "acceleration Y: %f ", 0);
    sprintf(gyroscopeStringY, "gyroscope Y: %f ", 0);
    sprintf(accelerationStringZ, "acceleration Z: %f", 0 );
    sprintf(gyroscopeStringZ, "gyroscope Z: %f", 0 );

sprintf(finalString, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%f", batteryLevelString, batteryPercentString, accelerationStringX, accelerationStringY, accelerationStringZ, gyroscopeStringX, gyroscopeStringY, gyroscopeStringZ, batteryPercent );

FILE *f = fopen("output.txt", "w");
if (f == NULL)
{
    printf("Error opening file!\n");
    exit(1);
}

fprintf(f, "%s", finalString);

fclose(f);

return 0;

}


int main( int argc, char **argv ) {


	while(1){
	sleep(1);
 	batteryLevel = (rand() / (double)RAND_MAX) * 4;
    sprintf(batteryLevelString, "batteryLevel : %f", batteryLevel );
    batteryPercent = 100.0 * (batteryLevel / 4 );
    sprintf(batteryPercentString, "batteryPercent : %d%%", (int)batteryPercent );
    printf("writing to file");
    writeToFile();
}

}