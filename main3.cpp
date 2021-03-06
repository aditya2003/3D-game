// Credits www.videotutorialsrock.com
/* Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* File for "Terrain" lesson of the OpenGL tutorial on
 * www.videotutorialsrock.com
 */


#include <cmath>
#include <iostream>
#include <stdlib.h>
#include <vector>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "glm.cpp"
#include "imageloader.cpp"
#include "vec3f.cpp"

#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)
#define RAD2DEG(rad) (rad * 180 / PI)

using namespace std;
GLdouble projection[16], modelview[16];

struct Ball {
	
	float pos[3]; //Position
	float r; //Radius
	float color[3];
	int marked;
};
vector<Ball*> _balls ;
typedef struct _cell {
    int id;
    int x, y;
    float min, max;
    float value;
    float step;
    char* info;
    char* format;
} cell;


//Gloabal variables
int current_view=3,pause_scene=1,game_start_flag=1,game_over_flag=0,score=0,game_time=100,enable=1;
float pitch=0,roll=0,thrust=0.2,accn=0,vel=0,prev_temp;

cell translation[3] = {
    { 1, 120, 40, -5.0, 5.0, 0.0, 0.01,
        "Specifies X coordinate of translation vector.", "%.2f" },
    { 2, 180, 40, -5.0, 5.0, 0.0, 0.01,
    "Specifies Y coordinate of translation vector.", "%.2f" },
    { 3, 240, 40, -5.0, 5.0, 0.0, 0.01,
    "Specifies Z coordinate of translation vector.", "%.2f" },
};

cell rotation[4] = {
    { 4, 120, 80, -360.0, 360.0, 0.0, 1.0,
        "Specifies angle of rotation, in degrees.", "%.1f" },
    { 5, 180, 80, -1.0, 1.0, 0.0, 0.01,
    "Specifies X coordinate of vector to rotate about.", "%.2f" },
    { 6, 240, 80, -1.0, 1.0, 1.0, 0.01,
    "Specifies Y coordinate of vector to rotate about.", "%.2f" },
    { 7, 300, 80, -1.0, 1.0, 0.0, 0.01,
    "Specifies Z coordinate of vector to rotate about.", "%.2f" },
};

cell scale[3] = {
    {  8, 120, 120, -5.0, 5.0, 1.0, 0.01,
        "Specifies scale factor along X axis.", "%.2f" },
    {  9, 180, 120, -5.0, 5.0, 1.0, 0.01,
    "Specifies scale factor along Y axis.", "%.2f" },
    { 10, 240, 120, -5.0, 5.0, 1.0, 0.01,
    "Specifies scale factor along Z axis.", "%.2f" },
};




GLfloat eye[3] = { 0.0, 0.0, 2.0 };
GLfloat at[3]  = { 0.0, 0.0, 0.0 };
GLfloat up[3]  = { 0.0, 1.0, 0.0 };


/*--------------------------------------------------------------------------*/


GLuint _textureId; //The id of the texture


//Makes the image into a texture, and returns the id of the texture
GLuint loadTexture(Image* image) {
	GLuint textureId;
	glGenTextures(1, &textureId); //Make room for our texture
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
				 0,                            //0 for now
				 GL_RGB,                       //Format OpenGL uses for image
				 image->width, image->height,  //Width and height
				 0,                            //The border of the image
				 GL_RGB, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
				                   //as unsigned numbers
				 image->pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}
/*--------------------------------------------------------------------------*/



//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
	private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) {
			w = w2;
			l = l2;
			
			hs = new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
			}
			
			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}
			
			computedNormals = false;
		}
		
		~Terrain() {
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
			}
			delete[] hs;
			
			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}
		
		int width() {
			return w;
		}
		
		int length() {
			return l;
		}
		
		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];
					
					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
};
Terrain* _terrain;



void drawBitmapText(char *string,float x,float y,float z) {
 char *c; glRasterPos3f(x, y,z); 
for (c=string; *c != '\0'; c++) 
{ glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c); 
}
 }






