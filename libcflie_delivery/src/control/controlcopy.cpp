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
#include <unistd.h>
#include "../leap/leap_c.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>


/*EXTENSION*/
#include <GLUT/glut.h>
//NOTE: for windows switch this to GL/glut.h
int w, h;
const int font = (int)GLUT_BITMAP_9_BY_15;

char batteryLevelString[30]; 
char batteryPercentString[30]; 
double cflieBatteryLevel;
double batteryPercent;
char batteryStateString[30]; 
float cflieBatteryState;

char pressureString[30]; 
char temperatureString[30]; 
float cfliePressure;
float cflieTemperature;
char accelerationString[30]; 
char altitudeString[30]; 
float cflieAccX;
float cflieAccY;
float cflieAccZ;
float cflieAltitude;

#define MAXBATTERYLEVEL 4.0

/*EXTENSION*/


using namespace std;

// STATES
#define FLY_STATE 1
#define HOVER_STATE 2
#define LAND_STATE 3
#define PRE_HOVER_STATE 4
#define PRE_FLY_STATE 5

// SIGNALS
#define NO_SIG 11
#define CHANGE_HOVER_SIG 12 
#define TIME_OUT_SIG 13
#define CHANGE_LAND_SIG 14

// TRIM VALUES 
#define TRIM_ROLL 0
#define TRIM_PITCH 0

// OTHER IMPORTANT CONSTANTS 
#define ABS_PITCH_VALUE 8.5 // constant (absolute value) for pitch value if pitch is activated
#define ABS_ROLL_VALUE 8.5 // constant (absolute value) for roll value if roll is activated
#define POS_PITCH_THRESHOLD .45 // threshold for leap direction to set positive pitch
#define NEG_PITCH_THRESHOLD -.45 // threshold for leap direction to set negative pitch
#define POS_ROLL_THRESHOLD .45 // threshold for leap direction sensor to set positive roll
#define NEG_ROLL_THRESHOLD -.45 // threshold for leap direction sensor to set negative roll
#define HOVER_SWIPE_THRESHOLD 600 // threshold for the velocity sensor to interpret hover swipe gesture
#define THRUST_CONSTANT 35700 // constant for base thrust level
#define FINGER_COUNT_THRESHOLD 2 // if we have less than this amount of fingers detected, we will land
#define HOVER_THRUST_CONST 32767 // hover thrust constant (preprogrammed in Crazyflie)
#define LANDING_REDUCTION_CONSTANT 80 // when we are landing, this constant is reduced from thrust every cycle
#define THRUST_MULTIPLIER 50.0 // constant used to calculate thrust
#define BATT_MULTIPLIER_CONST 4.0 // constant used in conjunction with batteryLevel to calculate thrust
#define TIME_GAP 550 // gap for break between state transitions
#define INITIAL_THRUST 10001 // initial thrust level

//The pointer to the crazy flie data structure
CCrazyflie *cflieCopter = NULL;

// KEY GLOBALS
int current_signal = NO_SIG; // default signal is no signal
int current_state = LAND_STATE; //default state is land state
float current_thrust; // holds the current thrust
float current_roll;  // holds the current roll
float current_pitch; // holds the current pitch
double dTimeNow;  // keeps track of time for state transitions
double dTimePrevious = -1; // keeps track of time for state transitions

// COPTER CONTROL HELPER FUNCTIONS

// Fly the copter normally
void flyNormal( CCrazyflie *cflieCopter ) { 
  if ( current_thrust != -1 ) {
    setThrust( cflieCopter, THRUST_CONSTANT + ( current_thrust * ( THRUST_MULTIPLIER - ( BATT_MULTIPLIER_CONST * batteryLevel(cflieCopter) ) ) ) );
  }
  else {
    setThrust( cflieCopter, 0 );
  }
  setPitch( cflieCopter, current_pitch );
  setRoll( cflieCopter, current_roll );
}

// Fly the copter in hover mode
void flyHover( CCrazyflie *cflieCopter ) {
  setThrust( cflieCopter, HOVER_THRUST_CONST );
  setPitch( cflieCopter, current_pitch );
  setRoll( cflieCopter, current_roll );
}

// Land the copter
void land( CCrazyflie *cflieCopter ) {
  current_thrust -= LANDING_REDUCTION_CONSTANT;
  if( ( current_thrust - THRUST_CONSTANT ) < 0 ) {
    current_thrust = -1;
  }
  current_roll = 0;
  current_pitch = 0;
  flyNormal( cflieCopter );
}

