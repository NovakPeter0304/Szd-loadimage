#pragma once

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL.h>
#include <SDL_opengl.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

// mesh
#include "ObjParser_OGL3.h"

#include <imgui/imgui.h>


class Image {
public:
	/*Image(void);
	~Image(void);*/

	SDL_Surface* getSurface() { return surface; }
	void setSurface(SDL_Surface* surface) { this->surface = surface; }
	//void blintSurface(SDL_Surface* copy) { SDL_BlitSurface(copy, NULL, surface, NULL); }
	GLuint getTexture() { return texture; }
	void textureFromSurface();

	void Load(char* s);
	//void loadImage(char* s);

private:
	SDL_Surface* surface;
	GLuint texture;
};

class ImageModify {
public:
	static void plotLine(int x0, int y0, int x1, int y1, Image im);
	static void PutPixel32(int x, int y, Uint32 color, Image im);
	static Uint32 GetColor(int x, int y, Image im);

private:
	static void plotLineLow(int x0, int y0, int x1, int y1, Image im);
	static void plotLineHigh(int x0, int y0, int x1, int y1, Image im);
	static void PutPixel32_nolock(int x, int y, Uint32 color, Image im);
};

class RegularModify {
public:
	static void CursorPos(float offset);
	static Uint8 greyscale(Uint32 pixel, SDL_PixelFormat* format);
	//static void Resize(int, int);
};

class Zoom { //magnify
public:
	Zoom(void);
	void setZoom(char str1verified[]);

	void safeZoomHelpingMethod(Image im1);
	
	void ZoomMethod(Image im1);

private: //name it correctly!
	Image imZoom;
	int zoomW;
	int zoomH;
	float zoomTimes;
	int smallW;
	int smallH;
	bool smallChange;
	int bigW;
	int bigH;

};

class SSIM {
public:
	SSIM(void);
	void setSSIM(Image im1, Image im2, char str1verified[]);

	void Window2AfterColumn(Image im1, Image im2, bool updSSIM, bool currentError[], char str1verified[]);

	void plotLineSSIM(int x, int y, float slope, Image im1);
	
	SDL_Surface* SSIMSurface(Image img1, Image img2, int windowSize);
	struct colorsStruckt { Uint8 grey1, grey2, red1, red2, green1, green2, blue1, blue2, alpha1, alpha2; };
	float SSIMmethod(std::vector<std::vector<colorsStruckt>> window, int size, int currCol);

	void saveSSIM(char outstr2[]);

private:
	Image imSSIM1;
	Image imSSIM2;
	int ssimColor;
	float slope;
	int ssimSize;
	const float C1 = 0.01f, C2 = 0.03f;
	float ssimOsszeg;
};

class CMyApp
{
public:

	CMyApp(void);
	~CMyApp(void);

	//basic
	bool Init();
	void Clean();
	void Update();
	void Render();

	void Resize(int, int);

	//static??
	bool Verify(char strIn[],char strInv[],int noErr);

	//potencialremove
	void KeyboardDown(SDL_KeyboardEvent&);
	void KeyboardUp(SDL_KeyboardEvent&);
	void MouseMove(SDL_MouseMotionEvent&);
	void MouseDown(SDL_MouseButtonEvent&);
	void MouseUp(SDL_MouseButtonEvent&);
	void MouseWheel(SDL_MouseWheelEvent&);
	/*
	*/

	std::vector<Image> imageVec;

	//zoom
	void setZoom(char str1verified[]);
	void ZoomMethod(Image im1); //!!!!!!!!!!

	//ssim
	//void setSSIM(Image im1, Image im2, char str1verified[]);
	void plotLineSSIM(int x, int y, float slope, Image im1);
	SDL_Surface* SSIMSurface(Image img1, Image img2, int windowSize);
	struct colorsStruckt { Uint8 grey1, grey2, red1, red2, green1, green2, blue1, blue2, alpha1, alpha2; };
	float SSIMmethod(std::vector<std::vector<colorsStruckt>> window, int size, int currCol);

protected:

	// shaderekhez szükséges változók
	GLuint m_programID; // shaderek programja

	// transzformációs mátrixok
	glm::mat4 m_matWorld;
	glm::mat4 m_matView;
	glm::mat4 m_matProj;

	// Uniformok helye a shaderekben
	GLuint	m_loc_mvp;
	GLuint	m_loc_worldIT;
	GLuint	m_loc_world;
	GLuint	m_loc_texture;
	GLuint	m_loc_eye;

	// OpenGL-es dolgok
	GLuint m_vaoID; // vertex array object erõforrás azonosító
	GLuint m_vboID; // vertex buffer object erõforrás azonosító
	GLuint m_ibID;  // index buffer object erõforrás azonosító
	GLuint m_waterTextureID; // fájlból betöltött textúra azonosítója
	GLuint m_samplerID; // sampler object azonosító

	struct Vertex
	{
		glm::vec3 p; // pozíció
		glm::vec3 c; // szín
		glm::vec2 t; // textúra koordináták
	};

	// mesh adatok
	Mesh* m_mesh;

	glm::vec3 toDesc(float fi, float theta);
	float m_fi = M_PI / 2.0;
	float m_theta = M_PI / 2.0;

	glm::vec3 m_eye = glm::vec3(0, 5, 20);
	glm::vec3 m_at = m_eye + toDesc(m_fi, m_theta);
	glm::vec3 m_up = glm::vec3(0, 1, 0);
	glm::vec3 m_forward = glm::vec3(m_at - m_eye);
	float t = 1;

	enum Windows {
		WINDOW1,
		WINDOW2
	};

	enum ImageEnum {
		SEMMIenum,
		ZOOMenum,
		SSIMenum,
		SAVEenum
	};
	ImageEnum currentImageEnum;

	Image im1;
	Image im2;

	ImGuiWindowFlags window_flags;
	Windows currentWindow;
	bool currentError[5];

	char stradd[128];
	char straddverified[128];
	int selected1, selected2;

	char str1[128];
	char str1verified[128];
	char str2[128];
	char str2verified[128];
	char outstr[128];
	char outstr1[128];
	char outstr2[128];
	
	bool upd;
	bool updSSIM;

	Zoom zoomClass;
	SSIM ssimClass;

	
	//zoom
	Image imZoom;
	int zoomW;
	int zoomH;
	float zoomTimes;
	int smallW;
	int smallH;
	bool smallChange;
	int bigW;
	int bigH;

	//ssim
	Image imSSIM1;
	Image imSSIM2;
	int ssimColor;
	float slope;
	int ssimSize;
	const float C1 = 0.01f, C2 = 0.03f;
	float ssimOsszeg;

	bool ssimSelect;
};


