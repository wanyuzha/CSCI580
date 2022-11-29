/* CS580 Homework 3 */

#include	"stdafx.h"
#include	"stdio.h"
#include	"math.h"
#include	"Gz.h"
#include	"rend.h"
#include	<limits>
#include <time.h>
long  start, mid, ending;

#define PI (float) 3.14159265358979323846
int GzRender::GzRotXMat(float degree, GzMatrix mat)
{
/* HW 3.1
// Create rotate matrix : rotate along x axis
// Pass back the matrix using mat value
*/
	GzMatrix rotX = { {1, 0, 0, 0},
						{0, cos(degree * PI / 180), -sin(degree * PI / 180), 0},
						{0, sin(degree * PI / 180), cos(degree * PI / 180), 0},
						{0, 0, 0, 1}
	};
	memcpy((void*)mat, (void*)rotX, sizeof(GzMatrix));
	return GZ_SUCCESS;
}

int GzRender::GzRotYMat(float degree, GzMatrix mat)
{
/* HW 3.2
// Create rotate matrix : rotate along y axis
// Pass back the matrix using mat value
*/
	GzMatrix rotY = { {cos(degree * PI / 180), 0, sin(degree * PI / 180), 0},
						{0, 1, 0, 0},
						{-sin(degree * PI / 180), 0, cos(degree * PI / 180), 0},
						{0, 0, 0, 1}
	};
	memcpy((void*)mat, (void*)rotY, sizeof(GzMatrix));
	return GZ_SUCCESS;
}

int GzRender::GzRotZMat(float degree, GzMatrix mat)
{
/* HW 3.3
// Create rotate matrix : rotate along z axis
// Pass back the matrix using mat value
*/
	GzMatrix rotZ = { {cos(degree * PI / 180), -sin(degree * PI / 180), 0, 0},
						{sin(degree * PI / 180), cos(degree * PI / 180), 0, 0},
						{0, 0, 1, 0},
						{0, 0, 0, 1}
	};
	memcpy((void*)mat, (void*)rotZ, sizeof(GzMatrix));
	return GZ_SUCCESS;
}

int GzRender::GzTrxMat(GzCoord translate, GzMatrix mat)
{
/* HW 3.4
// Create translation matrix
// Pass back the matrix using mat value
*/
	mat[0][3] = translate[0];
	mat[1][3] = translate[1];
	mat[2][3] = translate[2];
	return GZ_SUCCESS;
}


int GzRender::GzScaleMat(GzCoord scale, GzMatrix mat)
{
/* HW 3.5
// Create scaling matrix
// Pass back the matrix using mat value
*/
	mat[0][0] = mat[0][0] * scale[0];
	mat[1][1] = mat[1][1] * scale[1];
	mat[2][2] = mat[2][2] * scale[2];
	return GZ_SUCCESS;
}


GzRender::GzRender(int xRes, int yRes)
{
/* HW1.1 create a framebuffer for MS Windows display:
 -- set display resolution
 -- allocate memory for framebuffer : 3 bytes(b, g, r) x width x height
 -- allocate memory for pixel buffer
 */
	xres = xRes;
	yres = yRes;
	matlevel = 0;
	numlights = 0;
	framebuffer = (char*) malloc (3 * sizeof(char) * xRes * yRes);
	pixelbuffer = new GzPixel[xres * yres];
	trianglebuffer = new GzTri[MAX_TRIANGLES];
	triIndex = 0;

	/* Initializa root */
	root = new BSP_tree();
	root->order = 0;


/* HW 3.6
- setup Xsp and anything only done once 
- init default camera 
*/ 
	GzMatrix Xsp_t = { {xres / 2, 0, 0, xres / 2},
			{0, -yres / 2, 0, yres / 2},
			{0, 0, MAXINT, 0},
			{0, 0, 0, 1} };
	memcpy((void*)Xsp, (void*)Xsp_t, sizeof(GzMatrix));
	m_camera.FOV = DEFAULT_FOV;
	m_camera.lookat[0] = 0;
	m_camera.lookat[1] = 0;
	m_camera.lookat[2] = 0;
	m_camera.position[0] = DEFAULT_IM_X;
	m_camera.position[1] = DEFAULT_IM_Y;
	m_camera.position[2] = DEFAULT_IM_Z;
	m_camera.worldup[0] = 0;
	m_camera.worldup[1] = 1;
	m_camera.worldup[2] = 0;
}

GzRender::~GzRender()
{
/* HW1.2 clean up, free buffer memory */
	delete[] framebuffer;
	delete[] pixelbuffer;
}

int GzRender::GzDefault()
{
/* HW1.3 set pixel buffer to some default values - start a new frame */
	for (int i = 0; i < xres; i++)
	{
		for (int j = 0; j < yres; j++)
		{
			pixelbuffer[j * xres + i].alpha = 1;
			pixelbuffer[j * xres + i].red = 4095;
			pixelbuffer[j * xres + i].blue = 4095;
			pixelbuffer[j * xres + i].green = 4095;
			pixelbuffer[j * xres + i].z = 2147483647;
			pixelbuffer[j * xres + i].triangle = nullptr;
			pixelbuffer[j * xres + i].tValue = (float)MAXINT;
			pixelbuffer[j * xres + i].hitPoint[X] = 0;
			pixelbuffer[j * xres + i].hitPoint[Y] = 0;
			pixelbuffer[j * xres + i].hitPoint[Z] = 0;
		}
	}

	for (int i = 0; i < MAX_TRIANGLES; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				trianglebuffer[triIndex].screenVerts[j][k] = 0;
				trianglebuffer[triIndex].imageVerts[j][k] = 0;
			}
			for (int k = 0; k < 2; k++)
			{
				trianglebuffer[triIndex].uv[j][k] = 0;
			}
		}
	}
	return GZ_SUCCESS;
}

float GzCoordLength(GzCoord coord)
{
	return sqrt(coord[0] * coord[0] + coord[1] * coord[1] + coord[2] * coord[2]);
}

float GzDot(GzCoord coord1, GzCoord coord2)
{
	return coord1[0] * coord2[0] + coord1[1] * coord2[1] + coord1[2] * coord2[2];
}

int GzRender::GzBeginRender()
{
/* HW 3.7 
- setup for start of each frame - init frame buffer color,alpha,z
- compute Xiw and projection xform Xpi from camera definition 
- init Ximage - put Xsp at base of stack, push on Xpi and Xiw 
- now stack contains Xsw and app can push model Xforms when needed 
*/ 
	float d_reciprocal = tan(m_camera.FOV * PI / (180 * 2));
	GzMatrix Xpi_t = { {1, 0, 0, 0},{0, 1, 0, 0},{0, 0, d_reciprocal, 0},{0, 0, d_reciprocal, 1} };
	memcpy((void*)m_camera.Xpi, (void*)Xpi_t, sizeof(GzMatrix));
	GzCoord CL = { m_camera.lookat[0] - m_camera.position[0], m_camera.lookat[1] - m_camera.position[1], m_camera.lookat[2] - m_camera.position[2] };
	float lookatLength = GzCoordLength(CL);
	GzCoord Zaxis = { CL[0] / lookatLength, CL[1] / lookatLength, CL[2] / lookatLength };
	float up_dot = m_camera.worldup[0] * Zaxis[0] + m_camera.worldup[1] * Zaxis[1] + m_camera.worldup[2] * Zaxis[2];
	GzCoord up_prime = { m_camera.worldup[0] - up_dot * Zaxis[0], m_camera.worldup[1] - up_dot * Zaxis[1] ,m_camera.worldup[2] - up_dot * Zaxis[2] };
	float up_prime_length = GzCoordLength(up_prime);
	GzCoord Yaxis = { up_prime[0] / up_prime_length, up_prime[1] / up_prime_length, up_prime[2] / up_prime_length };
	GzCoord Xaxis = { Yaxis[1] * Zaxis[2] - Yaxis[2] * Zaxis[1], Yaxis[2] * Zaxis[0] - Yaxis[0] * Zaxis[2], Yaxis[0] * Zaxis[1] - Yaxis[1] * Zaxis[0] };
	GzMatrix Xiw_t = { {Xaxis[0], Xaxis[1], Xaxis[2], -GzDot(Xaxis,m_camera.position)},
					   {Yaxis[0], Yaxis[1], Yaxis[2], -GzDot(Yaxis,m_camera.position)},
					   {Zaxis[0], Zaxis[1], Zaxis[2], -GzDot(Zaxis,m_camera.position)},
					   {0, 0, 0, 1} };
	memcpy((void*)m_camera.Xiw, (void*)Xiw_t, sizeof(GzMatrix));

	GzPushMatrix(Xsp);
	GzPushMatrix(Xpi_t);
	GzPushMatrix(Xiw_t);

	/*MatrixMultiply(Xsp, Xpi_t, Xspi);
	MatrixMultiply(Xspi, Xiw_t, Xspiw);*/
	MatrixMultiply(Xiw_t, Xpi_t, Xspi);
	MatrixMultiply(Xspi, Xsp, Xspiw);

	return GZ_SUCCESS;
}

