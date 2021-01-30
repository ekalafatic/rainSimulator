#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>

#define MAX_PARTICLES_DROPS 1000
#define MAX_PARTICLES_FLOOR 50

using namespace std;


float slowdown = 800;
float gravity = -10;    // cambiar la gravedad para cambiar la vel?
float viento[3] = {0.f,0.f,0.f};
bool drops_die = false;
bool llueve = true;

float floor_color[]={.2f,.2f,.2f,.6f},
       rain_color[]={.8f,.8f,.8f,.6f}; // con transparencia queda mas logrado

typedef struct {
   // vida
   bool alive;	// la particula esta viva?
   float life;	// total de vida de la particula
   float fade;  // valor con el que decae la vida en cada iteracion
   // color
   float red; 
   float green;
   float blue;
   // posicion
   float xpos; 
   float ypos; 
   float zpos;
   // direccion y velocidad
   float vel;
   // gravedad: se suma en cada iteracion a la velocidad, para simular
   // la aceleracion de la gota.
   float gravity;
} particles;

particles par_sys_drops[MAX_PARTICLES_DROPS];
particles par_sys_floor[MAX_PARTICLES_FLOOR];

// inicializar particulas: setear valores p/ cada una
void initParticlesDrops(int i) {
   par_sys_drops[i].alive = true;
   par_sys_drops[i].life = 2;
//   par_sys_drops[i].fade = float(rand()%100)/1000.0f+0.003f;
   par_sys_drops[i].fade = float(rand()%100)/10.0f+0.003f;
   
   par_sys_drops[i].xpos = (float) (rand() % 20)+0.5; // valor entre 0.5 y 19.5 para que quede dentro del cubo
   par_sys_drops[i].ypos = 20;                      // las gotas arrancan sobre la cara superior del cubo
   par_sys_drops[i].zpos = (float) (rand() % 20)+0.5;
   
   par_sys_drops[i].red = rain_color[0];
   par_sys_drops[i].green = rain_color[1];
   par_sys_drops[i].blue = rain_color[2];
   
   par_sys_drops[i].vel = 0.0;
   par_sys_drops[i].gravity = gravity;
}

void initParticlesFloor(int i) {
   par_sys_floor[i].alive = true;
   par_sys_floor[i].life = 1.0; // tienen menos vida que las gotas
   par_sys_floor[i].fade = 0.01+float(rand()%100)/1000.0f+0.003f;
   
   par_sys_floor[i].xpos = (float) (rand() % 20)+0.5;
   par_sys_floor[i].ypos = 0.0; // func. diferente a "drops", siempre tienen y=0
   par_sys_floor[i].zpos = (float) (rand() % 20)+0.5;
   
   par_sys_floor[i].red = rain_color[0];
   par_sys_floor[i].green = rain_color[1];
   par_sys_floor[i].blue = rain_color[2];
   
   par_sys_floor[i].vel = 0; // no tienen movimiento
   par_sys_floor[i].gravity = 0;
}

void drawRain() {
   float x, y, z;
   for (int i = 0; i < MAX_PARTICLES_DROPS; i=i+1) {
      if (par_sys_drops[i].alive == true) {			
         
         x = par_sys_drops[i].xpos;
         y = par_sys_drops[i].ypos;
         z = par_sys_drops[i].zpos;
         
         // que solo dibuje las que comenzaron a bajar
         // evita que esten todo el tiempo gotas "colgadas" en la parte superior del cubo
         if(par_sys_drops[i].ypos<19.5) { // cuando la gota entera atraviesa la superficie superior, se dibuja
            glColor4fv(rain_color);
            glBegin(GL_LINES);
            glVertex3f(x, y, z);
            glVertex3f(x-(viento[0]/2), y+0.5, z-(viento[2]/2));
            glEnd();
         }
         
         // actualizar posicion o muerte
         par_sys_drops[i].ypos += par_sys_drops[i].vel / (slowdown*2); // se la divide por una cte. para reducir el avance
         
         // viento: si la gota sale del cuadrado, entra por el lado opuesto a la misma altura y
         par_sys_drops[i].xpos += viento[0];
         if (par_sys_drops[i].xpos>=20) par_sys_drops[i].xpos = par_sys_drops[i].xpos - 20;
         if (par_sys_drops[i].xpos<=0)  par_sys_drops[i].xpos = par_sys_drops[i].xpos + 20;
         par_sys_drops[i].zpos += viento[2];
         if (par_sys_drops[i].zpos>=20) par_sys_drops[i].zpos = par_sys_drops[i].zpos - 20;
         if (par_sys_drops[i].zpos<=0)  par_sys_drops[i].zpos = par_sys_drops[i].zpos + 20;
         
         par_sys_drops[i].vel += par_sys_drops[i].gravity;
         // Decay
         par_sys_drops[i].life -= par_sys_drops[i].fade;
         
         // revive porque toco el suelo
         if (par_sys_drops[i].ypos <= 0) {
            par_sys_drops[i].life = -1.0;
            drops_die = true;
         }
         //Revive porque se le acabo la vida
         if (par_sys_drops[i].life < 0.0) {
            initParticlesDrops(i);
         }
      }
   }
}

void drawFloorWater() {
   float x, y, z;
   for (int i = 0; i < MAX_PARTICLES_FLOOR; i=i+2) {
      if (par_sys_floor[i].alive == true) {
         
         x = par_sys_floor[i].xpos;
         y = par_sys_floor[i].ypos;
         z = par_sys_floor[i].zpos;
         
         // resta vida
         par_sys_floor[i].life -= par_sys_floor[i].fade;
         
         // dibuja un octogono centrado en x, y
         // dependiendo de la vida, lo dibuja mas o menos amplio
         if (par_sys_floor[i].life >= 0.50) {
            glColor4fv(rain_color);
            glBegin(GL_LINE_LOOP);//dibujaria un octagono alrededor del punto donde caeria la gota
            glVertex3f(x-0.00, y+0.05, z+0.10);
            glVertex3f(x-0.05, y+0.05, z+0.05);
            glVertex3f(x-0.10, y+0.05, z+0.00);
            glVertex3f(x-0.05, y+0.05, z-0.05);
            glVertex3f(x+0.00, y+0.05, z-0.10);
            glVertex3f(x+0.05, y+0.05, z-0.05);
            glVertex3f(x+0.10, y+0.05, z+0.00);
            glVertex3f(x+0.05, y+0.05, z+0.05);
            glEnd();
         }
         if (par_sys_floor[i].life >= 0.0) {
            // Draw particles
            glColor4fv(rain_color);
            glBegin(GL_LINE_LOOP);
            glVertex3f(x-0.00, y+0.15, z+0.15);
            glVertex3f(x-0.10, y+0.15, z+0.10);
            glVertex3f(x-0.15, y+0.15, z+0.00);
            glVertex3f(x-0.10, y+0.15, z-0.10);
            glVertex3f(x+0.00, y+0.15, z-0.15);
            glVertex3f(x+0.10, y+0.15, z-0.10);
            glVertex3f(x+0.15, y+0.15, z+0.00);
            glVertex3f(x+0.10, y+0.15, z+0.10);
            glEnd();
         }
         //Revive
         if (par_sys_floor[i].life <= 0.0 && llueve) {
            initParticlesFloor(i);
         }
      }
   }
}

