// Copyright (c) 2013, Jan Winkler <winkler@cs.uni-bremen.de>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Universität Bremen nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


#include <cflie/CCrazyflie.h>
#include <stdio.h>

using namespace std;

/*EXTENSION*/
#include <GLUT/glut.h>
//NOTE: for windows switch this to GL/glut.h
int w, h;
const int font=(int)GLUT_BITMAP_9_BY_15;

char batteryLevelString[30]; 
char batteryPercentString[30]; 
double batteryLevel;
double batteryPercent;
char batteryStateString[30]; 
float batteryState;

char pressureString[30]; 
char temperatureString[30]; 
float pressure;
float temperature;
char accelerationString[30]; 
char altitudeString[30]; 
float accX;
float accY;
float accZ;
float altitude;

#define MAXBATTERYLEVEL 4

/*EXTENSION*/
/*EXTENSION*/
//CITE: http://www.programming-techniques.com/2012/05/font-rendering-in-glut-using-bitmap.html

//resizes the viewport
static void resize(int width, int height){
    const float ar = (float) width / (float) height;
    w = width;
    h = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);     
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
} 

//prints the characters in a string in the correct direction and spacing
void setOrthographicProjection() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glScalef(1, -1, 1);
    glTranslatef(0, -h, 0);
    glMatrixMode(GL_MODELVIEW);
} 

//screen refresh helper
void resetPerspectiveProjection() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
} 

//Renders the string as a set of characters
void renderBitmapString(float x, float y, void *font,const char *string){
    const char *c;
    glRasterPos2f(x, y);
    for (c=string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
} 

//display function 
static void display(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //set the color to red
    glColor3d(1.0, 0.0, 0.0);

    //start changes
    setOrthographicProjection();
    glPushMatrix();
    glLoadIdentity();

    //print stats
    renderBitmapString(20,20,(void *)font,"Copter Battery Life");
    renderBitmapString(300,100,(void *)font,accelerationString);
    renderBitmapString(300,150,(void *)font,batteryStateString);
    renderBitmapString(300,200,(void *)font,temperatureString);
    renderBitmapString(300,250,(void *)font,altitudeString);
    renderBitmapString(300,300,(void *)font,pressureString);

    //draw battery shell
    int i = 60;
    renderBitmapString(20,i-20,(void *)font,"        _____");
    renderBitmapString(20,i-10,(void *)font,"        |   |");
    renderBitmapString(20,i,(void *)font,"________|   |________");
    renderBitmapString(20,i+40,(void *)font,"          +");
    while (i < 360){
    i+= 10;
    renderBitmapString(20,i,(void *)font,"|                  |");
    }
    renderBitmapString(20,i-40,(void *)font,"          -");
    renderBitmapString(20,i,(void *)font,"____________________");

    //fill battery
    i = 0;
    while (i < batteryPercent){

    glColor3d(1.0 - batteryPercent/100.0, batteryPercent/100, 0.0);
    renderBitmapString(29,300 - ((i/100.0)*300) + 60,(void *)font,"@@@@@@@@@@@@@@@@@@");
    i+= 5;
    }

    //print battery level
    renderBitmapString(20,400, (void*)font, batteryLevelString);
    //print battery percent
    renderBitmapString(20,420, (void*)font, batteryPercentString);

    //end changes
    glPopMatrix();
    resetPerspectiveProjection();
    glutSwapBuffers();
} 

//Gets the stats and calls redisplay every second
void update(int value){
    batteryLevel = (batteryLevel(cflieCopter));
    sprintf(batteryLevelString, "batteryLevel : %f", batteryLevel );
    batteryPercent = 100.0 * (batteryLevel / MAXBATTERYLEVEL );
    sprintf(batteryPercentString, "batteryPercent : %d%%", (int)batteryPercent );

    pressure = pressure(cflieCopter);
    temperature = temperature(cflieCopter);
    batteryState = batteryState(cflieCopter);
    accX = accX(cflieCopter);
    accY = accY(cflieCopter);
    accZ = accZ(cflieCopter);
    altitude = asl(cflieCopter);

    sprintf(batteryStateString, "batteryState : %f", batteryState );
    sprintf(temperatureString, "temperature : %f", temperature );
    sprintf(pressureString, "pressure : %f", pressure );
    sprintf(accelerationString, "acceleration X: %f Y: %f Z: %f", accX, accY, accZ);
    sprintf(temperatureString, "altitude : %f", altitude );

    //1000 ms timer to call update
    glutTimerFunc(1000, update, 0);
    glutPostRedisplay();
} 
/*EXTENSION*/

//Campbell's code for file opening
void WriteMemoryToFileOrDie(char* filename, char* data, int len) {
  int i, j;
  FILE* fp = fopen(filename, "w");
  if (!fp) {
    perror("Open file failed \n");
    exit(-1);
  }
  j = 0;
  while ((i = fwrite((data + j), sizeof(char), (len - j), fp)) > 0)
    j = j + i;
  if (j != len) {
    perror("Write file failed\n");
    exit(-1);2
  }
  fclose(fp);
}

int main(int argc, char **argv) {
  CCrazyRadio *crRadio = new CCrazyRadio;
  CCrazyRadioConstructor(crRadio,"radio://0/10/250K");
  
  if(startRadio(crRadio)) {
    CCrazyflie* cflieCopter=new CCrazyflie;
    CCrazyflieConstructor(crRadio,cflieCopter);

    //Initialize the set value
    setThrust(cflieCopter,10001);
    // Enable sending the setpoints. This can be used to temporarily
    // stop updating the internal controller setpoints and instead
    // sending dummy packets (to keep the connection alive).
    setSendSetpoints(cflieCopter,true);

/*EXTENSION*/
    glutInit(&argc, argv);
    glutInitWindowSize(640,480);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);     glutCreateWindow("Font Rendering Using Bitmap Font - Programming Techniques0");     glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutTimerFunc(25, update, 0);     
    glutMainLoop();
/*EXTENSION*/

    int i=0;
    while(cycle(cflieCopter)) {
      // Main loop. Currently empty.

      if(i<1200)setThrust(cflieCopter,37001);
      
      }
    
    delete cflieCopter;
  } else {
    printf("%s\n","Could not connect to dongle. Did you plug it in?" ); 
  }
  
  delete crRadio;
  return 0;
}