int GzRender::GzPutCamera(GzCamera camera)
{
/* HW 3.8 
/*- overwrite renderer camera structure with new camera definition
*/
	m_camera.FOV = camera.FOV;
	memcpy((void*)m_camera.lookat, (void*)camera.lookat, sizeof(GzCoord));
	memcpy((void*)m_camera.position, (void*)camera.position, sizeof(GzCoord));
	memcpy((void*)m_camera.worldup, (void*)camera.worldup, sizeof(GzCoord));
	return GZ_SUCCESS;	
}

int GzRender::GzPushMatrix(GzMatrix	matrix)
{
/* HW 3.9 
- push a matrix onto the Ximage stack
- check for stack overflow
*/
	if (matlevel == 0)
	{
		memcpy((void*)X2screen[0], (void*)matrix, sizeof(GzMatrix));

		// for Xnorm
		GzMatrix Xnorm_initial = { {1, 0, 0, 0},{0, 1, 0, 0},{0, 0, 1, 0},{0, 0, 0, 1} };
		memcpy((void*)Xnorm[0], (void*)Xnorm_initial, sizeof(GzMatrix));
		memcpy((void*)X2image[0], (void*)Xnorm_initial, sizeof(GzMatrix));
		// only do this after push matrix
		matlevel++;
	}
	else
	{
		GzMatrix temp;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				temp[i][j] = 0;
				for (int k = 0; k < 4; k++)
				{
					temp[i][j] += X2screen[matlevel - 1][i][k] * matrix[k][j];
				}
			}
		}
		memcpy((void*)X2screen[matlevel], (void*)temp, sizeof(GzMatrix));
		// do this for Xnorm, skip Xpi
		if (matlevel == 1)
		{
			GzMatrix Xnorm_initial = { {1, 0, 0, 0},{0, 1, 0, 0},{0, 0, 1, 0},{0, 0, 0, 1} };
			memcpy((void*)Xnorm[1], (void*)Xnorm_initial, sizeof(GzMatrix));
			memcpy((void*)X2image[1], (void*)Xnorm_initial, sizeof(GzMatrix));
		}
		else
		{
			MatrixMultiply(X2image[matlevel - 1], matrix, X2image[matlevel]);
			//unitary scale
			//only focused on 3*3 matrix
			float K = sqrt(matrix[0][0] * matrix[0][0] + matrix[0][1] * matrix[0][1] + matrix[0][2] * matrix[0][2]);
			GzMatrix Xnorm_initial = { {matrix[0][0] / K, matrix[0][1] / K, matrix[0][2] / K, 0},{matrix[1][0] / K, matrix[1][1] / K, matrix[1][2] / K, 0},{matrix[2][0] / K, matrix[2][1] / K, matrix[2][2] / K, 0},{0, 0, 0, 1} };
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					temp[i][j] = 0;
					for (int k = 0; k < 4; k++)
					{
						temp[i][j] += Xnorm[matlevel - 1][i][k] * Xnorm_initial[k][j];
					}
				}
			}
			memcpy((void*)Xnorm[matlevel], (void*)temp, sizeof(GzMatrix));
		}

		matlevel++;
	}
	return GZ_SUCCESS;
}

int GzRender::GzPopMatrix()
{
/* HW 3.10
- pop a matrix off the Ximage stack
- check for stack underflow
*/
	if (matlevel == 0)
		return GZ_FAILURE;
	matlevel--;
	return GZ_SUCCESS;
}

int GzRender::GzPut(int i, int j, GzIntensity r, GzIntensity g, GzIntensity b, GzIntensity a, GzDepth z)
{
/* HW1.4 write pixel values into the buffer */
	if (i >= 0 && i < xres && j >= 0 && j < yres && z <= pixelbuffer[j * xres + i].z && z >= 0)
	{
		pixelbuffer[j * xres + i].red = r <= 4095 ? r : 4095;
		pixelbuffer[j * xres + i].red = pixelbuffer[j * xres + i].red >= 0 ? pixelbuffer[j * xres + i].red : 0;
		pixelbuffer[j * xres + i].green = g <= 4095 ? g : 4095;
		pixelbuffer[j * xres + i].green = pixelbuffer[j * xres + i].green >= 0 ? pixelbuffer[j * xres + i].green : 0;
		pixelbuffer[j * xres + i].blue = b <= 4095 ? b : 4095;
		pixelbuffer[j * xres + i].blue = pixelbuffer[j * xres + i].blue >= 0 ? pixelbuffer[j * xres + i].blue : 0;
		pixelbuffer[j * xres + i].z = z;
	}
	return GZ_SUCCESS;
}


int GzRender::GzGet(int i, int j, GzIntensity *r, GzIntensity *g, GzIntensity *b, GzIntensity *a, GzDepth *z)
{
/* HW1.5 retrieve a pixel information from the pixel buffer */
	if (i < xres && j < yres && i >= 0 && j >= 0)
	{
		r = &pixelbuffer[j * xres + i].red;
		g = &pixelbuffer[j * xres + i].green;
		b = &pixelbuffer[j * xres + i].blue;
		z = &pixelbuffer[j * xres + i].z;
	}
	return GZ_SUCCESS;
}


int GzRender::GzFlushDisplay2File(FILE* outfile)
{
/* HW1.6 write image to ppm file -- "P6 %d %d 255\r" */
	fprintf(outfile, "P6 %d %d 255\r", xres, yres);
	for (int j = 0; j < yres; j++)
	{
		for (int i = 0; i < xres; i++)
		{
			char color[3];
			color[0] = pixelbuffer[j * xres + i].red >> 4;
			color[1] = pixelbuffer[j * xres + i].green >> 4;
			color[2] = pixelbuffer[j * xres + i].blue >> 4;
			fwrite(color, 1, 3, outfile);
		}
	}
	char* clock = new char[30];
	sprintf(clock, "%f %f %f", (float)start * 1000/ CLOCKS_PER_SEC, (float)mid * 1000 / CLOCKS_PER_SEC, (float)ending * 1000 / CLOCKS_PER_SEC);
	fputs(clock, outfile);
	return GZ_SUCCESS;
}

int GzRender::GzFlushDisplay2FrameBuffer()
{
/* HW1.7 write pixels to framebuffer: 
	- put the pixels into the frame buffer
	- CAUTION: when storing the pixels into the frame buffer, the order is blue, green, and red 
	- NOT red, green, and blue !!!
*/
	for (int i = 0; i < xres; i++)
	{
		for (int j = 0; j < yres; j++)
		{
			framebuffer[(j * xres + i) * 3] = pixelbuffer[j * xres + i].blue >> 4;
			framebuffer[(j * xres + i) * 3 + 1] = pixelbuffer[j * xres + i].green >> 4;
			framebuffer[(j * xres + i) * 3 + 2] = pixelbuffer[j * xres + i].red >> 4;
		}
	}
	return GZ_SUCCESS;
}


/***********************************************/
/* HW2 methods: implement from here */