void create_ball()
{
	for(int z = 0; z < _terrain->length() - 1; z++) {
		
		for(int x = 0; x < _terrain->width(); x++) {

//			x, _terrain->getHeight(x, z), z 


		if ((rand() % 5000)==1)
		{

			Ball* ball = new Ball();
				
				ball->pos[0] = x;
				ball->pos[1] =_terrain->getHeight(x, z);
				ball->pos[2] = z;
				
				ball->r = 0.5f;
				ball->marked=0;
				int color_code = rand() % 3;
				if (color_code==0)//red
				{
				ball->color[0] = 1.0f;
				ball->color[1] = 0.0f;
				ball->color[2] = 0.0f;
				}
				else if (color_code==1) // green
				{
				ball->color[0] = 0.0f;
				ball->color[1] = 1.0f;
				ball->color[2] = 0.0f;	
				}
				else
				{
				ball->color[0] = 0.0f;
				ball->color[1] = 0.0f;
				ball->color[2] = 0.0f;	
				}


				_balls.push_back(ball);



		}	
		

		}

	}




}

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for(int y = 0; y < image->height; y++) {
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);


		}
	}
	
	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;


void cleanup() {
	delete _terrain;
}

void change_camera()
{
if(current_view==0) //helicopter
{
			eye[0] =  translation[0].value -  10*sin(DEG2RAD(rotation[0].value));
 			 eye[1] = translation[1].value + 10;
			 eye[2] =  translation[2].value -  10*cos(DEG2RAD(rotation[0].value));
			 at[0]  =  translation[0].value + 4*sin(DEG2RAD(rotation[0].value));
			 at[1]  =    translation[1].value + 0;
			 at[2]  =  (translation[2].value +  4*cos(DEG2RAD(rotation[0].value)));
}
else if (current_view==1) // front wheel
{
	eye[0] =  translation[0].value;
 			 eye[1] = translation[1].value + 2 ;
			 eye[2] =  translation[2].value ;
			 at[0]  =  translation[0].value + 5*sin(DEG2RAD(rotation[0].value));
			 at[1]  =    translation[1].value + 2;
			 at[2]  =  (translation[2].value +  5*cos(DEG2RAD(rotation[0].value)));

}
else if (current_view==2) // driver
{
			eye[0] =  translation[0].value - 2.8*sin(DEG2RAD(rotation[0].value));
 			 eye[1] = translation[1].value +1;
			 eye[2] =  translation[2].value - 2.8*cos(DEG2RAD(rotation[0].value));
			 at[0]  =  translation[0].value + 2*sin(DEG2RAD(rotation[0].value));
			 at[1]  =    translation[1].value +2;
			 at[2]  =  (translation[2].value +  4*cos(DEG2RAD(rotation[0].value)));

}
else if (current_view==3) // overhead
{
			eye[0] =  translation[0].value -  10*sin(DEG2RAD(rotation[0].value));
 			 eye[1] = translation[1].value + 10;
			 eye[2] =  translation[2].value -  12*cos(DEG2RAD(rotation[0].value));
			 at[0]  =  translation[0].value + 10*sin(DEG2RAD(rotation[0].value));
			 at[1]  =    translation[1].value + 0;
			 at[2]  =  (translation[2].value +  10*cos(DEG2RAD(rotation[0].value)));
}
else if (current_view==4) // chase
{
			eye[0] =  translation[0].value - 5*sin(DEG2RAD(rotation[0].value));
 			 eye[1] = translation[1].value +1;
			 eye[2] =  translation[2].value - 10*cos(DEG2RAD(rotation[0].value));
			 at[0]  =  translation[0].value + 5*sin(DEG2RAD(rotation[0].value));
			 at[1]  =    translation[1].value +2;
			 at[2]  =  (translation[2].value +  5*cos(DEG2RAD(rotation[0].value)));
}



}




void handleKeypress(unsigned char key, int x, int y) 
{
	switch (key) 
		{


		case 97: // a - roll
			rotation[0].value+=10;
			break;

		case 100: // d - roll
			rotation[0].value-=10;
			break;
		case 104:
			if(enable==0)
				enable=1;
			else enable = 0;
			break;


		


		case 118: //v - change view
			current_view =(current_view + 1)%5;
			change_camera();
			

 
			break;

			




		case 27: //Escape key
			cleanup();
			exit(0);
			break;


		case 112:
		

		
			if(pause_scene==0)
				pause_scene=1;
			else pause_scene=0;
			break;

		
		case 113:
			cleanup();
			exit(0);     // q key is pressed
			break;
		

		case 13:
			
		      if(game_start_flag==1)
				{
					game_start_flag=0;
					pause_scene=0;
					
				}
			break;	
			
			
    
		}



	
}


