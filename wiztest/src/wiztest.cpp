#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <GL/glut.h>

#include <string>
#include <sstream>
#include <functional>
#include <vector>

#include "common/util.hpp"
#include "common/PlanckTicker.hpp"
#include "common/open_gl_renderable.hpp"
#include "wiztest/src/wiz.hpp"

using namespace std;
using namespace evo;

void sighandler (int signum)
{
  //switch (signum) {
  //}

  cerr << "Caught signal " << signum << ", exiting\n";

  exit(1);
}

static const int LISTNO_ORIGTEST = 1;
static const int LISTNO_T1 = 10;

static const int kWizNGlSphereSlices = 20;
static const int kWizNGlSphereStacks = 20;

GLUquadricObj* qobj = nullptr;

int g_win_1 = -1;
//int submenu1, submenu2;

float g_thetime = 0.0;

vector<Wiz> g_wizzes;


void display ()
{
  static int count = 0;
  ++count;

  time_t timestart = 0;
  time(&timestart);

  char* timestr = ctime(&timestart);
  timestr[strlen(timestr)-1] = '\0';

  g_wizzes.emplace_back(Coords3(0, 0, 5));
  g_wizzes.emplace_back(Coords3(0, 5, 15));
  g_wizzes.emplace_back(Coords3(20, 9, 16));

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (int i = 0; i < g_wizzes.size(); ++i)
  {
    Wiz& wiz = g_wizzes[i];

    glPushMatrix();

    glTranslatef(wiz.pos().x, wiz.pos().y, wiz.pos().z);
    gluSphere(qobj,
              wiz.radius(), // radius
              kWizNGlSphereSlices,  // slices
              kWizNGlSphereStacks); // stacks

    glPopMatrix();
  }

  //glCallList(LISTNO_T1);

  //if (glutGetWindow() == g_win_1) {
  //  glCallList(list);   /* render sphere display list */
  //} else {
  //  glCallList(1);      /* render sphere display list */
  //}
  glutSwapBuffers();

  printf("%s _ display() # %d\n", timestr, (int)count);
}

void init ()
{
  gluQuadricDrawStyle(qobj, GLU_FILL);
/*
  glNewList(LISTNO_ORIGTEST, GL_COMPILE);  // create sphere display list
  glTranslatef(wiz.pos.x, wiz.pos.y, wiz.pos.z);
  gluSphere(qobj,
            1.0, // radius
            20,  // slices
            20); // stacks
  glEndList();
*/
  GLfloat light_diffuse[]  = { 1.0, 1.0, 0.0, 1.0 };
  GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
                  /* aspect ratio */ 1.0,
                  /* Z near */ 1.0,
                  /* Z far */  200.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 1,  /* eye is at (0,0,5) */
            0.0, 0.0, 0.0,      /* center is at (0,0,0) */
            0.0, 1.0, 0.0);      /* up is in positive Y direction */
  glTranslatef(0.0, 0.0, -30.0);
  //qobj->
}