int GzRender::GzPutAttribute(int numAttributes, GzToken	*nameList, GzPointer *valueList) 
{
/* HW 2.1
-- Set renderer attribute states (e.g.: GZ_RGB_COLOR default color)
-- In later homeworks set shaders, interpolaters, texture maps, and lights
*/

/*
- GzPutAttribute() must accept the following tokens/values:

- GZ_RGB_COLOR					GzColor		default flat-shade color
- GZ_INTERPOLATE				int			shader interpolation mode
- GZ_DIRECTIONAL_LIGHT			GzLight
- GZ_AMBIENT_LIGHT            	GzLight		(ignore direction)
- GZ_AMBIENT_COEFFICIENT		GzColor		Ka reflectance
- GZ_DIFFUSE_COEFFICIENT		GzColor		Kd reflectance
- GZ_SPECULAR_COEFFICIENT       GzColor		Ks coef's
- GZ_DISTRIBUTION_COEFFICIENT   float		spec power
*/
	for (int i = 0; i < numAttributes; i++)
	{
		if (nameList[i] == GZ_AMBIENT_LIGHT)
		{
			memcpy((void*)&ambientlight, (void*)valueList[i], sizeof(GzLight));
		}
		else if (nameList[i] == GZ_DIRECTIONAL_LIGHT)
		{
			memcpy((void*)&lights[numlights], (void*)valueList[i], sizeof(GzLight));
			numlights++;
		}
		else if (nameList[i] == GZ_DIFFUSE_COEFFICIENT)
		{
			memcpy((void*)Kd, (void*)valueList[i], sizeof(float) * 3);
		}
		else if (nameList[i] == GZ_AMBIENT_COEFFICIENT)
		{
			memcpy((void*)Ka, (void*)valueList[i], sizeof(float) * 3);
		}
		else if (nameList[i] == GZ_SPECULAR_COEFFICIENT)
		{
			memcpy((void*)Ks, (void*)valueList[i], sizeof(float) * 3);
		}
		else if (nameList[i] == GZ_DISTRIBUTION_COEFFICIENT)
		{
			memcpy((void*)&spec, (void*)valueList[i], sizeof(float));
		}
		else if (nameList[i] == GZ_INTERPOLATE)
		{
			memcpy((void*)&interp_mode, (void*)valueList[i], sizeof(int));
		}
		else if (nameList[i] == GZ_TEXTURE_MAP)
		{
			tex_fun = (GzTexture)valueList[i];
			//memcpy((void*)tex_fun, (void*)valueList[i], sizeof(GzTexture));
		}
	}
	if (nameList[0] == GZ_RGB_COLOR)
	{
		//flatcolor[0] = *((GzColor*)valueList[0])[0];
		memcpy((void*)flatcolor, (void*)valueList[0], sizeof(float) * 3);
	}
	return GZ_SUCCESS;
}

void computeCoeffcient(float x, float y, float dx, float dy, float& A, float& B, float& C)
{
	A = dy;
	B = -dx;
	C = dx * y - dy * x;
}

void computePlane(float x, float y, float z, float a1, float b1, float c1, float a2, float b2, float c2, float& A, float& B, float& C, float& D)
{
	A = b1 * c2 - b2 * c1;
	B = c1 * a2 - a1 * c2;
	C = a1 * b2 - a2 * b1;
	D = -A * x - B * y - C * z;
}

int checkCCW(float x1, float y1, float x2, float y2)
{
	// 1 stands for CCW, 2 stands for CW, 0 stands for same line
	if (x1 * y2 - x2 * y1 == 0)
	{
		return 0;
	}
	return x1 * y2 - x2 * y1 > 0 ? 1 : 2;
}

struct vertex {
	float x;
	float y;
	int index;
};

struct edge {
	int from;
	int to;
	float A, B, C;
	bool isLeft = false;
};

int maximum(float a, float b, float c)
{
	int max = a > b ? a : b;
	max = max > c ? max : c;
	return max;
}

int minimum(float a, float b, float c)
{
	int min = a < b ? a : b;
	min = min < c ? min : c;
	return min;
}

void checkLeft(float x0, float y0, float x1, float y1, float x2, float y2, edge* edges)
{
	// final render order 0-1-2-0 or 0-2-1-0, according to clock, returns left edge or not
	// edges 1: left edge 0:right edge edges[0] stands for 0-1 or 0-2 according to the clock
	vertex v[3];
	v[0].index = 0;
	v[0].x = x0;
	v[0].y = y0;
	v[1].index = 1;
	v[1].x = x1;
	v[1].y = y1;
	v[2].index = 2;
	v[2].x = x2;
	v[2].y = y2;
	for (int i = 0; i < 3; i++)
	{
		for (int j = i + 1; j < 3; j++)
		{
			if (v[j].y > v[i].y)
			{
				vertex tv;
				tv = v[i];
				v[i] = v[j];
				v[j] = tv;
			}
		}
	}
	//v[0] maximum v[2] mininum
	if (v[0].y == v[1].y)
	{
		if (v[0].x < v[1].x)
		{
			edges[v[0].index].isLeft = true;
		}
		else
		{
			edges[v[1].index].isLeft = true;
		}
	}
	else if (v[1].y == v[2].y)
	{
		edges[v[0].index].isLeft = true;
		//check which is left
		if (v[1].x < v[2].x)
		{
			edges[v[1].index].isLeft = true;
		}
		else
		{
			edges[v[2].index].isLeft = true;
		}
	}
	else
	{
		if (edges[v[0].index].to == v[2].index)
		{
			edges[v[0].index].isLeft = true;
		}
		else
		{
			edges[v[0].index].isLeft = true;
			edges[v[1].index].isLeft = true;
		}
	}
}

bool isInside(int x, int y, edge* edges)
{
	return	(
		edges[0].A * x + edges[0].B * y + edges[0].C == 0 && edges[0].isLeft ||
		edges[1].A * x + edges[1].B * y + edges[1].C == 0 && edges[1].isLeft ||
		edges[2].A * x + edges[2].B * y + edges[2].C == 0 && edges[2].isLeft
		)
		||
		edges[0].A * x + edges[0].B * y + edges[0].C > 0 &&
		edges[1].A * x + edges[1].B * y + edges[1].C > 0 &&
		edges[2].A * x + edges[2].B * y + edges[2].C > 0;
}

void GzComputeCoord(GzMatrix matrix, GzCoord src, GzCoord des)
{
	float x = matrix[0][0] * src[0] + matrix[0][1] * src[1] + matrix[0][2] * src[2] + matrix[0][3] * 1.0;
	float y = matrix[1][0] * src[0] + matrix[1][1] * src[1] + matrix[1][2] * src[2] + matrix[1][3] * 1.0;
	float z = matrix[2][0] * src[0] + matrix[2][1] * src[1] + matrix[2][2] * src[2] + matrix[2][3] * 1.0;
	float w = matrix[3][0] * src[0] + matrix[3][1] * src[1] + matrix[3][2] * src[2] + matrix[3][3] * 1.0;
	des[0] = x / w;
	des[1] = y / w;
	des[2] = z / w;
}

float clamp(float min, float max, float x)
{
	if (x < min) return min;
	if (x >= min && x <= max) return x;
	if (x > max) return max;
}

void GzRender::GzComputeColor(GzCoord coord, GzColor color)
{
	GzCoord eye = { 0, 0, -1 };
	//GzCoord coord = { coord0[0], coord0[1], coord0[2] };
	GzCoord specular = { 0, 0, 0 }, diffuse = { 0, 0, 0 }, ambient = { 0, 0, 0 };
	for (int i = 0; i < numlights; i++)
	{
		float nl = GzDot(coord, lights[i].direction);
		float ne = GzDot(coord, eye);
		if (nl >= 0 && ne >= 0)
		{
			// R dot E inside this or not?
			// do nothing
			// equal 0 is fine??
		}
		else if (nl <= 0 && ne <= 0)
		{
			coord[0] = -coord[0];
			coord[1] = -coord[1];
			coord[2] = -coord[2];
		}
		else
		{
			continue;
		}

		nl = GzDot(coord, lights[i].direction);
		GzCoord R = { 2 * nl * coord[0] - lights[i].direction[0], 2 * nl * coord[1] - lights[i].direction[1], 2 * nl * coord[2] - lights[i].direction[2] };
		float re = GzDot(R, eye);
		re = clamp(0, 1, re);

		specular[0] += lights[i].color[0] * pow(re, spec);
		specular[1] += lights[i].color[1] * pow(re, spec);
		specular[2] += lights[i].color[2] * pow(re, spec);
		diffuse[0] += lights[i].color[0] * nl;
		diffuse[1] += lights[i].color[1] * nl;
		diffuse[2] += lights[i].color[2] * nl;
	}
	color[0] = Ks[0] * specular[0] + Kd[0] * diffuse[0] + ambientlight.color[0] * Ka[0];
	color[1] = Ks[1] * specular[1] + Kd[1] * diffuse[1] + ambientlight.color[1] * Ka[1];
	color[2] = Ks[2] * specular[2] + Kd[2] * diffuse[2] + ambientlight.color[2] * Ka[2];
	color[0] = clamp(0, 1, color[0]);
	color[1] = clamp(0, 1, color[1]);
	color[2] = clamp(0, 1, color[2]);
}

void GzRender::GzComputePlane(GzCoord* vertices, GzCoord* coeff)
{
	float a1 = vertices[1][0] - vertices[0][0]; //X1
	float b1 = vertices[1][1] - vertices[0][1]; //y1
	float a2 = vertices[2][0] - vertices[0][0]; //X2
	float b2 = vertices[2][1] - vertices[0][1]; //Y2

	for (int i = 0; i < 3; i++)
	{
		float c1 = coeff[1][i] - coeff[0][i];
		float c2 = coeff[2][i] - coeff[0][i];
		computePlane(vertices[0][0], vertices[0][1], coeff[0][i], a1, b1, c1, a2, b2, c2, planes[i][0], planes[i][1], planes[i][2], planes[i][3]);
	}
}