void handleKeypress2(int key, int x, int y) 
{
if (pause_scene==0)
{

    if (key == GLUT_KEY_UP)
	{
		vel+=thrust;		


	}
if (key == GLUT_KEY_DOWN)
	{
		vel-=thrust;


       	
	}
    if (key == GLUT_KEY_RIGHT)
        {

		
		rotation[0].value-=3;
		if(rotation[0].value<0)
			{
				rotation[0].value = 360 + rotation[0].value;
			}
		rotation[1].value = 0.0;
	        rotation[2].value = 1.0;
	        rotation[3].value = 0.0;
		roll += 2;


	}
if (key == GLUT_KEY_LEFT)
        {

		rotation[0].value+=3;
		if(rotation[0].value>360)
			{
				rotation[0].value =  rotation[0].value - 360;
			}
		rotation[1].value = 0.0;
	        rotation[2].value = 1.0;
	        rotation[3].value = 0.0;
		roll -= 2;

	}

}
}


void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);


	Image* image = loadBMP("grass1.bmp");
	_textureId = loadTexture(image);
	delete image;

}

void handleResize(int width, int height) {
/*	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);*/

glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)width/height, 1.0, 200.0);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  // gluLookAt(eye[0], eye[1], eye[2], at[0], at[1], at[2], up[0], up[1],up[2]);
   
    /*glTranslatef(translation[0].value, translation[1].value,
            translation[2].value);
        glRotatef(rotation[0].value, rotation[1].value, 
    rotation[2].value, rotation[3].value);
    
    glScalef(scale[0].value, scale[1].value, scale[2].value);*/
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glClearColor(0.2, 0.2, 0.2, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);


}

GLMmodel* pmodel = NULL;
void
drawmodel(void)
{
    if (!pmodel) {
        pmodel = glmReadOBJ("Bike.obj");
        if (!pmodel) exit(0);
        glmUnitize(pmodel);
        glmFacetNormals(pmodel);
        glmVertexNormals(pmodel, 90.0);
    }
    
    glmDraw(pmodel, GLM_SMOOTH | GLM_MATERIAL);
}
int temp=0;
void drawScene() {


if(temp==0)
{create_ball();
temp=1;
}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, 0.0f);
	//glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
gluLookAt(eye[0], eye[1], eye[2], at[0], at[1], at[2], up[0], up[1],up[2]);	



	glColor3f(0.3f, 0.9f, 0.0f);	
	GLfloat ambientColor[] = {0.3f, 0.3f, 0.3f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
	
//	GLfloat lightColor0[] = {0.5f, 0.5f, 0.0f, 1.0f};
	GLfloat lightColor0[] = {0.4f, 0.4f, 0.0f, 1.0f};
	GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	
	//float scale = 5.0f / max(_terrain->width() - 1, _terrain->length() - 1);
	//glScalef(scale, scale, scale);
	//glTranslatef(-(float)(_terrain->width() - 1) / 2,
	//			 0.0f,
	//			 -(float)(_terrain->length() - 1) / 2);
	

glPushMatrix();

	GLfloat lightPos2[]={ 100.0 , 60.0 , 80.0, 1.0};
//	GLfloat dirVector2[]={ 100.0, -5.0, 100.0, 1.0};
	GLfloat dirVector2[]={ translation[0].value - 100, translation[1].value - 60,translation[2].value- 80.0, 1.0};
	GLfloat ambientLight2[] = {1.0f, 0.0f, 0.0f, 1.0f};
	GLfloat diffuseLight2[] = {1.0f, 0.0f, 0.0f, 1.0f};
//	GLfloat specLight2[] = {0.7f,0.7f,0.7f,1.0f};

	glLightfv(GL_LIGHT2, GL_AMBIENT, ambientLight2);
//	glLightfv(GL_LIGHT2,GL_SPECULAR, specLight2);
glLightfv(GL_LIGHT2, GL_DIFFUSE, diffuseLight2);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPos2);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF,1.0);
	glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, dirVector2);
	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 10);
