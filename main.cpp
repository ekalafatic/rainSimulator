#include <cmath> // atan sqrt
#include <cstdlib> // exit
#include <iostream> // cout
#include <fstream> // file io
#include <GL/glut.h>


#define MAX_PARTICLES_DROPS 1000
#define MAX_PARTICLES_FLOOR 50

/// Authors: Emiliano Kalafatic, Martín Córdoba

using namespace std;

// variables globales y defaults

int
    w=500,h=500, // tamaño de la ventana
    boton=-1, // boton del mouse clickeado
    xclick,yclick; // x e y cuando clickeo un boton

float // luces y colores en float
    line_color[]={.2f,.3f,.4f,0},
    linewidth=1, // ancho de lineas
    escala=15,escala0, // escala de los objetos window/modelo pixeles/unidad
    eye[]={0,0,30}, target[]={0,0,0}, up[]={0,1,0}, // camara, mirando hacia y vertical
    //   eye[]={0,0,5}, target[]={0,0,0}, up[]={0,1,0}, // camara, mirando hacia y vertical
    znear=10, zfar=50, //clipping planes cercano y alejado de la camara (en 30 => veo de 20 a -20)
    //   znear=2, zfar=8, //clipping planes cercano y alejado de la camara (en 5 => veo de 20 a -20)
    amy,amy0, // angulo del modelo alrededor del eje y
    ac0,rc0; // angulo resp x y distancia al target de la camara al clickear

bool // variables de estado de este programa
    perspectiva=true, // perspectiva u ortogonal
    rota=false,       // gira continuamente los objetos respecto de y
    dibuja=true,      // false si esta minimizado
    wire=true,       // dibuja lineas o no
    relleno=true,     // dibuja relleno o no
    cl_info=true,     // informa por la linea de comandos
    blend=true;      // transparencias

extern bool
    llueve,    /// detiene o continua la lluvia
    drops_die;
// cuando la primer gota muere en el piso (drops_die=true), se
// comienza a mostrar el segundo juego de particulas... Esto se condiciona en
// display_cb luego de llamar a drawRain()

extern float
    floor_color[],
    rain_color[]; // con transparencia queda mas logrado

extern float
    slowdown,
    gravity,    // cambiar la gravedad para cambiar la vel?
    viento[3];

short modifiers=0;  // ctrl, alt, shift (de GLUT)
inline short get_modifiers() {return modifiers=(short)glutGetModifiers();}

// temporizador:
static const int ms_lista[]={1,2,5,10,20,50,100,200,500,1000,2000,5000},ms_n=12;
static int ms_i=4,msecs=ms_lista[ms_i]; // milisegundos por frame

static const double R2G=45/atan(1.0);

GLuint texid;
bool texmode=true; // modo de aplicacion de textura
static const unsigned int texmodelist[]={GL_REPLACE,GL_BLEND,0};
static const char texmodestring[3][20]={"GL_DECAL","GL_REPLACE","Sin Textura"};

// Load a PPM Image
bool mipmap_ppm(const char *ifile) {
    char dummy; int maxc,wt,ht;
    ifstream fileinput(ifile, ios::binary);
    if (!fileinput.is_open()) {cerr<<"Not found"<<endl; return false;}
    fileinput.get(dummy);
    if (dummy!='P') {cerr<<"Not P6 PPM file"<<endl; return false;}
    fileinput.get(dummy);
    if (dummy!='6') {cerr<<"Not P6 PPM file"<<endl; return false;}
    fileinput.get(dummy);
    dummy=fileinput.peek();
    if (dummy=='#') do {
        fileinput.get(dummy);
    } while (dummy!=10);
    fileinput >> wt >> ht;
    fileinput >> maxc;
    fileinput.get(dummy);
    
    unsigned char *img=new unsigned char[3*wt*ht];
    fileinput.read((char *)img, 3*wt*ht);
    fileinput.close();
    
    unsigned char *imga=new unsigned char[4*wt*ht];
    for (int i=0;i<wt*ht;i++){
        imga[4*i+0]=img[3*i+0];
        imga[4*i+1]=img[3*i+1];
        imga[4*i+2]=img[3*i+2];
        imga[4*i+3]=240;//alfa de la textura, seteada a 240.
        
    }
    delete[] img;
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, wt, ht, GL_RGBA, GL_UNSIGNED_BYTE, imga);
    delete[] imga;
    return true;
}

extern void drawFloorWater();
extern void drawRain();