void GzRender::GzComputePlane(GzCoord* vertices, GzCoord* coeff, float (*plane)[4])
{
	float a1 = vertices[1][0] - vertices[0][0]; //X1
	float b1 = vertices[1][1] - vertices[0][1]; //y1
	float a2 = vertices[2][0] - vertices[0][0]; //X2
	float b2 = vertices[2][1] - vertices[0][1]; //Y2
	for (int i = 0; i < 2; i++)
	{
		float c1 = coeff[1][i] - coeff[0][i];
		float c2 = coeff[2][i] - coeff[0][i];
		computePlane(vertices[0][0], vertices[0][1], coeff[0][i], a1, b1, c1, a2, b2, c2, plane[i][0], plane[i][1], plane[i][2], plane[i][3]);
	}
}

int GzRender::GzPutTriangle(int numParts, GzToken *nameList, GzPointer *valueList)
/* numParts - how many names and values */
{
/* HW 2.2
-- Pass in a triangle description with tokens and values corresponding to
      GZ_NULL_TOKEN:		do nothing - no values
      GZ_POSITION:		3 vert positions in model space
-- Return error code
*/
/*
-- Xform positions of verts using matrix on top of stack 
-- Clip - just discard any triangle with any vert(s) behind view plane 
		- optional: test for triangles with all three verts off-screen (trivial frustum cull)
-- invoke triangle rasterizer  
*/
	GzCoord colors[3];
	GzTextureIndex uvList[3];
	if (nameList[0] == GZ_NULL_TOKEN)
	{
		return GZ_FAILURE;
	}
	if (nameList[2] == GZ_TEXTURE_INDEX)
	{
		memcpy((void*)uvList, (void*)valueList[2], sizeof(GzTextureIndex) * 3);

		for (int j = 0; j < 3; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				trianglebuffer[triIndex].uv[j][k] = uvList[j][k];
			}
		}
	}
	if (nameList[1] == GZ_NORMAL)
	{
		GzCoord temp[3];
		memcpy((void*)temp, (void*)valueList[1], sizeof(GzCoord) * 3);
		GzCoord coord0, coord1, coord2;
		GzComputeCoord(Xnorm[matlevel - 1], temp[0], coord0);
		GzComputeCoord(Xnorm[matlevel - 1], temp[1], coord1);
		GzComputeCoord(Xnorm[matlevel - 1], temp[2], coord2);

		//save transformed norms to triangle buffer
		for (int i = 0; i < 3; i++)
		{
			trianglebuffer[triIndex].normals[0][i] = coord0[i];
			trianglebuffer[triIndex].normals[1][i] = coord1[i];
			trianglebuffer[triIndex].normals[2][i] = coord2[i];
		}
		if (triIndex == 0)
		{

			for (int i = 0; i < 3; i++)
			{
				trianglebuffer[triIndex].normals[i][0] = 0;
				trianglebuffer[triIndex].normals[i][1] = 0;
				trianglebuffer[triIndex].normals[i][2] = 1;

			}
		}
		if (interp_mode == GZ_FLAT)
		{
			// choose first norm coord0
			GzCoord coord = { coord0[0], coord0[1], coord0[2] };
			GzComputeColor(coord, trianglebuffer[triIndex].colors[0]);
		}
		else if (interp_mode == GZ_COLOR)
		{
			GzCoord K_temp = { 1.0, 1.0, 1.0 };
			memcpy((void*)Ka, (void*)K_temp, sizeof(GzCoord));
			memcpy((void*)Kd, (void*)K_temp, sizeof(GzCoord));
			memcpy((void*)Ks, (void*)K_temp, sizeof(GzCoord));
			GzComputeColor(coord0, trianglebuffer[triIndex].colors[0]);
			GzComputeColor(coord1, trianglebuffer[triIndex].colors[1]);
			GzComputeColor(coord2, trianglebuffer[triIndex].colors[2]);
		}
		else if (interp_mode == GZ_NORMALS)
		{
			GzCoord coord = { coord0[0], coord0[1], coord0[2] };
			GzComputeColor(coord, trianglebuffer[triIndex].colors[0]);

			/*memcpy((void*)norms[0], (void*)coord0, sizeof(GzCoord));
			memcpy((void*)norms[1], (void*)coord1, sizeof(GzCoord));
			memcpy((void*)norms[2], (void*)coord2, sizeof(GzCoord));*/
		}
	}
	if (nameList[0] == GZ_POSITION)
	{
		edge edges[3];
		GzCoord temp[3];
		memcpy((void*)temp, (void*)valueList[0], sizeof(GzCoord) * 3);
		
		GzCoord screenvertices[3];
		GzComputeCoord(X2screen[matlevel - 1], temp[0], screenvertices[0]);
		GzComputeCoord(X2screen[matlevel - 1], temp[1], screenvertices[1]);
		GzComputeCoord(X2screen[matlevel - 1], temp[2], screenvertices[2]);
		
		GzCoord imagevertices[3];
		GzComputeCoord(X2image[matlevel - 1], temp[0], imagevertices[0]);
		GzComputeCoord(X2image[matlevel - 1], temp[1], imagevertices[1]);
		GzComputeCoord(X2image[matlevel - 1], temp[2], imagevertices[2]);

		//make the first triangle a mirror!
		if (triIndex == 0)
		{
			trianglebuffer[triIndex].isMirror = true;

			imagevertices[0][0] -= 30;
			imagevertices[1][0] -= 30;
			imagevertices[2][0] -= 30;
			imagevertices[0][1] += 10;
			imagevertices[1][1] += 10;
			imagevertices[2][1] += 10;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					//imagevertices[i][j] *= 5;
				}
				imagevertices[i][2] = 50;
				
			}
		}
		
		//save verts to triangle buffer
		for (int j = 0; j < 3; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				trianglebuffer[triIndex].screenVerts[j][k] = screenvertices[j][k];
				trianglebuffer[triIndex].imageVerts[j][k] = imagevertices[j][k];
				switch (k)
				{
				case 0:
					root->max_x = max(trianglebuffer[triIndex].imageVerts[j][k], root->max_x);
					root->min_x = min(trianglebuffer[triIndex].imageVerts[j][k], root->min_x);
					break;
				case 1:
					root->max_y = max(trianglebuffer[triIndex].imageVerts[j][k], root->max_y);
					root->min_y = min(trianglebuffer[triIndex].imageVerts[j][k], root->min_y);
					break;
				case 2:
					root->max_z = max(trianglebuffer[triIndex].imageVerts[j][k], root->max_z);
					root->min_z = min(trianglebuffer[triIndex].imageVerts[j][k], root->min_z);
					break;
				default:
					continue;
				}
			}
		}

		//save plane coeff to triangle buffer
		//compute triangle (plane) normal using: crossProduct(1, 2, result)
		GzCoord edge1, edge2, edge3;
		minus(trianglebuffer[triIndex].imageVerts[1], trianglebuffer[triIndex].imageVerts[0], edge1);
		minus(trianglebuffer[triIndex].imageVerts[2], trianglebuffer[triIndex].imageVerts[1], edge2);
		minus(trianglebuffer[triIndex].imageVerts[0], trianglebuffer[triIndex].imageVerts[2], edge3);

		GzCoord planeNorm;
		crossProduct(edge1, edge2, planeNorm);
		normalize(planeNorm, planeNorm);
		float dCoeff = -dotProduct(planeNorm, trianglebuffer[triIndex].imageVerts[2]);
		trianglebuffer[triIndex].coeff[0] = planeNorm[0];
		trianglebuffer[triIndex].coeff[1] = planeNorm[1];
		trianglebuffer[triIndex].coeff[2] = planeNorm[2];
		trianglebuffer[triIndex].coeff[3] = dCoeff;
	}

	triIndex++;
	
	return GZ_SUCCESS;
}
bool GzRender::insideBoundingBox(GzCoord vertex, BSP_tree* node)
{
	return ((vertex[0] >= node->min_x && vertex[0] <= node->max_x) &&
		(vertex[1] >= node->min_y && vertex[1] <= node->max_y) &&
		(vertex[2] >= node->min_z && vertex[2] <= node->max_z));
}

