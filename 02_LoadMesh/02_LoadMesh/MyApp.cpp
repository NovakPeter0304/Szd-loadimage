#define STB_IMAGE_IMPLEMENTATION


#include "MyApp.h"
#include "GLUtils.hpp"

#include <math.h>
#include <imgui/imgui.h>
#include "stb_image.h"

//#define _CRT_SECURE_NO_WARNINGS

CMyApp::CMyApp(void)
{
	m_vaoID = 0;
	m_vboID = 0;
	m_programID = 0;
	m_waterTextureID = 0;
	m_samplerID = 0;

	m_mesh = 0;
}


CMyApp::~CMyApp(void)
{
}

bool CMyApp::Init()
{
	// törlési szín legyen kékes
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	glEnable(GL_CULL_FACE); // kapcsoljuk be a hatrafele nezo lapok eldobasat
	glEnable(GL_DEPTH_TEST); // mélységi teszt bekapcsolása (takarás)
	glCullFace(GL_BACK); // GL_BACK: a kamerától "elfelé" nézõ lapok, GL_FRONT: a kamera felé nézõ lapok

	//
	// geometria letrehozasa
	//

	Vertex vert[] =
	{
		//          x,  y, z               nx,ny,nz			 s, t
		{glm::vec3(-10, 0, -10), glm::vec3(0, 1, 0), glm::vec2(0, 0)},
		{glm::vec3(-10, 0,  10), glm::vec3(0, 1, 0), glm::vec2(0, 1)},
		{glm::vec3(10, 0, -10), glm::vec3(0, 1, 0), glm::vec2(1, 0)},
		{glm::vec3(10, 0,  10), glm::vec3(0, 1, 0), glm::vec2(1, 1)},
	};

	// indexpuffer adatai
	GLushort indices[] =
	{
		// 1. háromszög
		0,1,2,
		// 2. háromszög
		2,1,3,
	};

	// 1 db VAO foglalasa
	glGenVertexArrays(1, &m_vaoID);
	// a frissen generált VAO beallitasa aktívnak
	glBindVertexArray(m_vaoID);

	// hozzunk létre egy új VBO erõforrás nevet
	glGenBuffers(1, &m_vboID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID); // tegyük "aktívvá" a létrehozott VBO-t
	// töltsük fel adatokkal az aktív VBO-t
	glBufferData(GL_ARRAY_BUFFER,	// az aktív VBO-ba töltsünk adatokat
		sizeof(vert),		// ennyi bájt nagyságban
		vert,	// errõl a rendszermemóriabeli címrõl olvasva
		GL_STATIC_DRAW);	// úgy, hogy a VBO-nkba nem tervezünk ezután írni és minden kirajzoláskor felhasnzáljuk a benne lévõ adatokat


	// VAO-ban jegyezzük fel, hogy a VBO-ban az elsõ 3 float sizeof(Vertex)-enként lesz az elsõ attribútum (pozíció)
	glEnableVertexAttribArray(0); // ez lesz majd a pozíció
	glVertexAttribPointer(
		0,				// a VB-ben található adatok közül a 0. "indexû" attribútumait állítjuk be
		3,				// komponens szam
		GL_FLOAT,		// adatok tipusa
		GL_FALSE,		// normalizalt legyen-e
		sizeof(Vertex),	// stride (0=egymas utan)
		0				// a 0. indexû attribútum hol kezdõdik a sizeof(Vertex)-nyi területen belül
	);

	// a második attribútumhoz pedig a VBO-ban sizeof(Vertex) ugrás után sizeof(glm::vec3)-nyit menve újabb 3 float adatot találunk (szín)
	glEnableVertexAttribArray(1); // ez lesz majd a szín
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)(sizeof(glm::vec3)));

	// textúrakoordináták bekapcsolása a 2-es azonosítójú attribútom csatornán
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)(2 * sizeof(glm::vec3)));

	// index puffer létrehozása
	glGenBuffers(1, &m_ibID);
	// a VAO észreveszi, hogy bind-olunk egy index puffert és feljegyzi, hogy melyik volt ez!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0); // feltöltüttük a VAO-t, kapcsoljuk le
	glBindBuffer(GL_ARRAY_BUFFER, 0); // feltöltöttük a VBO-t is, ezt is vegyük le
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // feltöltöttük a VBO-t is, ezt is vegyük le

	//
	// shaderek betöltése
	//
	GLuint vs_ID = loadShader(GL_VERTEX_SHADER, "myVert.vert");
	GLuint fs_ID = loadShader(GL_FRAGMENT_SHADER, "myFrag.frag");

	// a shadereket tároló program létrehozása
	m_programID = glCreateProgram();

	// adjuk hozzá a programhoz a shadereket
	glAttachShader(m_programID, vs_ID);
	glAttachShader(m_programID, fs_ID);

	// VAO-beli attribútumok hozzárendelése a shader változókhoz
	// FONTOS: linkelés elõtt kell ezt megtenni!
	glBindAttribLocation(m_programID,	// shader azonosítója, amibõl egy változóhoz szeretnénk hozzárendelést csinálni
		0,				// a VAO-beli azonosító index
		"vs_in_pos");	// a shader-beli változónév
	glBindAttribLocation(m_programID, 1, "vs_in_norm");
	glBindAttribLocation(m_programID, 2, "vs_in_tex0");

	// illesszük össze a shadereket (kimenõ-bemenõ változók összerendelése stb.)
	glLinkProgram(m_programID);

	// linkeles ellenorzese
	GLint infoLogLength = 0, result = 0;

	glGetProgramiv(m_programID, GL_LINK_STATUS, &result);
	glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (GL_FALSE == result)
	{
		std::vector<char> ProgramErrorMessage(infoLogLength);
		glGetProgramInfoLog(m_programID, infoLogLength, NULL, &ProgramErrorMessage[0]);
		fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

		char* aSzoveg = new char[ProgramErrorMessage.size()];
		memcpy(aSzoveg, &ProgramErrorMessage[0], ProgramErrorMessage.size());

		std::cout << "[app.Init()] Sáder Huba panasza: " << aSzoveg << std::endl;

		delete aSzoveg;
	}

	// mar nincs ezekre szukseg
	glDeleteShader(vs_ID);
	glDeleteShader(fs_ID);

	//
	// egyéb inicializálás
	//

	// vetítési mátrix létrehozása
	m_matProj = glm::perspective(45.0f, 640 / 480.0f, 1.0f, 1000.0f);

	// shader-beli transzformációs mátrixok címének lekérdezése
	m_loc_mvp = glGetUniformLocation(m_programID, "MVP");

	m_loc_world = glGetUniformLocation(m_programID, "world");
	m_loc_worldIT = glGetUniformLocation(m_programID, "WorldIT");

	m_loc_texture = glGetUniformLocation(m_programID, "texture");
	m_loc_eye = glGetUniformLocation(m_programID, "eye_Pos");

	//
	// egyéb erõforrások betöltése
	//

	// textúra betöltése
	m_waterTextureID = TextureFromFile("texture.bmp");
	//m_waterTextureID = TextureFromFile("Large_cloud_over_Mexican_land.jpg");
	m_samplerID = genSampler();

	// mesh betoltese
	m_mesh = ObjParser::parse("Suzanne.obj");
	m_mesh->initBuffers();

	return true;
}