// redibuja los objetos
// Cada vez que hace un redisplay
void Display_cb() {
    
    if (!dibuja) return;
    
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    
    /// dibuja los ejes (para tener una referencia nada mas)
    glBegin(GL_LINES);
    glColor3d(1,0,0); glVertex3d(1.1,  0,  0); glVertex3d(1.5,  0,  0); // Eje x = Rojo
    glColor3d(0,1,0); glVertex3d(  0,1.1,  0); glVertex3d(  0,1.5,  0); // Eje y = Verde
    glColor3d(0,0,1); glVertex3d(  0,  0,1.1); glVertex3d(  0,  0,1.5); // Eje z = Azul
    glEnd();
    
    glPushMatrix();
    
    glTranslated(-10,-10,-10);
    
    drawRain();
    if(drops_die && gravity!=0) { // no comienza a dibujar salpicaduras hasta que una gota toque el suelo
        drawFloorWater();
    }
    
    /// dibuja caja de alambre (limite)
    if (wire){
        glColor3fv(line_color);
        glBegin(GL_LINE_STRIP);
        glVertex3i( 0, 0, 0);
        glVertex3i( 0,20, 0);
        glVertex3i(20,20, 0);
        glVertex3i(20, 0, 0);
        glVertex3i( 0, 0, 0);
        
        glVertex3i( 0, 0,20);
        glVertex3i(20, 0,20);
        glVertex3i(20,20,20);
        glVertex3i( 0,20,20);
        glVertex3i( 0, 0,20);
        glEnd();
        
        glBegin(GL_LINES); // tres aristas faltantes
        glVertex3i( 0,20, 0);
        glVertex3i( 0,20,20);
        glVertex3i(20,20,20);
        glVertex3i(20,20, 0);
        glVertex3i(20, 0, 0);
        glVertex3i(20, 0,20);
        glEnd();
    }
    
    glPopAttrib();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    /// dibuja cara inferior (suelo)
    if (relleno){
        glBegin(GL_QUADS);
        glColor4fv(floor_color);
        glTexCoord2d(0,0); glVertex3i( 0, 0, 0); // coordenadas para cheker.ppm
        glTexCoord2d(0,1); glVertex3i( 0, 0,20);
        glTexCoord2d(1,1); glVertex3i(20, 0,20);
        glTexCoord2d(1,0); glVertex3i(20, 0, 0);
        glEnd();
    }
    
    glPopAttrib();
    glPopMatrix();
    
    glutSwapBuffers();
    
#ifdef _DEBUG
    // chequea errores
    int errornum=glGetError();
    while(errornum!=GL_NO_ERROR){
        if (cl_info){
            if(errornum==GL_INVALID_ENUM)
                cout << "GL_INVALID_ENUM" << endl;
            else if(errornum==GL_INVALID_VALUE)
                cout << "GL_INVALID_VALUE" << endl;
            else if (errornum==GL_INVALID_OPERATION)
                cout << "GL_INVALID_OPERATION" << endl;
            else if (errornum==GL_STACK_OVERFLOW)
                cout << "GL_STACK_OVERFLOW" << endl;
            else if (errornum==GL_STACK_UNDERFLOW)
                cout << "GL_STACK_UNDERFLOW" << endl;
            else if (errornum==GL_OUT_OF_MEMORY)
                cout << "GL_OUT_OF_MEMORY" << endl;
        }
        errornum=glGetError();
    }
#endif // _DEBUG
}

// Regenera la matriz de proyeccion
// cuando cambia algun parametro de la vista
void regen() {
    
    if (!dibuja) return;
    
    // matriz de proyeccion
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    
    double w0=(double)w/2/escala,h0=(double)h/2/escala; // semiancho y semialto en el target
    
    // frustum, perspective y ortho son respecto al eye pero con los z positivos (delante del ojo)
    if (perspectiva){ // perspectiva
        double // "medio" al cuete porque aqui la distancia es siempre 5
            delta[3]={(target[0]-eye[0]), // vector ojo-target
                (target[1]-eye[1]),
                (target[2]-eye[2])},
            dist=sqrt(delta[0]*delta[0]+
            delta[1]*delta[1]+
            delta[2]*delta[2]);
        w0*=znear/dist,h0*=znear/dist;  // w0 y h0 en el near
        glFrustum(-w0,w0,-h0,h0,znear,zfar);
    }
    else { // proyeccion ortogonal
        glOrtho(-w0,w0,-h0,h0,znear,zfar);
    }
    
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); // matriz del modelo->view
    gluLookAt(   eye[0],   eye[1],   eye[2],
        target[0],target[1],target[2],
        up[0],    up[1],    up[2]);// ubica la camara
    glRotatef(amy,0,1,0); // rota los objetos alrededor de y
    glutPostRedisplay(); // avisa que hay que redibujar
}