void GzRender::GzCreateBSPTree(BSP_tree* node, int depth)
{
	if (depth > BSP_MAX_DEPTH || node->triangles.size() < BSP_MAX_NODE_SIZE)
	{
		return;
	}
	BSP_tree* front = new BSP_tree(node);
	BSP_tree* back = new BSP_tree(node);
	float mid;
	if (node->order == 0)		/* xy plane*/
	{
		mid = (node->max_z + node->min_z) / 2;
		front->min_z = mid;
		back->max_z = mid;

	}
	else if (node->order == 1)	/* yz plane*/
	{
		mid = (node->max_x + node->min_x) / 2;
		front->min_x = mid;
		back->max_x = mid;
	}
	else if (node->order == 2)	/* xz plane*/
	{
		mid = (node->max_y + node->min_y) / 2;
		front->min_y = mid;
		back->max_y = mid;
	}
	/* Compute triangle candidate list in their own space */
	for (auto& index : node->triangles)
	{
		GzTri tri = trianglebuffer[index];
		/* check if three vertices inside the bounding box */
		bool putInFront = false, putInBack = false;
		for (int i = 0; i < 3; i++)
		{
			//if (insideBoundingBox(tri.imageVerts[i], front) && !putInFront)
			//if (tri.imageVerts[i][(node->order +2)%3] >= mid && !putInFront)
			if (GzAABBCollisionDetect(tri, front) && !putInFront)
			{
				front->triangles.push_back(index);
				putInFront = true;
			}
			//if (insideBoundingBox(tri.imageVerts[i], back) && !putInBack)
			//if (tri.imageVerts[i][(node->order + 2) % 3] <= mid && !putInBack)
			if (GzAABBCollisionDetect(tri, back) && !putInBack)
			{
				back->triangles.push_back(index);
				putInBack = true;
			}
		}
	}
	//node->triangles.clear();
	node->front = front;
	node->back = back;
	GzCreateBSPTree(node->front, depth + 1);
	GzCreateBSPTree(node->back, depth + 1);
}
bool GzRender::GzAABBCollisionDetect(GzTri& triangle, BSP_tree* node)
{
	float min_x = min(min(triangle.imageVerts[0][0], triangle.imageVerts[1][0]), triangle.imageVerts[2][0]);
	float max_x = max(max(triangle.imageVerts[0][0], triangle.imageVerts[1][0]), triangle.imageVerts[2][0]);
	float min_y = min(min(triangle.imageVerts[0][1], triangle.imageVerts[1][1]), triangle.imageVerts[2][1]);
	float max_y = max(max(triangle.imageVerts[0][1], triangle.imageVerts[1][1]), triangle.imageVerts[2][1]);
	float min_z = min(min(triangle.imageVerts[0][2], triangle.imageVerts[1][2]), triangle.imageVerts[2][2]);
	float max_z = max(max(triangle.imageVerts[0][2], triangle.imageVerts[1][2]), triangle.imageVerts[2][2]);
	return (!((node->min_x > max_x) || (node->max_x < min_x)) &&
		!((node->min_y > max_y) || (node->max_y < min_y)) &&
		!((node->min_z > max_z) || (node->max_z < min_z)));
}
bool GzRender::GzRayTreeIntersect(BSP_tree* node, GzTri*& intersectTriangle, GzCoord intersectPoint, float min, float max, GzTri* exception)
{
	if (node == NULL)	return false;
	if (node->front == NULL && node->back == NULL)
	{
		// leaf node
		return GzFindFrontestFromList(intersectTriangle, intersectPoint, node, exception);
	}

	/* compute dist to cutting plane */
	float dist;
	BSP_tree* nearRay = node->front, * farRay = node->back;
	if (node->order == 0)		/* xy plane*/
	{
		float mid_z = (node->max_z + node->min_z) / 2;
		dist = (mid_z - ray.origin[2]) / ray.direction[2];
		if (ray.origin[2] < mid_z)
		{
			nearRay = node->back;
			farRay = node->front;
		}
	}
	else if (node->order == 1)	/* yz plane*/
	{
		float mid_x = (node->max_x + node->min_x) / 2;
		dist = (mid_x - ray.origin[0]) / ray.direction[0];
		if (ray.origin[0] < mid_x)
		{
			nearRay = node->back;
			farRay = node->front;
		}
	}
	else if (node->order == 2)	/* xz plane*/
	{
		float mid_y = (node->max_y + node->min_y) / 2;
		dist = (mid_y - ray.origin[1]) / ray.direction[1];
		if (ray.origin[1] < mid_y)
		{
			nearRay = node->back;
			farRay = node->front;
		}
	}
	if (dist > max || dist < 0)	GzRayTreeIntersect(nearRay, intersectTriangle, intersectPoint, min, max, exception);
	else if (dist < min)	GzRayTreeIntersect(farRay, intersectTriangle, intersectPoint, min, max, exception);
	else
	{
		return GzRayTreeIntersect(nearRay, intersectTriangle, intersectPoint, min, dist, exception) ? true : GzRayTreeIntersect(farRay, intersectTriangle, intersectPoint, dist, max, exception);
	}
}

bool GzRender::GzFindFrontestIntersection_BSP(GzTri*& intersectTriangle, GzCoord intersectPoint, GzTri* exception)
{
	float bmin = FLT_MAX, bmax = FLT_MIN;

	auto t_section = [](float origin, float direction, float p)
	{
		return (p - origin) / direction;
	};

	// compute ray intersection with six surfaces
	// xy plane
	float t_minz = t_section(ray.origin[2], ray.direction[2], root->min_z);
	float t_maxz = t_section(ray.origin[2], ray.direction[2], root->max_z);
	GzCoord intersection_minz = { ray.origin[0] + ray.direction[0] * t_minz, ray.origin[1] + ray.direction[1] * t_minz, root->min_z };
	GzCoord intersection_maxz = { ray.origin[0] + ray.direction[0] * t_maxz, ray.origin[1] + ray.direction[1] * t_maxz, root->max_z };
	if (insideBoundingBox(intersection_minz, root))
	{
		bmin = min(bmin, t_minz);
		bmax = max(bmax, t_minz);
	}
	if (insideBoundingBox(intersection_maxz, root))
	{
		bmin = min(bmin, t_maxz);
		bmax = max(bmax, t_maxz);
	}
	//yz plane
	float t_minx = t_section(ray.origin[0], ray.direction[0], root->min_x);
	float t_maxx = t_section(ray.origin[0], ray.direction[0], root->max_x);
	GzCoord intersection_minx = { root->min_x, ray.origin[1] + ray.direction[1] * t_minx, ray.origin[2] + ray.direction[2] * t_minx };
	GzCoord intersection_maxx = { root->max_x, ray.origin[1] + ray.direction[1] * t_maxx, ray.origin[2] + ray.direction[2] * t_maxx };
	if (insideBoundingBox(intersection_minx, root))
	{
		bmin = min(bmin, t_minx);
		bmax = max(bmax, t_minx);
	}
	if (insideBoundingBox(intersection_maxx, root))
	{
		bmin = min(bmin, t_maxx);
		bmax = max(bmax, t_maxx);
	}
	// xz plane
	float t_miny = t_section(ray.origin[1], ray.direction[1], root->min_y);
	float t_maxy = t_section(ray.origin[1], ray.direction[1], root->max_y);
	GzCoord intersection_miny = { ray.origin[0] + ray.direction[0] * t_miny, root->min_y, ray.origin[2] + ray.direction[2] * t_miny };
	GzCoord intersection_maxy = { ray.origin[0] + ray.direction[0] * t_maxy, root->max_y, ray.origin[2] + ray.direction[2] * t_maxy };
	if (insideBoundingBox(intersection_miny, root))
	{
		bmin = min(bmin, t_miny);
		bmax = max(bmax, t_miny);
	}
	if (insideBoundingBox(intersection_maxy, root))
	{
		bmin = min(bmin, t_maxy);
		bmax = max(bmax, t_maxy);
	}

	return GzRayTreeIntersect(root, intersectTriangle, intersectPoint, bmin, bmax, exception);
}