glEnable(GL_LIGHT2);
	glPushMatrix(); 
	glTranslatef(100.0,10.0,100.0);
	glColor4f(0.5f,0.5f,0.5f,1.0f);
	//glutSolidSphere(2.0,60,60);
	glPopMatrix();

	glPopMatrix();


	
//--------------------------------------------------------------------------//

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _textureId);
	
	//Bottom
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glColor3f(1.0f, 1.0f, 1.0f);
	
	//glColor3f(0.3f, 0.9f, 0.0f);
	for(int z = 0; z < _terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);

		for(int x = 0; x < _terrain->width(); x++) {
			if(z>160 && z<170)
			{
				glColor3f(0.0f,0.0f,1.0f);
			}
			else if(_terrain->getHeight(x,z) > 5 )
			{
				glColor3f(1.0f,1.0f,1.0f);
			}
			else
			{
				glColor3f(1.0f, 1.0f, 1.0f);
	
				//glColor3f(0.3f, 0.9f, 0.0f);
			}
			Vec3f normal = _terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(x, _terrain->getHeight(x, z), z);
			normal = _terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glTexCoord2f(5.0f, 5.0f);		
			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

//--------------------------------------------------------------------------//

/*
	glColor3f(0.3f, 0.9f, 0.0f);

	for(int z = 0; z < _terrain->length() - 1; z++) {
if(z < (_terrain->length() - 1)/2)
glColor3f(0.0f, 0.3f, 0.1f);
else if(z < 2*(_terrain->length() - 1)/3)
glColor4f(0.0f, 0.0f, 0.7f,0.5f);
else
glColor3f(0.0f, 0.3f, 0.1f);
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for(int x = 0; x < _terrain->width(); x++) {

			if(_terrain->getHeight(x, z) > 13 )
				glColor3f(1.0f,1.0f,1.0f);
			Vec3f normal = _terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z), z);
			normal = _terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
		
			}
		glEnd();
	}
*/
glPushMatrix();   
//154-205-50
glColor3f(0.0,0.0,1.0);
char ar3[25];
char ar7[25];

char ar2[]="Score: ";
char ar4[]="Crashed!! Game over!\nPress Q to Quit!";
char ar8[]="Time up!! Game over!\nPress Q to Quit!";

char ar5[]="Press Enter to start the game!";
char ar6[]="Time: ";

    glRasterPos3f(translation[0].value, 1.0f,translation[2].value); 

 sprintf(ar3,"           %d",score);
sprintf(ar7,"           %d",game_time);  


int x_x = (glutGet(GLUT_SCREEN_WIDTH)/100) - 2,z_z= glutGet(GLUT_SCREEN_WIDTH)/50 ;

int temp_int = 10;
drawBitmapText(ar2,translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,_terrain->getHeight(translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z) + 1 ,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z);

drawBitmapText(ar3,translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,_terrain->getHeight(translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z) + 1 ,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z);
if(game_over_flag!=1)
{
if(game_time<=20)
glColor3f(0.5f, 0.0f, 0.0f);
drawBitmapText(ar6,translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,_terrain->getHeight(translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z) + 2.5 ,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z);
drawBitmapText(ar7,translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,_terrain->getHeight(translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z) + 2.5 ,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z);
}



  if(game_over_flag==1)
    {
	glColor3f(1.0f, 0.0f, 0.0f);
if(game_time<=0)
	drawBitmapText(ar8,translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,_terrain->getHeight(translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z) + 2.5 ,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z);

else

	drawBitmapText(ar4,translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,_terrain->getHeight(translation[0].value +  temp_int*sin(DEG2RAD(rotation[0].value)) + x_x,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z) + 2.5 ,translation[2].value  +  temp_int*cos(DEG2RAD(rotation[0].value)) + z_z);

	}
if(game_start_flag==1)
    {
	glColor3f(0.7f, 0.0f, 0.0f);
drawBitmapText(ar5,14,_terrain->getHeight(14,3),3);

	}






 
/* checking height of next point
float tempx = translation[2].value + vel * 0.1 * cos(DEG2RAD(rotation[0].value));
float tempy = translation[0].value + vel * 0.1 * sin(DEG2RAD(rotation[0].value));
float next_height = _terrain->getHeight(int(tempx),int(tempy));*/
float current_height = _terrain->getHeight(int(translation[0].value) ,int(translation[2].value))  +1; ;

if(prev_temp > current_height + 0.1)
{

//translation[1].value = current_height + (current_height*0.1);
translation[1].value = prev_temp ;

pitch = 0;

}
else
{
translation[1].value = current_height; 

}

prev_temp = _terrain->getHeight(int(translation[0].value) ,int(translation[2].value))  +1; 


        glTranslatef(translation[0].value,translation[1].value  ,
            translation[2].value);
        glRotatef(rotation[0].value, rotation[1].value, 
            rotation[2].value, rotation[3].value);
	glRotatef(-pitch,1,0,0);
	glRotatef(roll,0,0,1);

    //glScalef(scale[0].value, scale[1].value, scale[2].value);


  glPushMatrix();   
GLfloat light1_ambient[] = { 1.0, 1.0, 0.0, 1.0 };
			GLfloat light1_diffuse[] = { 1.0, 1.0, 0.0, 1.0 };
			GLfloat light1_specular[] = { 1.0, 1.0, 0.0, 1.0 };
			GLfloat light1_position[] = { 0.0, 0.0, 5.0, 1.0 };
			GLfloat spot_direction[] = { 1.0, 0.0, 1.0 };

			glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
			glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
			glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
			glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
			glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.1);
			glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.1);
			glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.1);

			glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 90.0);
			glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spot_direction);
			glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 2.0);
			if(enable==1)
			glEnable(GL_LIGHT1);
			else glDisable(GL_LIGHT1);
			