// LEAP MOTION CALLBACK FUNCTIONS
void on_init(leap_controller_ref controller, void *user_info)
{
  printf("init\n");
}

void on_connect(leap_controller_ref controller, void *user_info)
{
  printf("connect\n");
}

void on_disconnect(leap_controller_ref controller, void *user_info)
{
  printf("disconnect\n");
}

void on_exit(leap_controller_ref controller, void *user_info)
{
  printf("exit\n");
}

// This function is called every time the leap detects motion
void on_frame(leap_controller_ref controller, void *user_info)
{
  leap_frame_ref frame = leap_controller_copy_frame(controller, 0);
  leap_vector velocity;
  leap_vector position;
  leap_vector direction;

  if ( current_signal == NO_SIG ) {

     // Delay until the time period has expired
    if ( current_state == PRE_HOVER_STATE || current_state == PRE_FLY_STATE ) {
      dTimeNow = currentTime();
      if( dTimePrevious == -1 ) {
        dTimePrevious = dTimeNow;
      }
      if( ( dTimeNow - dTimePrevious ) > TIME_GAP ) {
        current_signal = TIME_OUT_SIG;
        dTimePrevious = -1;
        leap_frame_release( frame );
      } 
      return;
    }

      // Loop through each hand in the frame
    for ( int i = 0; i < leap_frame_hands_count( frame ); i++ ) {

      leap_hand_ref hand = leap_frame_hand_at_index( frame, i );

      // Grab the hand velocity, direction, and position vectors
      leap_hand_palm_velocity( hand, &velocity );
      leap_hand_direction( hand, &direction );
      leap_hand_palm_position( hand, &position );

       // If we detect a swipe gesture (high velocity) and are not in transition already, enter or exit hover mode
      if ( velocity.x > HOVER_SWIPE_THRESHOLD ) {
       current_signal = CHANGE_HOVER_SIG;
       leap_frame_release( frame );
       return;
     }  

    // Send the change land signal if we need to 
     // Trigger for land is sensing less than 2 fingers
     if ( ( leap_hand_fingers_count( hand ) < FINGER_COUNT_THRESHOLD && current_state == FLY_STATE ) || 
      ( leap_hand_fingers_count( hand ) >= FINGER_COUNT_THRESHOLD && current_state == LAND_STATE ) ) {
      current_signal = CHANGE_LAND_SIG;
    leap_frame_release( frame );
    return;
  }

// Set the thrust value
  current_thrust = position.y;

      // Set the roll value
  if ( direction.x > POS_ROLL_THRESHOLD ) {
   current_roll = ABS_ROLL_VALUE;
 }
 else if ( direction.x < NEG_ROLL_THRESHOLD ) {
   current_roll = -ABS_ROLL_VALUE;
 }
 else {
   current_roll = 0;
 }

// Set the pitch value
 if ( direction.y > POS_PITCH_THRESHOLD ) {
   current_pitch = ABS_PITCH_VALUE;
 }
 else if ( direction.y < NEG_PITCH_THRESHOLD ) {
   current_pitch = -ABS_PITCH_VALUE;
 }
 else {
   current_pitch = 0;
 }     
}

leap_frame_release( frame );
return;
}

// Wait for the current signal to be consumed before doing anything else
else {
  leap_frame_release( frame );
  return;
}
}

// Leap motion thread
// It calls the on_frame function when it senses movement
void* leap_thread(void * param){
  struct leap_controller_callbacks callbacks;
  callbacks.on_init = on_init;
  callbacks.on_connect = on_connect;
  callbacks.on_disconnect = on_disconnect;
  callbacks.on_exit = on_exit;
  callbacks.on_frame = on_frame;
  leap_listener_ref listener = leap_listener_new(&callbacks, NULL);
  leap_controller_ref controller = leap_controller_new();
  leap_controller_add_listener(controller, listener);
  while(1);
}