bool GzRender::GzFindFrontestFromList(GzTri*& intersectTriangle, GzCoord intersectPoint, BSP_tree* node, GzTri* exception)
{
	boolean flag = false;
	float minDistance = -1.0;
	for (auto& i : node->triangles)
	{
		GzTri* pt = &trianglebuffer[i];
		if (pt == exception)//skip a specific triangle
		{
			continue;
		}
		GzTri triangle = trianglebuffer[i];
		GzCoord planeNorm = { triangle.coeff[0], triangle.coeff[1], triangle.coeff[2] };
		float dCoeff = triangle.coeff[3];
		//check if ray and triangle are parallel
		float dir = dotProduct(planeNorm, ray.direction);
		if (dir == 0)
		{
			//no intersection
			continue;
		}

		//compute D then t, ensure t >= 0
		//t = (- D - dot(N, orig)) / dot(N, dir); 

		float tValue = (-dCoeff - dotProduct(planeNorm, ray.origin)) / (dotProduct(planeNorm, ray.direction));
		if (tValue <= 0)
		{
			//no intersection
			continue;
		}

		//Intersection point
		GzCoord Pvalue;
		PointAtTValue(tValue, Pvalue);

		//check if intersection point is inside bounding box
		if (!insideBoundingBox(Pvalue, node))
		{
			continue;
		}

		//check if intersection point is inside triangle
		GzCoord edge1, edge2, edge3, coord1, coord2, coord3, cross1, cross2, cross3;
		minus(Pvalue, triangle.imageVerts[0], coord1);
		minus(Pvalue, triangle.imageVerts[1], coord2);
		minus(Pvalue, triangle.imageVerts[2], coord3);

		minus(triangle.imageVerts[1], triangle.imageVerts[0], edge1);
		minus(triangle.imageVerts[2], triangle.imageVerts[1], edge2);
		minus(triangle.imageVerts[0], triangle.imageVerts[2], edge3);
		crossProduct(edge1, coord1, cross1);
		crossProduct(edge2, coord2, cross2);
		crossProduct(edge3, coord3, cross3);

		if (dotProduct(cross1, planeNorm) >= 0 && dotProduct(cross2, planeNorm) >= 0 && dotProduct(cross3, planeNorm) >= 0)
		{
			flag = true;//ray hits triangle :)
			if (minDistance < 0 || tValue < minDistance)
			{
				minDistance = tValue;
				intersectTriangle = &trianglebuffer[i];
				for (int c = 0; c < 3; c++)
				{
					intersectPoint[c] = Pvalue[c];
				}
			}
		}

	}
	return flag;
}
int GzRender::GzRaytracing()
{
	
	start = clock();
	/* GzCreateBSPTree initial */
	for (int i = 0; i < triIndex; i++)	root->triangles.push_back(i);
	GzCreateBSPTree(root, 1);
	mid = clock();

	//shoot primary ray through each pixel
	float focalDistance = 1 / tan(m_camera.FOV * PI / (180 * 2));
	for (int i = 0; i < xres; i++)
	{
		for (int j = 0; j < yres; j++)
		{
			//compute primary ray
			GzCoord start = { 0, 0, 0 };
			memcpy((void*)ray.origin, (void*)start, sizeof(GzCoord));
			GzCoord destination = { i * (2.0 / xres) - 1, 1 - j * (2.0 / yres), focalDistance };
			memcpy((void*)ray.direction, (void*)destination, sizeof(GzCoord));
			normalize(ray.direction, ray.direction);

			//compute color at this screen pixel and draw
			GzColor color;
			bool result = GzIntersectColor(color, 0, nullptr);
			if (result)
			{
				this->GzPut(i, j, ctoi(color[0]), ctoi(color[1]), ctoi(color[2]), 1, pixelbuffer[j * xres + i].z);
			}


		}
	}
	ending = clock();
	return GZ_SUCCESS;
}

int GzRender::PointAtTValue(float t, GzCoord coord)
{
	//origin + t * dir
	GzCoord scaled;
	for (int i = 0; i < 3; i++)
	{
		coord[i] = ray.direction[i] * t + ray.origin[i];
	}

	return GZ_SUCCESS;
}

//exception: the triangle that need to skip. if none, use nullptr
//if return value is true, result will be in triangle and intersection. 
//else there is no intersection
//example: 
//GzTri* pTriangle;
//GzCoord pIntersection;
//bool flag = GzFindFrontestIntersection(pTriangle, pIntersection);
bool GzRender::GzFindFrontestIntersection(GzTri*& intersectTriangle, GzCoord intersectPoint, GzTri* exception)
{
	bool flag = false;
	float minDistance = -1.0;
	for (int i = 0; i < triIndex; i++)
	{
		GzTri* pt = &trianglebuffer[i];
		if (pt == exception)//skip a specific triangle
		{
			continue;
		}
		GzTri triangle = trianglebuffer[i];
		GzCoord planeNorm = { triangle.coeff[0], triangle.coeff[1], triangle.coeff[2] };
		float dCoeff = triangle.coeff[3];
		//check if ray and triangle are parallel
		float dir = dotProduct(planeNorm, ray.direction);
		if (dir == 0)
		{
			//no intersection
			continue;
		}

		//t = (- D - dot(N, orig)) / dot(N, dir); 
		float tValue = (-dCoeff - dotProduct(planeNorm, ray.origin)) / (dotProduct(planeNorm, ray.direction));
		if (tValue <= 0)
		{
			//no intersection
			continue;
		}

		//check if intersection point is inside triangle
		GzCoord Pvalue, edge1, edge2, edge3, coord1, coord2, coord3, cross1, cross2, cross3;
		PointAtTValue(tValue, Pvalue);

		minus(Pvalue, triangle.imageVerts[0], coord1);
		minus(Pvalue, triangle.imageVerts[1], coord2);
		minus(Pvalue, triangle.imageVerts[2], coord3);

		minus(triangle.imageVerts[1], triangle.imageVerts[0], edge1);
		minus(triangle.imageVerts[2], triangle.imageVerts[1], edge2);
		minus(triangle.imageVerts[0], triangle.imageVerts[2], edge3);
		crossProduct(edge1, coord1, cross1);
		crossProduct(edge2, coord2, cross2);
		crossProduct(edge3, coord3, cross3);

		if (dotProduct(cross1, planeNorm) >= 0 && dotProduct(cross2, planeNorm) >= 0 && dotProduct(cross3, planeNorm) >= 0)
		{
			flag = true;//ray hits triangle :)
			if (minDistance < 0 || tValue < minDistance)
			{
				minDistance = tValue;
				intersectTriangle = &trianglebuffer[i];
				for (int c = 0; c < 3; c++)
				{
					intersectPoint[c] = Pvalue[c];
				}
			}
		}

	}
	return flag;
}