int main (int argc, char** argv)
{
  const Duration real_time_per_evo_tick = Duration::FromMilliseconds(100);

  signal(SIGINT,  sighandler);
  signal(SIGTERM, sighandler);

  EvoUniverse universe ();

  PlanckTicker uclock ( real_time_per_evo_tick,
      std::bind(&EvoUniverse::TickHandler, &universe,
          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) );

  qobj = gluNewQuadric();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  g_win_1 = glutCreateWindow("sphere");
  //glutEntryFunc(enter_leave);
  init();
  glutDisplayFunc(display); //display_g_win_1);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

void idle ()
{
  GLfloat light_position [] = {1.0, 1.0, 1.0, 0.0};

  glutSetWindow(g_win_1);
  g_thetime += 0.05;
  light_position[1] = 1 + sin(g_thetime);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  display();
}

/* ARGSUSED */
void delayed_stop (int value)
{
  glutIdleFunc(NULL);
}

void it (int value)
{
  glutDestroyWindow(glutGetWindow());
  printf("menu selection: win=%d, menu=%d\n", glutGetWindow(), glutGetMenu());
  switch (value) {
  case 1:
  /*
    if (list == 1) {
      list = 2;
    } else {
      list = 1;
    }
    */
    break;
  case 2:
    exit(0);
    break;
  case 3:
    glutAddMenuEntry("new entry", value + 9);
    break;
  case 4:
    glutChangeToMenuEntry(1, "toggle it for drawing", 1);
    glutChangeToMenuEntry(3, "motion done", 3);
    glutIdleFunc(idle);
    break;
  case 5:
    glutIdleFunc(NULL);
    break;
  case 6:
    glutTimerFunc(2000, delayed_stop, 0);
    break;
  default:
    printf("value = %d\n", value);
  }
}

void
menustate(int inuse)
{
  printf("menu is %s\n", inuse ? "INUSE" : "not in use");
  if (!inuse) {
  }
}

void
keyboard(unsigned char key, int x, int y)
{
  if (isprint(key)) {
    printf("key: `%c' %d,%d\n", key, x, y);
  } else {
    printf("key: 0x%x %d,%d\n", key, x, y);
  }
}

void
special(int key, int x, int y)
{
  bool redraw = false;
  string name;

  switch (key) {
  case GLUT_KEY_F1:
    name = "F1";
    break;
  case GLUT_KEY_F2:
    name = "F2";
    break;
  case GLUT_KEY_F3:
    name = "F3";
    break;
  case GLUT_KEY_F4:
    name = "F4";
    break;
  case GLUT_KEY_F5:
    name = "F5";
    break;
  case GLUT_KEY_F6:
    name = "F6";
    break;
  case GLUT_KEY_F7:
    name = "F7";
    break;
  case GLUT_KEY_F8:
    name = "F8";
    break;
  case GLUT_KEY_F9:
    name = "F9";
    break;
  case GLUT_KEY_F10:
    name = "F10";
    break;
  case GLUT_KEY_F11:
    name = "F11";
    break;
  case GLUT_KEY_F12:
    name = "F12";
    break;
  case GLUT_KEY_LEFT:
    name = "Left: move sphere -X";
    //wiz.pos().x -= 1;
    redraw = true;
    break;
  case GLUT_KEY_UP:
    name = "Up: move sphere +Y";
    //wiz.pos().y += 1;
    redraw = true;
    break;
  case GLUT_KEY_RIGHT:
    name = "Right: move sphere +X";
    //wiz.pos().x += 1;
    redraw = true;
    break;
  case GLUT_KEY_DOWN:
    name = "Down: move sphere -Y";
    //wiz.pos().y -= 1;
    redraw = true;
    break;
  case GLUT_KEY_PAGE_UP:
    name = "Page up";
    break;
  case GLUT_KEY_PAGE_DOWN:
    name = "Page down";
    break;
  case GLUT_KEY_HOME:
    name = "Home: move sphere +Z";
    //wiz.pos().z += 1;
    redraw = true;
    break;
  case GLUT_KEY_END:
    name = "End: move sphere -Z";
    //wiz.pos().z -= 1;
    redraw = true;
    break;
  case GLUT_KEY_INSERT:
    name = "Insert";
    break;
  default:
    name = "UNKNOWN";
    break;
  }
  printf("special: %s %d,%d; wiz = %s\n", name.c_str(), x, y, "undefined");// wiz.ToString().c_str());

  if (redraw) {
    display();
  }
}

void
mouse(int button, int state, int x, int y)
{
  printf("button: %d %s %d,%d\n", button, state == GLUT_UP ? "UP" : "down", x, y);
}

void
motion(int x, int y)
{
  printf("motion: %d,%d\n", x, y);
}

void
visible(int status)
{
  printf("visible: %s\n", status == GLUT_VISIBLE ? "YES" : "no");
}

void
enter_leave(int state)
{
  printf("enter/leave %d = %s\n",
    glutGetWindow(),
    state == GLUT_LEFT ? "left" : "entered");
}