// Animacion
// Si no hace nada hace esto glutIdleFunc lo hace a la velocidad que de la maquina
// glutTimerFunc lo hace cada tantos milisegundos
void Idle_cb() {
    static int suma,counter=0;// esto es para analisis del framerate real
    // Cuenta el lapso de tiempo
    static int anterior=glutGet(GLUT_ELAPSED_TIME); // milisegundos desde que arranco
    if (msecs!=1){ // si msecs es 1 no pierdo tiempo
        int tiempo=glutGet(GLUT_ELAPSED_TIME), lapso=tiempo-anterior;
        if (lapso<msecs) return;
        suma+=lapso;
        if (++counter==100) {
            // cout << "<ms/frame>= " << suma/100.0 << endl;
            counter=suma=0;
        }
        anterior=tiempo;
    }
    if (rota) { // los objetos giran 'amy' grados alrededor de y
        amy+=0.3; if (amy>360) amy-=360;
        regen();
        if (llueve) glutPostRedisplay();
    }
    else {if (llueve) glutPostRedisplay();} // redibuja
}

// Maneja cambios de ancho y alto de la ventana
void Reshape_cb(int width, int height){
    h=height; w=width;
    if (cl_info) cout << "reshape: " << w << "x" << h << endl;
    
    if (w==0||h==0) {// minimiza
        dibuja=false; // no dibuja mas
        glutIdleFunc(0); // no llama a cada rato a esa funcion
        return;
    }
    else if (!dibuja&&w&&h){// des-minimiza
        dibuja=true; // ahora si dibuja
        glutIdleFunc(Idle_cb); // registra de nuevo el callback
    }
    
    glViewport(0,0,w,h); // region donde se dibuja
    
    regen(); //regenera la matriz de proyeccion
    Display_cb();
}

// Movimientos del mouse
void Motion_cb(int xm, int ym){ // drag
    if (boton==GLUT_LEFT_BUTTON){
        if (modifiers==GLUT_ACTIVE_SHIFT){ // cambio de escala
            escala=escala0*exp((yclick-ym)/100.0);
            regen();
        }
        else { // manipulacion
            double yc=eye[1]-target[1],zc=eye[2]-target[2];
            double ac=ac0+(ym-yclick)*720.0/h/R2G;
            yc=rc0*sin(ac); zc=rc0*cos(ac);
            up[1]=zc; up[2]=-yc;  // perpendicular
            eye[1]=target[1]+yc; eye[2]=target[2]+zc;
            amy=amy0+(xm-xclick)*180.0/w;
            regen();
        }
    }
}

// Clicks del mouse
void Mouse_cb(int button, int state, int x, int y){
    static bool old_rota=false;
    if (button==GLUT_LEFT_BUTTON){
        if (state==GLUT_DOWN) {
            xclick=x; yclick=y;
            boton=button;
            old_rota=rota; rota=false;
            get_modifiers();
            glutMotionFunc(Motion_cb); // callback para los drags
            if (modifiers==GLUT_ACTIVE_SHIFT){ // cambio de escala
                escala0=escala;
            }
            else { // manipulacion
                double yc=eye[1]-target[1],zc=eye[2]-target[2];
                rc0=sqrt(yc*yc+zc*zc); ac0=atan2(yc,zc);
                amy0=amy;
            }
        }
        else if (state==GLUT_UP){
            rota=old_rota;
            boton=-1;
            glutMotionFunc(0); // anula el callback para los drags
        }
    }
}