int GzRender::Rasterize(GzTri triangle)
{
	GzCoord norms[3], temp[3];
	GzTextureIndex uvList[3];
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			temp[i][j] = triangle.screenVerts[i][j];
			norms[i][j] = triangle.normals[i][j];
		}
		for (int k = 0; k < 2; k++)
		{
			uvList[i][k] = triangle.uv[i][k];
		}
	}

	edge edges[3];
	GzCoord coord0, coord1, coord2;
	memcpy((void*)coord0, (void*)temp[0], sizeof(GzCoord));
	memcpy((void*)coord1, (void*)temp[1], sizeof(GzCoord));
	memcpy((void*)coord2, (void*)temp[2], sizeof(GzCoord));

	GzCoord vertices[3] = { {coord0[0],coord0[1], coord0[2] / (MAXINT - coord0[2])},
		{coord1[0], coord1[1], coord1[2] / (MAXINT - coord1[2])},
		{coord2[0], coord2[1], coord2[2] / (MAXINT - coord2[2])} };

	GzCoord coeff[3] = { {uvList[0][0] / (vertices[0][2] + 1), uvList[0][1] / (vertices[0][2] + 1), 1},
			{uvList[1][0] / (vertices[1][2] + 1), uvList[1][1] / (vertices[1][2] + 1), 1},
			{uvList[2][0] / (vertices[2][2] + 1), uvList[2][1] / (vertices[2][2] + 1), 1} };
	float uv_plane[2][4];
	GzComputePlane(vertices, coeff, uv_plane);

	// Given first point as A, second as B, third as C, and try to compute AB, AC
	// first try to find CCW of vertexs compute (A,B) (A,C)
	int clock = checkCCW(temp[1][0] - temp[0][0], temp[1][1] - temp[0][1], temp[2][0] - temp[0][0], temp[2][1] - temp[0][1]);
	if (!clock)
	{
		return GZ_SUCCESS;
	}
	if (clock == 1)
	{
		edges[0].from = 0;
		edges[0].to = 1;
		edges[1].from = 1;
		edges[1].to = 2;
		edges[2].from = 2;
		edges[2].to = 0;
	}
	else if (clock == 2)
	{
		edges[0].from = 0;
		edges[0].to = 2;
		edges[1].from = 1;
		edges[1].to = 0;
		edges[2].from = 2;
		edges[2].to = 1;
	}
	// try to compute coeffcient
	for (int i = 0; i < 3; i++)
	{
		int from = edges[i].from;
		int to = edges[i].to;
		computeCoeffcient(temp[to][0], temp[to][1], temp[from][0] - temp[to][0], temp[from][1] - temp[to][1], edges[i].A, edges[i].B, edges[i].C);
	}
	// try to compute global plane normal
	float Az, Bz, Cz, Dz;
	float a1 = temp[1][0] - temp[0][0];
	float b1 = temp[1][1] - temp[0][1];
	float c1 = temp[1][2] - temp[0][2];
	float a2 = temp[2][0] - temp[0][0];
	float b2 = temp[2][1] - temp[0][1];
	float c2 = temp[2][2] - temp[0][2];
	computePlane(temp[0][0], temp[0][1], temp[0][2], a1, b1, c1, a2, b2, c2, Az, Bz, Cz, Dz);

	if (interp_mode == GZ_COLOR)
	{
		GzComputePlane(temp, trianglebuffer[triIndex].colors);
	}
	else if (interp_mode == GZ_NORMALS)
	{
		GzComputePlane(temp, norms);
	}

	// try to find the left edge when same y, if bottom, renders the bottom line and left line; if top, renders the left line;
	checkLeft(temp[0][0], temp[0][1], temp[1][0], temp[1][1], temp[2][0], temp[2][1], edges);
	// try to render the points inside the triangle
	int boundLeft, boundRight, boundUp, boundDown;
	boundLeft = minimum(temp[0][0], temp[1][0], temp[2][0]);
	boundRight = maximum(temp[0][0], temp[1][0], temp[2][0]);
	boundDown = minimum(temp[0][1], temp[1][1], temp[2][1]);
	boundUp = maximum(temp[0][1], temp[1][1], temp[2][1]);
	for (int i = ceil(boundLeft); i <= floor(boundRight); i++)
	{
		for (int j = ceil(boundDown); j <= floor(boundUp); j++)
		{
			if (isInside(i, j, edges))
			{
				float z = (-Az * i - Bz * j - Dz) / Cz;
				float x = i;//(i - xres / 2) / (xres / 2);
				float y = j;//(j - yres / 2) / (-yres / 2);
				float u = (-uv_plane[0][0] * x - uv_plane[0][1] * y - uv_plane[0][3]) / uv_plane[0][2];
				float v = (-uv_plane[1][0] * x - uv_plane[1][1] * y - uv_plane[1][3]) / uv_plane[1][2];
				float z_prime = z / (MAXINT - z);
				GzColor normColor;
				if (tex_fun != 0)
					tex_fun(u * (z_prime + 1), v * (z_prime + 1), normColor);
				if (interp_mode == GZ_COLOR)
				{
					flatcolor[0] = normColor[0] * (-planes[0][0] * i - planes[0][1] * j - planes[0][3]) / planes[0][2];
					flatcolor[1] = normColor[1] * (-planes[1][0] * i - planes[1][1] * j - planes[1][3]) / planes[1][2];
					flatcolor[2] = normColor[2] * (-planes[2][0] * i - planes[2][1] * j - planes[2][3]) / planes[2][2];
				}
				else if (interp_mode == GZ_NORMALS)
				{
					if (tex_fun != 0)
					{
						memcpy((void*)Kd, (void*)normColor, sizeof(GzColor));
						memcpy((void*)Ka, (void*)normColor, sizeof(GzColor));
					}
					GzCoord temp_norm;
					temp_norm[0] = (-planes[0][0] * i - planes[0][1] * j - planes[0][3]) / planes[0][2];
					temp_norm[1] = (-planes[1][0] * i - planes[1][1] * j - planes[1][3]) / planes[1][2];
					temp_norm[2] = (-planes[2][0] * i - planes[2][1] * j - planes[2][3]) / planes[2][2];
					float d = GzCoordLength(temp_norm);
					temp_norm[0] = temp_norm[0] / d;
					temp_norm[1] = temp_norm[1] / d;
					temp_norm[2] = temp_norm[2] / d;
					GzComputeColor(temp_norm, flatcolor);
				}
				else if (interp_mode == GZ_FLAT)
				{
					flatcolor[RED] = triangle.colors[0][RED];
					flatcolor[GREEN] = triangle.colors[0][GREEN];
					flatcolor[BLUE] = triangle.colors[0][BLUE];
				}
				this->GzPut(i, j, ctoi(flatcolor[0]), ctoi(flatcolor[1]), ctoi(flatcolor[2]), 255, ceil(z));
			}
		}
	}

	return GZ_SUCCESS;
}

int GzRender::PerspectiveCorrection(GzTextureIndex uvVals[], GzCoord pixel, GzCoord verts[], GzTextureIndex affine)
{
	//if at vertex: compute texture color using vertex u,v
	//if not at vertex: interpolate u,v values, then compute color
	int i, j, k;

	//transform u,v, into perspective space U,V
	GzTextureIndex perspUV[3];
	float Vz;
	for (i = 0; i < 3; i++)
	{
		Vz = verts[i][Z] / (MAXINT - verts[i][Z]);
		perspUV[i][U] = uvVals[i][U] / (Vz + 1);
		perspUV[i][V] = uvVals[i][V] / (Vz + 1);
	}

	//interpolate transformed U,V for current pixel
	float edgesdX[3], edgesdY[3], edgesdU[3], edgesdV[3];

	edgesdX[0] = verts[1][X] - verts[0][X];
	edgesdY[0] = verts[1][Y] - verts[0][Y];
	edgesdU[0] = perspUV[1][U] - perspUV[0][U];
	edgesdV[0] = perspUV[1][V] - perspUV[0][V];

	edgesdX[1] = verts[2][X] - verts[1][X];
	edgesdY[1] = verts[2][Y] - verts[1][Y];
	edgesdU[1] = perspUV[2][U] - perspUV[1][U];
	edgesdV[1] = perspUV[2][V] - perspUV[1][V];

	edgesdX[2] = verts[0][X] - verts[2][X];
	edgesdY[2] = verts[0][Y] - verts[2][Y];
	edgesdU[2] = perspUV[0][U] - perspUV[2][U];
	edgesdV[2] = perspUV[0][V] - perspUV[2][V];

	//A*x + B*y + C*(U, V) + D = 0
	GzTextureIndex interpUV;
	float Acoeff, B, C, D;

	//U
	Acoeff = edgesdY[0] * edgesdU[1] - edgesdU[0] * edgesdY[1];
	B = edgesdU[0] * edgesdX[1] - edgesdX[0] * edgesdU[1];
	C = edgesdX[0] * edgesdY[1] - edgesdY[0] * edgesdX[1];
	D = -1 * (Acoeff * verts[0][X] + B * verts[0][Y] + C * perspUV[0][U]);
	interpUV[U] = -1 * (Acoeff * pixel[X] + B * pixel[Y] + D) / C;

	//V
	Acoeff = edgesdY[0] * edgesdV[1] - edgesdV[0] * edgesdY[1];
	B = edgesdV[0] * edgesdX[1] - edgesdX[0] * edgesdV[1];
	C = edgesdX[0] * edgesdY[1] - edgesdY[0] * edgesdX[1];
	D = -1 * (Acoeff * verts[0][X] + B * verts[0][Y] + C * perspUV[0][V]);
	interpUV[V] = -1 * (Acoeff * pixel[X] + B * pixel[Y] + D) / C;

	//transform interpolated U,V into affine space u,v for function
	Vz = pixel[Z] / (MAXINT - pixel[Z]);
	affine[U] = interpUV[U] * (Vz + 1);
	affine[V] = interpUV[V] * (Vz + 1);

	return GZ_SUCCESS;
}

int GzRender::ComputeTextureIndex(GzCoord hit, GzTri triangle)
{
	if (interp_mode == GZ_FLAT)
	{
		//no textures for flat shading
		return GZ_SUCCESS;
	}

	GzTextureIndex affineUV;
	PerspectiveCorrection(triangle.uv, hit, triangle.imageVerts, affineUV);
	
	GzColor textureColor;
	tex_fun(affineUV[U], affineUV[V], textureColor);

	if (interp_mode == GZ_NORMALS)
	{
		memcpy((void*)Kd, (void*)textureColor, sizeof(GzColor));
		memcpy((void*)Ka, (void*)textureColor, sizeof(GzColor));
	}

	return GZ_SUCCESS;
}