glPopMatrix();   


	glColor3f(0.0f, 0.2f, 0.0f);	
	drawmodel();

glPopMatrix();


for(unsigned int i = 0; i < _balls.size(); i++) {
		Ball* ball = _balls[i];
		glPushMatrix();
		glTranslatef(ball->pos[0], ball->pos[1], ball->pos[2]);
		if(ball->color[0]==0 && ball->color[1]==0 && ball->color[2]==0)
			glColor3f(0.0, 0.0, 0.0);
		else if(ball->color[1]==1)
						glColor3f(1.0, 0.0, 0.0);
		else 			glColor3f(1.0, 1.0, 0.0);
		glutSolidSphere(ball->r,
                    50,50);
				glPopMatrix();

	}


	
	glutSwapBuffers();
}

void update(int value) {
if(pause_scene==0)
{

if(game_time<=0)
{
  game_over_flag=1;
pause_scene=1;
}


game_time--;

for(unsigned int i = 0; i < _balls.size(); i++) {
			Ball* ball = _balls[i];
	if(abs(ball->pos[0]-translation[0].value)<2  && abs(ball->pos[1]-translation[1].value)<2 && abs(ball->pos[2]-translation[2].value)<2)
		{		_balls.erase (_balls.begin()+i);
			score++;
			game_time+=25;
		}
	}



if(abs(roll)>25)
{
  game_over_flag=1;
pause_scene=1;
}



// Calculation of pitch and roll
Vec3f normal_new = _terrain->getNormal(translation[0].value, translation[2].value);
float theta = acos(normal_new[1]/sqrt( (pow(normal_new[0],2)) + (pow(normal_new[1],2)) + (pow(normal_new[2],2))));
theta = RAD2DEG(theta);
pitch = theta;


accn -= 0.00005*sin(DEG2RAD(theta)) ;
vel += accn;


translation[2].value += vel * 0.1 * cos(DEG2RAD(rotation[0].value));
translation[0].value += vel * 0.1 * sin(DEG2RAD(rotation[0].value));
//printf("\nI was here\n %f %f %f \n\n",translation[0].value,translation[2].value,theta);


		change_camera();        	
	glutPostRedisplay();


}
	glutTimerFunc(25, update, 0);

}


int main(int argc, char** argv) {




 translation[0].value=0;
 translation[2].value=0;
rotation[0].value = 45;
prev_temp = -10;

change_camera();





	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(400, 400);
	
	glutCreateWindow("Terrain - videotutorialsrock.com");
	initRendering();
	
	_terrain = loadTerrain("height_map.bmp", 20);

	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutSpecialFunc(handleKeypress2);
	glutReshapeFunc(handleResize);
	


	glutTimerFunc(1000, update, 0);
	//glutTimerFunc(1000, update2, 0);


	glutMainLoop();
	
	

	return 0;
}