// Teclado
// Special keys (non-ASCII)
// aca es int key
void Special_cb(int key,int xm=0,int ym=0) {
    if (key==GLUT_KEY_F4){ // alt+f4 => exit
        get_modifiers();
        if (modifiers==GLUT_ACTIVE_ALT)
            exit(EXIT_SUCCESS);
    }
    if (key==GLUT_KEY_UP){ /// aumenta viento hacia -z
        viento[2]-=0.05;
        cout << "Viento: [x:" << viento[0] << ", z: " << viento[2]<< "]" << endl;
    }
    if (key==GLUT_KEY_DOWN){ /// aumenta viento hacia +z
        viento[2]+=0.05;
        cout << "Viento: [x:" << viento[0] << ", z: " << viento[2]<< "]" << endl;
    }
    if (key==GLUT_KEY_LEFT){ /// aumenta viento hacia -x
        viento[0]-=0.05;
        cout << "Viento: [x:" << viento[0] << ", z: " << viento[2]<< "]" << endl;
    }
    if (key==GLUT_KEY_RIGHT){ /// aumenta viento hacia +x
        viento[0]+=0.05;
        cout << "Viento: [x:" << viento[0] << ", z: " << viento[2]<< "]" << endl;
    }
    if (key==GLUT_KEY_PAGE_UP||key==GLUT_KEY_PAGE_DOWN){ // velocidad
        if (key==GLUT_KEY_PAGE_DOWN) ms_i++;
        else ms_i--;
        if (ms_i<0) ms_i=0; if (ms_i==ms_n) ms_i--;
        msecs=ms_lista[ms_i];
        if (cl_info){
            if (msecs<1000)
                cout << 1000/msecs << "fps" << endl;
            else
                cout << msecs/1000 << "s/frame)" << endl;
        }
    }
}
// Maneja pulsaciones del teclado (ASCII keys)
void Keyboard_cb(unsigned char key,int x=0,int y=0) {
    switch (key){
    case 'b': // alterna textura
        texmode=!texmode;
        if (texmode) {
            if (blend){
                glEnable(GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texmodelist[1]);
                if (cl_info) cout << "Blending Mode: " << texmodestring[1] << endl;
            }
            else {
                glEnable(GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texmodelist[0]);
                if (cl_info) cout << "Blending Mode: " << texmodestring[0] << endl;
            }
        }
        else {
            glDisable(GL_TEXTURE_2D);
            if (cl_info) cout << "Blending Mode: " << texmodestring[2] << endl;
        }
        break;
    case 'f': case 'F': // relleno
        relleno=!relleno;
        if (cl_info) cout << ((relleno)? "Relleno" : "Sin Relleno") << endl;
        break;
    case 's': case 'S': // para o comienza lluvia
        llueve=!llueve;
        if (cl_info) cout << ((llueve)? "Lluvia activada" : "Lluvia desactivada") << endl;
        break;
    case 'i': case 'I': // info
        cl_info=!cl_info;
        cout << ((cl_info)? "Info" : "Sin Info") << endl;
        break;
    case 'l': case 'L': // wire
        wire=!wire;
        if (cl_info) cout << ((wire)? "Lineas" : "Sin Lineas") << endl;
        break;
    case 'p': case 'P':  // perspectiva
        perspectiva=!perspectiva;
        if (cl_info) cout << ((perspectiva)? "Perspectiva" : "Ortogonal") << endl;
        regen();
        break;
    case 'r': case 'R': // rotacion
        rota=!rota;
        if (cl_info) cout << ((rota)? "Gira" : "No Gira") << endl;
        break;
    case 't': case 'T': // transparencia
        blend=!blend;
        if (blend){
            glEnable(GL_BLEND);
            if(texmode) {
                glEnable(GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texmodelist[1]);
            }
            else {
                glDisable(GL_TEXTURE_2D);
            }
            if (cl_info) cout << "Transparencia" << endl;
        }
        else {
            glDisable(GL_BLEND);
            if(texmode) {
                glEnable(GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texmodelist[0]);
            }
            else {
                glDisable(GL_TEXTURE_2D);
            }
            if (cl_info) cout << "Sin Transparencia" << endl;
        }
        break;
    case '+': /// aumenta velocidad
        if (slowdown>=200){
            slowdown-=100;
        }
        if (cl_info) cout << "Slowdown: " << slowdown << endl;
        break;
    case '-': /// disminuir velocidad
        
        if (slowdown<=2900) {
            slowdown+=100;
        }
        if (cl_info) cout << "Slowdown: " << slowdown << endl;
        break;
    case 27: // escape => exit
        get_modifiers();
        if (!modifiers)
            exit(EXIT_SUCCESS);
        break;
    }
    if (rota||llueve) glutIdleFunc(Idle_cb); // no llama a cada rato a esa funcion
    else {
        glutIdleFunc(0); // registra el callback
    }
    
    glutPostRedisplay();
}

// Menu
void Menu_cb(int value) {
    if (value<256) Keyboard_cb(value);
    else Special_cb(256-value);
}

// pregunta a OpenGL por el valor de una variable de estado
int integerv(GLenum pname){
    int value;
    glGetIntegerv(pname,&value);
    return value;
}

#define _PRINT_INT_VALUE(pname) #pname << ": " << integerv(pname) <<endl

extern void initParticlesDrops(int i);
extern void initParticlesFloor(int i);

