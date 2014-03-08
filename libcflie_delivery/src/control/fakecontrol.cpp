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
char batteryStateString[30]; 
char temperatureString[30]; 
char pressureString[30]; 
char accelerationString[90]; 
char altitudeString[30];
char finalString[400];

int writeToFile(){

sprintf(batteryStateString, "batteryState : %f", 0.0 );
sprintf(temperatureString, "temperature : %f", 0.0 );
sprintf(pressureString, "pressure : %f", 0.0 );
sprintf(accelerationString, "acceleration X: %f Y: %f Z: %f", 1.0, 1.0, 1.0 );
sprintf(altitudeString, "altitude : %f", 0.0 );

sprintf(finalString, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%f", batteryLevelString, batteryPercentString, batteryStateString, temperatureString, pressureString, accelerationString, altitudeString, batteryPercent );

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