int GzRender::ComputePixelNormal(int i, int j, GzTri triangle, GzCoord normal)
{
	GzCoord coeff[3] = { {triangle.uv[0][0] / (triangle.screenVerts[0][2] + 1), triangle.uv[0][1] / (triangle.screenVerts[0][2] + 1), 1},
			{triangle.uv[1][0] / (triangle.screenVerts[1][2] + 1), triangle.uv[1][1] / (triangle.screenVerts[1][2] + 1), 1},
			{triangle.uv[2][0] / (triangle.screenVerts[2][2] + 1), triangle.uv[2][1] / (triangle.screenVerts[2][2] + 1), 1} };
	float uv_plane[2][4];
	GzComputePlane(triangle.screenVerts, coeff, uv_plane);

	// try to compute global plane normal
	float Az, Bz, Cz, Dz;
	float a1 = triangle.screenVerts[1][0] - triangle.screenVerts[0][0];
	float b1 = triangle.screenVerts[1][1] - triangle.screenVerts[0][1];
	float c1 = triangle.screenVerts[1][2] - triangle.screenVerts[0][2];
	float a2 = triangle.screenVerts[2][0] - triangle.screenVerts[0][0];
	float b2 = triangle.screenVerts[2][1] - triangle.screenVerts[0][1];
	float c2 = triangle.screenVerts[2][2] - triangle.screenVerts[0][2];
	computePlane(triangle.screenVerts[0][0], triangle.screenVerts[0][1], triangle.screenVerts[0][2], a1, b1, c1, a2, b2, c2, Az, Bz, Cz, Dz);

	GzComputePlane(triangle.screenVerts, triangle.normals);

	normal[0] = (-planes[0][0] * i - planes[0][1] * j - planes[0][3]) / planes[0][2];
	normal[1] = (-planes[1][0] * i - planes[1][1] * j - planes[1][3]) / planes[1][2];
	normal[2] = (-planes[2][0] * i - planes[2][1] * j - planes[2][3]) / planes[2][2];
	float d = GzCoordLength(normal);
	normal[0] = normal[0] / d;
	normal[1] = normal[1] / d;
	normal[2] = normal[2] / d;

	return GZ_SUCCESS;
}

bool GzRender::GzIntersectColor(GzColor result, int depth, GzTri* exception)
{
	//find intersection
	GzTri* triangle;
	GzCoord hit;
	bool hasIntersection = GzFindFrontestIntersection_BSP(triangle, hit, exception);
	GzRay originRay;
	memcpy((void*)&originRay, (void*)&ray, sizeof(GzRay));

	if (!hasIntersection)
	{
		return false;
	}
	else
	{
		GzColor tmpResult = { 0, 0, 0 };
		//part1: color from lights
		GzColor lightShadingColor;
		GzCoord normal;
		ComputeHitPointNormal(hit, *triangle, normal);
		if (tex_fun != 0)
		{
			ComputeTextureIndex(hit, *triangle);
		}
		ComputeLightShading(triangle, hit, normal, lightShadingColor);
		
		for (int i = 0; i < 3; i++)
		{
			tmpResult[i] += lightShadingColor[i];
		}


		if (depth < 3 && triangle->isMirror)
		{
			//part2: color from reflection
			float vn = GzDot(normal, originRay.direction);
			if (vn > 0) vn = -vn;
			GzCoord reflectionRay = { -2 * vn * normal[0] + ray.direction[0], -2 * vn * normal[1] + ray.direction[1], -2 * vn * normal[2] + ray.direction[2] };
			memcpy((void*)ray.origin, (void*)hit, sizeof(GzCoord));
			memcpy((void*)ray.direction, (void*)reflectionRay, sizeof(GzCoord));
			normalize(ray.direction, ray.direction);

			GzColor reflectionColor;
			if (GzIntersectColor(reflectionColor, depth + 1, triangle))
			{
				for (int i = 0; i < 3; i++)
				{
					tmpResult[i] = reflectionColor[i];
				}
			}


			//todo: part3: color from refraction
		}
		

		//todo : add reflection and refraction color, clamp value
		for (int i = 0; i < 3; i++)
		{
			result[i] = clamp(0, 1, tmpResult[i]);
		}
		return true;
	}
}

int GzRender::ComputeHitPointNormal(GzCoord hit, GzTri triangle, GzCoord normal)
{
	GzCoord coeff[3] = { {triangle.uv[0][0] / (triangle.imageVerts[0][2] + 1), triangle.uv[0][1] / (triangle.imageVerts[0][2] + 1), 1},
			{triangle.uv[1][0] / (triangle.imageVerts[1][2] + 1), triangle.uv[1][1] / (triangle.imageVerts[1][2] + 1), 1},
			{triangle.uv[2][0] / (triangle.imageVerts[2][2] + 1), triangle.uv[2][1] / (triangle.imageVerts[2][2] + 1), 1} };
	float uv_plane[2][4];
	GzComputePlane(triangle.imageVerts, coeff, uv_plane);

	// try to compute global plane normal
	float Az, Bz, Cz, Dz;
	float a1 = triangle.imageVerts[1][0] - triangle.imageVerts[0][0];
	float b1 = triangle.imageVerts[1][1] - triangle.imageVerts[0][1];
	float c1 = triangle.imageVerts[1][2] - triangle.imageVerts[0][2];
	float a2 = triangle.imageVerts[2][0] - triangle.imageVerts[0][0];
	float b2 = triangle.imageVerts[2][1] - triangle.imageVerts[0][1];
	float c2 = triangle.imageVerts[2][2] - triangle.imageVerts[0][2];
	computePlane(triangle.imageVerts[0][0], triangle.imageVerts[0][1], triangle.imageVerts[0][2], a1, b1, c1, a2, b2, c2, Az, Bz, Cz, Dz);

	GzComputePlane(triangle.imageVerts, triangle.normals);

	normal[0] = (-planes[0][0] * hit[X] - planes[0][1] * hit[Y] - planes[0][3]) / planes[0][2];
	normal[1] = (-planes[1][0] * hit[X] - planes[1][1] * hit[Y] - planes[1][3]) / planes[1][2];
	normal[2] = (-planes[2][0] * hit[X] - planes[2][1] * hit[Y] - planes[2][3]) / planes[2][2];
	float d = GzCoordLength(normal);
	normal[0] = normal[0] / d;
	normal[1] = normal[1] / d;
	normal[2] = normal[2] / d;

	return GZ_SUCCESS;
}

void GzRender::ComputeLightShading(GzTri* intersectTriangle, GzCoord intersectPoint, GzCoord normal, GzColor result)
{
	// ray shoot from intersection point to light
	memcpy((void*)ray.origin, (void*)intersectPoint, sizeof(GzCoord));

	//eye vector should be adjusted according to the ray!
	GzCoord eye = { 0, 0, -1 };
	memcpy((void*)eye, (void*)ray.direction, sizeof(GzCoord));
	scalarProduct(eye, -1, eye);

	GzColor color = { 0, 0, 0 };
	GzCoord specular = { 0, 0, 0 }, diffuse = { 0, 0, 0 };
	for (int l = 0; l < numlights; l++)
	{
		//Check for shadow - omit light if intersection found
		memcpy((void*)ray.direction, (void*)lights[l].direction, sizeof(GzCoord));
		GzTri* tmpTriangle;
		GzCoord tmpHit;
		if (GzFindFrontestIntersection_BSP(tmpTriangle, tmpHit, intersectTriangle))//find triangle except for "intersectTriangle"(where the ray start from)
		{
			continue;
		}

		float nl = GzDot(normal, lights[l].direction);
		float ne = GzDot(normal, eye);
		if (nl >= 0 && ne >= 0)
		{
			// R dot E inside this or not?
			// do nothing
			// equal 0 is fine??
		}
		else if (nl <= 0 && ne <= 0)
		{
			normal[0] = -normal[0];
			normal[1] = -normal[1];
			normal[2] = -normal[2];
		}
		else
		{
			continue;
		}

		nl = GzDot(normal, lights[l].direction);
		GzCoord R = { 2 * nl * normal[0] - lights[l].direction[0], 2 * nl * normal[1] - lights[l].direction[1], 2 * nl * normal[2] - lights[l].direction[2] };
		float re = GzDot(R, eye);
		re = clamp(0, 1, re);

		specular[0] += lights[l].color[0] * pow(re, spec);
		specular[1] += lights[l].color[1] * pow(re, spec);
		specular[2] += lights[l].color[2] * pow(re, spec);
		diffuse[0] += lights[l].color[0] * nl;
		diffuse[1] += lights[l].color[1] * nl;
		diffuse[2] += lights[l].color[2] * nl;
	}

	color[0] = Ks[0] * specular[0] + Kd[0] * diffuse[0] + ambientlight.color[0] * Ka[0];
	color[1] = Ks[1] * specular[1] + Kd[1] * diffuse[1] + ambientlight.color[1] * Ka[1];
	color[2] = Ks[2] * specular[2] + Kd[2] * diffuse[2] + ambientlight.color[2] * Ka[2];
	color[0] = clamp(0, 1, color[0]);
	color[1] = clamp(0, 1, color[1]);
	color[2] = clamp(0, 1, color[2]);

	for (int i = 0; i < 3; i++)
	{
		result[i] = color[i];
	}
}