// Copter / main control thread
//This thread will check the current state and send corresponding command to the copter
void* main_control( void * param ) {
  CCrazyflie *cflieCopter = ( CCrazyflie * )param;

  while( cycle( cflieCopter ) ) {

    // Update the FSM based on what signal we are currently reading
    switch ( current_signal ) {

      // If it's no sig, we just want to get back into fly mode if we are landing and then continue
      case NO_SIG:
      break;

      // If it's change hover sig, we want to change to the pre-fly / pre-hover state
      case CHANGE_HOVER_SIG:
      if ( current_state == HOVER_STATE ) {
        current_state = PRE_FLY_STATE;
      }
      else if ( current_state == FLY_STATE ) {
        current_state = PRE_HOVER_STATE;
      }
      break;

      // If it's land sig, we want to get to land state
      case CHANGE_LAND_SIG:
      if ( current_state == LAND_STATE ) {
        current_state = FLY_STATE;
      }
      else if ( current_state == FLY_STATE ) {
        current_state = LAND_STATE;
      }
      break;

      // If it's time out sig, we want to move out of our pre-fly / pre-hover states
      case TIME_OUT_SIG:
      if ( current_state == PRE_FLY_STATE ) {
        printf( "Changing to normal fly state.\n" );
        turnOffHoverMode( cflieCopter );
        current_state = FLY_STATE;
      }
      else if ( current_state == PRE_HOVER_STATE ) {
        printf( "Changing to hover state.\n" );
        turnOnHoverMode( cflieCopter ); 
        current_state = HOVER_STATE;
      }
      break;
    } 

     // Consume the current signal
    current_signal = NO_SIG;   

    // Perform another switch case where appropriate state actions are executed
    switch( current_state ) {

      case PRE_FLY_STATE:
      flyHover( cflieCopter );
      break;

      case FLY_STATE:
      flyNormal( cflieCopter );
      break;

      case LAND_STATE:
      land( cflieCopter );
      break;

      case HOVER_STATE:
      flyHover( cflieCopter );
      break;

      case PRE_HOVER_STATE:
      flyNormal( cflieCopter );
      break;

    }
}
  printf("%s\n", "exit");
  return 0;
}

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
    cflieBatteryLevel = (batteryLevel(cflieCopter));
    sprintf(batteryLevelString, "batteryLevel : %f", cflieBatteryLevel );
    batteryPercent = 100.0 * (cflieBatteryLevel / MAXBATTERYLEVEL );
    sprintf(batteryPercentString, "batteryPercent : %d%%", (int)batteryPercent );

    cfliePressure = pressure(cflieCopter);
    cflieTemperature = temperature(cflieCopter);
    cflieBatteryState = batteryState(cflieCopter);
    cflieAccX = accX(cflieCopter);
    cflieAccY = accY(cflieCopter);
    cflieAccZ = accZ(cflieCopter);
    cflieAltitude = asl(cflieCopter);

    sprintf(batteryStateString, "batteryState : %f", cflieBatteryState );
    sprintf(temperatureString, "temperature : %f", cflieTemperature );
    sprintf(pressureString, "pressure : %f", cfliePressure );
    sprintf(accelerationString, "acceleration X: %f Y: %f Z: %f", cflieAccX, cflieAccY, cflieAccZ );
    sprintf(temperatureString, "altitude : %f", cflieAltitude );

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
    exit(-1);
  }
  fclose(fp);
}


//This this the main function, use to set up the radio and init the copter
int main( int argc, char **argv ) {
  CCrazyRadio *crRadio = new CCrazyRadio;
  CCrazyRadioConstructor( crRadio,"radio://0/34/250K" );
  
  if(startRadio(crRadio)) {

    // Construct the crazyflie and radio
    cflieCopter = new CCrazyflie;
    CCrazyflieConstructor( crRadio,cflieCopter );

    //Initialize the thrust value
    setThrust( cflieCopter, INITIAL_THRUST );
    
    // Enable sending the setpoints. This can be used to temporarily
    // stop updating the internal controller setpoints and instead
    // sending dummy packets (to keep the connection alive).
    setSendSetpoints(cflieCopter,true);

    // Set up the leap and main copter control threads
    pthread_t leapThread;
    pthread_t mainThread;
    pthread_create( &leapThread, NULL, leap_thread, NULL ); 
    pthread_create( &mainThread, NULL, main_control, cflieCopter );

    /*EXTENSION*/
    glutInit(&argc, argv);
    glutInitWindowSize(640,480);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);     
    glutCreateWindow("Crazyflie Stat Tracker");     
    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutTimerFunc(25, update, 0);     
    glutMainLoop();
    /*EXTENSION*/

    printf("at the end");

    // Loop until we exit
    while ( 1 ) {}

  } 
else {
  printf("%s\n", "Could not connect to dongle. Did you plug it in?");
}
return 0;
}