void CMyApp::Clean()
{
	delete m_mesh;
	glDeleteTextures(1, &m_waterTextureID);

	glDeleteSamplers(1, &m_samplerID);

	glDeleteBuffers(1, &m_vboID);
	glDeleteVertexArrays(1, &m_vaoID);

	glDeleteProgram(m_programID);
}

void CMyApp::Update()
{
	// nézeti transzformáció beállítása
	m_matView = glm::lookAt(m_eye,	// honnan nézzük a színteret
		m_at,	// a színtér melyik pontját nézzük
		m_up);	// felfelé mutató irány a világban
}

float f(float t) {
	//return (t - 1) * (t - 1);
	return t * t;
}

float df(float t) {
	return 2 * t;
}


void CMyApp::Render()
{
	// töröljük a frampuffert (GL_COLOR_BUFFER_BIT) és a mélységi Z puffert (GL_DEPTH_BUFFER_BIT)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform3f(glGetUniformLocation(m_programID, "eye_pos"), m_eye.x, m_eye.y, m_eye.z);

	switch (currentWindow) {
		case WINDOW_MAIN:
		MainWindow(); 
		break;
	case WINDOW1:
		Window1();
		break;
	case WINDOW2:
		Window2();
		break;
	}
}

void CMyApp::MainWindow() {
	ImGui::Begin("Main");
	if (ImGui::Button("Window1")) {
		currentWindow = WINDOW1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Window2")) {
		currentWindow = WINDOW2;
	}
	ImGui::End();
}