// Inicializa GLUT y OpenGL
void initialize() {
    // pide z-buffer, color RGBA y double buffering
    glutInitDisplayMode(GLUT_DEPTH|GLUT_RGBA|GLUT_DOUBLE);
    
    glutInitWindowSize(w,h); glutInitWindowPosition(50,50);
    
    glutCreateWindow("Lluvia"); // crea el main window
    
    //declara los callbacks
    //los que no se usan no se declaran
    glutDisplayFunc(Display_cb); // redisplays
    glutReshapeFunc(Reshape_cb); // cambio de alto y ancho
    glutKeyboardFunc(Keyboard_cb); // teclado
    glutSpecialFunc(Special_cb); // teclas especiales
    glutMouseFunc(Mouse_cb); // botones picados
    if (!(dibuja)) glutIdleFunc(0); // no llama a cada rato a esa funcion
    else glutIdleFunc(Idle_cb); // registra el callback
    
    // crea el menu
    glutCreateMenu(Menu_cb);
    glutAddMenuEntry("     [s]_Iniciar luvia         ", 's');
    glutAddMenuEntry("     [+]_Aumentar velocidad    ", '+');
    glutAddMenuEntry("     [-]_Disminuir velocidad   ", '-');
    glutAddMenuEntry("     [b]_Textura               ", 'b');
    glutAddMenuEntry("     [f]_Caras Rellenas        ", 'f');
    glutAddMenuEntry("     [i]_Info ON/OFF           ", 'i');
    glutAddMenuEntry("     [l]_Lineas                ", 'l');
    glutAddMenuEntry("     [p]_Perspectiva/Ortogonal ", 'p');
    glutAddMenuEntry("     [r]_Rota                  ", 'r');
    glutAddMenuEntry("     [t]_Transparencia         ", 't');
    glutAddMenuEntry("    [Up]_Viento hacia -z       ", (256+GLUT_KEY_UP));
    glutAddMenuEntry("  [Down]_Viento hacia +z       ", (256+GLUT_KEY_DOWN));
    glutAddMenuEntry("  [Left]_Viento hacia -x       ", (256+GLUT_KEY_LEFT));
    glutAddMenuEntry(" [Right]_Viento hacia +x       ", (256+GLUT_KEY_RIGHT));
    glutAddMenuEntry("  [PgUp]_Aumenta Framerate     ", (256+GLUT_KEY_PAGE_UP));
    glutAddMenuEntry("  [Pgdn]_Disminuye Framerate   ", (256+GLUT_KEY_PAGE_DOWN));
    glutAddMenuEntry("   [Esc]_Exit                  ", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    // ========================
    // estado normal del OpenGL
    // ========================
    
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); // habilita el z-buffer
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset (1,1); // coplanaridad
    
    // interpola normales por nodos o una normal por plano
    glShadeModel(GL_SMOOTH);
    
    // ancho de lineas
    glLineWidth(linewidth);
    
    // antialiasing
    glDisable(GL_LINE_SMOOTH); glDisable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
    
    // transparencias
    if (blend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    // color de fondo
    glClearColor(0,0,0.03,0);
    
    // direccion de los poligonos
    glFrontFace(GL_CCW); glPolygonMode(GL_FRONT_AND_BACK,GL_POLYGON_SMOOTH);
    glDisable(GL_CULL_FACE);
    
    // textura
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    mipmap_ppm("checker.ppm");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    if (texmode<2){
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texmodelist[texmode]);
    }
    else
        glDisable(GL_TEXTURE_2D);
    
    // ========================
    // info
    if (cl_info)
        cout << "Vendor:         " << glGetString(GL_VENDOR) << endl
        << "Renderer:       " << glGetString(GL_RENDERER) << endl
        << "GL_Version:     " << glGetString(GL_VERSION) << endl
        << "GLU_Version:    " << gluGetString(GLU_VERSION) << endl
        << " "<< endl
        << " "<< endl
        << "Integrantes:  "<< endl
        << " Cordoba Martin, Kalafatic Emiliano "<< endl
        << " "<< endl
        << "Sistema de representacion de lluvia:"<<endl
        <<" "<<endl
        ;
    // ========================
    
    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES_DROPS; i++) {
        initParticlesDrops(i);
    }
    for (int i = 0; i < MAX_PARTICLES_FLOOR; i++) {
        initParticlesFloor(i);
    }
    
    regen(); // para que setee las matrices antes del 1er draw
}

//------------------------------------------------------------
// main
int main(int argc,char** argv) {
    glutInit(&argc,argv);// inicializa glut
    initialize(); // condiciones iniciales de la ventana y OpenGL
    glutMainLoop(); // entra en loop de reconocimiento de eventos
    return 0;
}