void CMyApp::Window1() {
	ImGui::Begin("First");
	ImGui::Text("A kep eleresi utvonala");
	ImGui::InputText("", str0, IM_ARRAYSIZE(str0));
	if (ImGui::Button("Verify")) {
		Verify(str0);
	}

	ImGuiIO& io = ImGui::GetIO();
		
	PictureData texture = LoadImage(str0);
	GLuint my_image_texture = texture.textureID;
	ImTextureID my_tex_id = (ImTextureID)(intptr_t)my_image_texture;
	float my_tex_w = texture.width;
	float my_tex_h = texture.height;

	//ImGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::Image(my_tex_id, ImVec2(my_tex_w, my_tex_h), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
	ImVec2 u0 = ImVec2(0,0);
	ImVec2 u1 = ImVec2(1, 1);
	if (ImGui::IsItemHovered())
	{
		float focus_sz = 32.0f;
		float focus_x = io.MousePos.x - pos.x - focus_sz * 0.5f; if (focus_x < 0.0f) focus_x = 0.0f; else if (focus_x > my_tex_w - focus_sz) focus_x = my_tex_w - focus_sz;
		float focus_y = io.MousePos.y - pos.y - focus_sz * 0.5f; if (focus_y < 0.0f) focus_y = 0.0f; else if (focus_y > my_tex_h - focus_sz) focus_y = my_tex_h - focus_sz;
		ImVec2 uv0 = ImVec2((focus_x) / my_tex_w, (focus_y) / my_tex_h);
		u0 = uv0;
		ImVec2 uv1 = ImVec2((focus_x + focus_sz) / my_tex_w, (focus_y + focus_sz) / my_tex_h);
		u1 = uv1;
	}
	ImGui::Image(my_tex_id, ImVec2(128, 128), u0, u1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
		
	if (ImGui::Button("Back")) {
		currentWindow = WINDOW_MAIN;
	}
	ImGui::End();
}

void CMyApp::Window2() {
	ImGui::Begin("Second");
	if (ImGui::Button("Back")) {
		currentWindow = WINDOW_MAIN;
	}
	ImGui::End();
}

CMyApp::PictureData CMyApp::LoadImage(const char* filename)
{
	PictureData result;

	int width, height, channels;

	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
	if (!data)
	{
		data = stbi_load("texture.bmp", &width, &height, &channels, 0);
	}
	if (!data)
	{
		result.textureID = 50;
		result.width = result.height = 50;
		return result;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	result.textureID = textureID;
	result.width = width;
	result.height = height;

	return result;
}

void CMyApp::Verify(char str[]) {
	
}

glm::vec3 CMyApp::toDesc(float fi, float theta) {
	return glm::vec3(sin(fi) * cos(theta), cos(fi), -sin(fi) * sin(theta));
}

void CMyApp::KeyboardDown(SDL_KeyboardEvent& key)
{
}

void CMyApp::KeyboardUp(SDL_KeyboardEvent& key)
{
}

void CMyApp::MouseMove(SDL_MouseMotionEvent& mouse)
{
}

void CMyApp::MouseDown(SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseWheel(SDL_MouseWheelEvent& wheel)
{
}

// a két paraméterbe az új ablakméret szélessége (_w) és magassága (_h) található
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);

	m_matProj = glm::perspective(45.0f,		// 90 fokos nyilasszog
		_w / (float)_h,	// ablakmereteknek megfelelo nezeti arany
		0.01f,			// kozeli vagosik
		100.0f);		// tavoli vagosik
}