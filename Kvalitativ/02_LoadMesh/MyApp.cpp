#define STB_IMAGE_IMPLEMENTATION

#define _CRT_SECURE_NO_WARNINGS

//#define IM_ALLOC(_SIZE)                     ImGui::MemAlloc(_SIZE)
//#define IM_FREE(_PTR)                       ImGui::MemFree(_PTR)

#include "MyApp.h"
#include "GLUtils.hpp"

#include <math.h>
#include <imgui/imgui.h>
#include "stb_image.h"

float f(float t) {
	//return (t - 1) * (t - 1);
	return t * t;
}

float df(float t) {
	return 2 * t;
}

CMyApp::CMyApp(void)
{
	m_vaoID = 0;
	m_vboID = 0;
	m_programID = 0;
	m_waterTextureID = 0;
	m_samplerID = 0;
	m_mesh = 0;

	window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar;
	upd = false;
	updSSIM = false;

	strcpy(str1, "C:/Users/User/Pictures/ac2.jpg");
	strcpy(str1verified, str1);
	strcpy(str2, "C:/Users/User/Pictures/ac.jpg");
	strcpy(str2verified, str2);
	strcpy(outstr1, "C:/Users/User/Pictures/save.png");
	strcpy(outstr2, "C:/Users/User/Pictures/ssim.png");

	Verify(str1, str1verified,0);
	Verify(str2, str2verified,1);

	im1.Load(str1verified);
	im1.textureFromSurface();
	//zoomClass.setZoom(str1verified);


	im2.Load(str2verified);
	im2.textureFromSurface();

	ssimClass.setSSIM(im1, im2, str1verified);

	currentWindow = WINDOW1;

	for (int i = 0;i< sizeof(currentError);i++) {
		currentError[i] = false;
	}

	//multpics !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	imageVec.push_back(im1);
	imageVec.push_back(im2);
	imageVec.push_back(im1);
	strcpy(stradd, "C:/Users/User/Pictures/ac2.jpg");
	strcpy(straddverified, stradd);
	Verify(stradd, straddverified, 0);

	selected1 = -1;
	selected2 = -1;

	currentImageEnum = SEMMIenum;
	strcpy(outstr, "C:/Users/User/Pictures/save.png");

	//zoom
	zoomW = 100;
	zoomH = 50;
	zoomTimes = 2.f;
	smallW = 0;
	smallH = 0;
	smallChange = true;
	bigW = 0;
	bigH = 0;

	//ssim
	ssimSize = 1;
	ssimColor = 0;
	slope = 1.3f;

	ssimSelect = true;
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

void CMyApp::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform3f(glGetUniformLocation(m_programID, "eye_pos"), m_eye.x, m_eye.y, m_eye.z);

	ImGuiIO io = ImGui::GetIO();
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::GetStyle().WindowRounding = 0.0f;

	ImGui::Begin("Kvalitativ osszehasonlitas", 0, window_flags); 
	ImFont* imFo = ImGui::GetFont();
	ImVec2 pos = ImGui::GetCursorScreenPos();

	ImGui::NewLine();
	imFo->Scale = 2.f;
	ImGui::PushFont(imFo);
	ImGui::Text("Kepek kvalitativ osszehasonlitasa");
	imFo->Scale = 1.f;
	ImGui::PopFont();
	ImGui::NewLine();

	//pics begin !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if (currentImageEnum==SEMMIenum) {

		ImGui::Text("A betoltendo kep eleresi utvonala:");
		ImGui::PushItemWidth(300);
		ImGui::InputText("##StrAdd", stradd, IM_ARRAYSIZE(stradd));
		ImGui::PopItemWidth();

		if (currentError[0]) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::Text("Hibas eleresi utvonal");
			ImGui::PopStyleColor();
		}
		else ImGui::NewLine();

		imFo->Scale = 1.3f;
		ImGui::PushFont(imFo);
		if (ImGui::Button("Betoltes##stradd", ImVec2(150, 50))) {
			if (Verify(stradd, straddverified, 0)) {
				Image imadd{};
				imadd.Load(straddverified);
				imadd.textureFromSurface();
				imageVec.push_back(imadd);

				ImGui::OpenPopup("Betoltes##Pop");
			}
		}
		imFo->Scale = 1.f;
		ImGui::PopFont();
	}

	int maxHeightVecIndex = 0;
	for (int i = 0; i < imageVec.size(); i++) {
		if (imageVec[i].getSurface()->h > imageVec[maxHeightVecIndex].getSurface()->h) {
			maxHeightVecIndex = i;
		}
	}

	if (currentImageEnum == SEMMIenum || (currentImageEnum == SSIMenum && ssimSelect))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		ImGui::BeginChild("scrolling", ImVec2(0, std::min(370 + 30 , imageVec[maxHeightVecIndex].getSurface()->h +30)), false, ImGuiWindowFlags_HorizontalScrollbar); //what if no elem!!!!!!!!!!!!!!!!!!!!
			for (int i = 0; i < imageVec.size(); i++) {
				ImGui::Image((void*)(intptr_t)imageVec[i].getTexture(), ImVec2(imageVec[i].getSurface()->w, imageVec[i].getSurface()->h));
				if (ImGui::IsItemClicked()) 
				{
					selected1 = i;
				}
				ImGui::SameLine();
			}
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
	}

	ImGui::NewLine();

	if (0 <= selected1 && selected1 < imageVec.size()) {
		if(currentImageEnum == SEMMIenum) ImGui::Image((void*)(intptr_t)imageVec[selected1].getTexture(), ImVec2(imageVec[selected1].getSurface()->w, imageVec[selected1].getSurface()->h));
	}
	else {
		ImGui::Text("*Kattints ra az egyik kepre, hogy kivalaszd azt!");
	}

	switch (currentImageEnum)
	{
	case CMyApp::SEMMIenum: //semmi

		if (0 <= selected1 && selected1 < imageVec.size()) {
			ImGui::Text("Choose what you want to do with the image!");
			ImGui::Columns(3, "mycolumns3", false); 
			imFo->Scale = 1.3f;
			ImGui::PushFont(imFo);
			if (ImGui::Button("Nagyitas", ImVec2(150, 50))) {
				imZoom.setSurface(imageVec[selected1].getSurface()); //potenciallyLoad !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				imZoom.textureFromSurface();
				currentImageEnum = ZOOMenum;
			}
			ImGui::NextColumn();
			if (ImGui::Button("SSIM", ImVec2(150, 50))) {
				currentImageEnum = SSIMenum;
			}
			ImGui::NextColumn();
			if (ImGui::Button("Mentes", ImVec2(150, 50))) {
				currentImageEnum = SAVEenum;
			}
			ImGui::NextColumn();
			ImGui::Columns(1);
			imFo->Scale = 1.f;
			ImGui::PopFont();
		}
		break;

	case CMyApp::ZOOMenum: //zoom

		ImGui::Separator();
		
		imFo = ImGui::GetFont();
		io = ImGui::GetIO();
		pos = ImGui::GetCursorScreenPos();

		ImGui::Image((void*)(intptr_t)imZoom.getTexture(), ImVec2(imZoom.getSurface()->w, imZoom.getSurface()->h));
		if (ImGui::IsItemHovered())
		{
			if (upd) {

				ZoomMethod(imageVec[selected1]);

				float focus_sz_x = 0.f;
				float focus_sz_y = 0.f;
				if (smallChange) {
					focus_sz_x = zoomW;
					focus_sz_y = zoomH;
				}
				else {
					focus_sz_x = zoomW * zoomTimes * -1;
					focus_sz_y = zoomH * zoomTimes * -1;
				}
				float focus_x = io.MousePos.x - pos.x - focus_sz_x * 0.5f;
				if (smallChange && focus_x < 0.0f) focus_x = 0.0f;
				else if (smallChange && focus_x > imageVec[selected1].getSurface()->w - focus_sz_x) focus_x = imageVec[selected1].getSurface()->w - focus_sz_x;
				else if (!smallChange && focus_x < zoomW * zoomTimes + 1) focus_x = zoomW * zoomTimes + 1;
				else if (!smallChange && focus_x > imageVec[selected1].getSurface()->w) focus_x = imageVec[selected1].getSurface()->w;
				float focus_y = io.MousePos.y - pos.y - focus_sz_y * 0.5f;
				if (smallChange && focus_y < 0.0f) focus_y = 0.0f;
				else if (smallChange && focus_y > imageVec[selected1].getSurface()->h - focus_sz_y) focus_y = imageVec[selected1].getSurface()->h - focus_sz_y;
				else if (!smallChange && focus_y < zoomH * zoomTimes + 1) focus_y = zoomH * zoomTimes + 1;
				else if (!smallChange && focus_y > imageVec[selected1].getSurface()->h) focus_y = imageVec[selected1].getSurface()->h;
				ImVec2 uv0 = ImVec2((focus_x) / imageVec[selected1].getSurface()->w, (focus_y) / imageVec[selected1].getSurface()->h);
				ImVec2 uv1 = ImVec2((focus_x + focus_sz_x) / imageVec[selected1].getSurface()->w, (focus_y + focus_sz_y) / imageVec[selected1].getSurface()->h);
				if (smallChange) {
					smallW = focus_x;
					smallH = focus_y;
				}
				else {
					bigW = imageVec[selected1].getSurface()->w - focus_x;
					bigH = imageVec[selected1].getSurface()->h - focus_y;
				}
			}
		}
		if (ImGui::IsItemClicked()) {
			upd = !upd;
		}

		ImGui::NewLine();

		imFo->Scale = 1.3f;
		ImGui::PushFont(imFo);
		RegularModify::CursorPos(1 * fmax(imageVec[selected1].getSurface()->w, 300) / 4 - 70);
		if (ImGui::Button("KisTeglalap", ImVec2(140, 40))) {
			upd = false;
			smallChange = true;
		}
		ImGui::SameLine();
		RegularModify::CursorPos(3 * fmax(imageVec[selected1].getSurface()->w, 300) / 4 - 70);
		if (ImGui::Button("Nagyteglalap", ImVec2(140, 40))) {
			upd = false;
			smallChange = false;
		}
		imFo->Scale = 1.f;
		ImGui::PopFont();

		ImGui::NewLine();

		ImGui::Text("Nagyitando terulet szelessege:");
		ImGui::SameLine();
		RegularModify::CursorPos(230);
		ImGui::PushItemWidth(fmax(imageVec[selected1].getSurface()->w, 300) - 230);
		ImGui::InputInt("##ZoomWidth", &zoomW, 0);
		ImGui::PopItemWidth();
		if (zoomW < 1) { zoomW = 1; }
		if (zoomW > fmin(imageVec[selected1].getSurface()->w / zoomTimes - 1, imageVec[selected1].getSurface()->w - 1)) { zoomW = fmin(imageVec[selected1].getSurface()->w / zoomTimes - 1, imageVec[selected1].getSurface()->w - 1); } //bigW = im1.getSurface()->w - zoomW * zoomTimes;
		ImGui::Text("Nagyitando terulet magassaga:");
		ImGui::SameLine();
		RegularModify::CursorPos(230);
		ImGui::PushItemWidth(fmax(imageVec[selected1].getSurface()->w, 300) - 230);
		ImGui::InputInt("##ZoomHeight", &zoomH, 0);
		ImGui::PopItemWidth();

		ImGui::Text("Nagyitas merteke:");
		ImGui::SameLine();
		RegularModify::CursorPos(150);
		ImGui::PushItemWidth(fmax(imageVec[selected1].getSurface()->w, 300) - 150);
		ImGui::SliderFloat("##ZoomTimes", &zoomTimes, 0.1f, 10.0f, "%.4f");
		ImGui::PopItemWidth();

		if (zoomH < 1) { zoomH = 1; }
		if (zoomH > fmin(imageVec[selected1].getSurface()->h / zoomTimes - 1, imageVec[selected1].getSurface()->h - 1)) { zoomH = fmin(imageVec[selected1].getSurface()->h / zoomTimes - 1, imageVec[selected1].getSurface()->h - 1); } //bigH = im1.getSurface()->h - zoomH * zoomTimes;
		
		imFo->Scale = 1.3f;
		ImGui::PushFont(imFo);
		if (ImGui::Button("Betoltes##magnify", ImVec2(150, 50))) {
			imageVec.push_back(imZoom);
			upd = false;
			currentImageEnum = SEMMIenum;
			selected1 = -1;

			ImGui::OpenPopup("Betoltes##Pop");
		}
		imFo->Scale = 1.f;
		ImGui::PopFont();

		break;

	case CMyApp::SSIMenum: //ssim

		ImGui::Separator();
		
		if(ssimSelect)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
			ImGui::BeginChild("scrollingssim", ImVec2(0, std::min(370 + 30, imageVec[maxHeightVecIndex].getSurface()->h + 30)), false, ImGuiWindowFlags_HorizontalScrollbar); //what if no elem!!!!!!!!!!!!!!!!!!!!
			for (int i = 0; i < imageVec.size(); i++) {
				ImGui::Image((void*)(intptr_t)imageVec[i].getTexture(), ImVec2(imageVec[i].getSurface()->w, imageVec[i].getSurface()->h));
				if (ImGui::IsItemClicked())
				{
					selected2 = i;
				}
				ImGui::SameLine();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar(2);

			ImGui::NewLine();

			ImGui::Image((void*)(intptr_t)imageVec[selected1].getTexture(), ImVec2(imageVec[selected1].getSurface()->w, imageVec[selected1].getSurface()->h));
			ImGui::SameLine();
			if (0 <= selected2 && selected2 < imageVec.size()) {
				ImGui::Image((void*)(intptr_t)imageVec[selected2].getTexture(), ImVec2(imageVec[selected2].getSurface()->w, imageVec[selected2].getSurface()->h));
			}
			else {
				ImGui::Text("*Kattints ra az egyik kepre, hogy kivalaszd azt!");
			}
		}

		ImGui::NewLine();

		if (0 <= selected2 && selected2 < imageVec.size() && 0 <= selected1 && selected1 < imageVec.size()) {

			ImGui::Text("Az atmenet meredeksege:");
			ImGui::SameLine();
			RegularModify::CursorPos(200);
			ImGui::PushItemWidth(200);
			ImGui::SliderFloat("##slope", &slope, 1.0f, 10.0f, "%.4f", 3.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Az SSIM szine: "); ImGui::SameLine();
			ImGui::RadioButton("Fekete-Feher", &ssimColor, 0); ImGui::SameLine();
			ImGui::RadioButton("Piros", &ssimColor, 1); ImGui::SameLine();
			ImGui::RadioButton("Zold", &ssimColor, 2);  ImGui::SameLine();
			ImGui::RadioButton("Kek", &ssimColor, 3);  ImGui::SameLine();
			ImGui::RadioButton("Osszes szin", &ssimColor, 4);

			ImGui::Text("Az SSIM szeletek merete:");
			ImGui::SameLine();
			RegularModify::CursorPos(200);
			ImGui::PushItemWidth(100);
			ImGui::InputInt("##Size", &ssimSize, 0);
			ImGui::PopItemWidth();
			if (ssimSize < 1) { //lookinto
				ssimSize = 1;
			}
			if (ssimSize > fmin(imageVec[selected2].getSurface()->w / 2, imageVec[selected2].getSurface()->h / 2)) {
				ssimSize = fmin(imageVec[selected2].getSurface()->w / 2, imageVec[selected2].getSurface()->h / 2);
			}

			imFo->Scale = 1.3f;
			ImGui::PushFont(imFo);
			if (ImGui::Button( (ssimSelect)?"Generalas":"Ujrageneralas", ImVec2(150, 50))) { //Generalas
				if (imageVec[selected1].getSurface()->w == imageVec[selected2].getSurface()->w && imageVec[selected1].getSurface()->h == imageVec[selected2].getSurface()->h) {
					ssimSelect = false;
					imSSIM1.setSurface(imageVec[selected1].getSurface()); //potenciallyLoad !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					imSSIM1.textureFromSurface();
					imSSIM2.setSurface(imageVec[selected1].getSurface()); //potenciallyLoad !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					imSSIM2.textureFromSurface();

					imSSIM1.setSurface(SSIMSurface(imageVec[selected1], imageVec[selected2], ssimSize));
					imSSIM1.textureFromSurface();
					imSSIM2.Load(str1verified);
					plotLineSSIM(imSSIM2.getSurface()->w / 2, imSSIM2.getSurface()->h / 2, slope, imageVec[selected1]);
					imSSIM2.textureFromSurface();
					currentError[4] = false;
				}
			}
			imFo->Scale = 1.f;
			ImGui::PopFont();
		}

		ImGui::NewLine();

		if (ssimSelect==false) {


			io = ImGui::GetIO();
			pos = ImGui::GetCursorScreenPos();
			ImGui::Image((void*)(intptr_t)imSSIM2.getTexture(), ImVec2(imSSIM2.getSurface()->w, imSSIM2.getSurface()->h));
			if (ImGui::IsItemHovered())
			{
				if (updSSIM) {
					plotLineSSIM(io.MousePos.x - pos.x, io.MousePos.y - pos.y, slope, imageVec[selected1]);
					imSSIM2.textureFromSurface();
				}
			}
			if (ImGui::IsItemClicked()) {
				updSSIM = !updSSIM;
			}

			ImGui::Text("Osszesitett SSIM ertek: %f", ssimOsszeg);

			imFo->Scale = 1.3f;
			ImGui::PushFont(imFo);
			if (ImGui::Button("Betoltes##ssim", ImVec2(150, 50))) {
				imageVec.push_back(imSSIM1);
				updSSIM = false;
				currentImageEnum = SEMMIenum;
				selected1 = -1;
				selected2 = -1;

				ImGui::OpenPopup("Betoltes##Pop");
			}
			imFo->Scale = 1.f;
			ImGui::PopFont();

		}

		break;

	case CMyApp::SAVEenum: //save

		ImGui::Separator();
		ImGui::Image((void*)(intptr_t)imageVec[selected1].getTexture(), ImVec2(imageVec[selected1].getSurface()->w, imageVec[selected1].getSurface()->h));

		ImGui::PushItemWidth(300);
		ImGui::InputText("##SavePath", outstr1, IM_ARRAYSIZE(outstr));
		ImGui::PopItemWidth();
		if (currentError[2]) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::Text("Hibas mentesi utvonal");
			ImGui::PopStyleColor();
		}
		imFo->Scale = 1.3f;
		ImGui::PushFont(imFo);
		if (ImGui::Button("Mentes", ImVec2(150, 50))) {
			struct stat sb;
			if (stat(outstr, &sb) != 0) {
				currentError[2] = true;
			}
			else {
				IMG_SavePNG(imageVec[selected1].getSurface(), outstr);
				currentImageEnum = SEMMIenum;
				selected1 = -1;
				currentError[2] = false;

				ImGui::OpenPopup("Mentes##Pop");
			}
		}
		imFo->Scale = 1.f;
		ImGui::PopFont();
		break;

	default:
		break;
	}

	if (currentImageEnum != SEMMIenum) {
		imFo->Scale = 1.3f;
		ImGui::PushFont(imFo);
		if (ImGui::Button("Vissza", ImVec2(150, 50))) {
			upd = false;
			updSSIM = false;
			ssimSelect = true;
			currentImageEnum = SEMMIenum;
			selected1 = -1;
			selected2 = -1;
		}
		imFo->Scale = 1.f;
		ImGui::PopFont();
	}

	
	ImGui::NewLine();

	//pics end !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 1.0f, 1.0f));
	//ImGui::PopStyleColor();

	if (ImGui::BeginPopupModal("Betoltes##Pop", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Kep sikeresen betoltve");
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Mentes##Pop", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Kep sikeresen elmentve");
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	ImGui::End();
}

bool CMyApp::Verify(char* filePath , char* filePathv, int noErr) {

	SDL_Surface* imageSurface = IMG_Load(filePath);
	if (!imageSurface) {
		imageSurface = IMG_Load("texture.bmp");
		strcpy(filePathv, "texture.bmp");
		currentError[noErr] = true;
	}
	else {
		strcpy(filePathv, filePath);
		currentError[noErr] = false;
	}
	if (!imageSurface) {
		printf("Failed to load image (also backup image got deleted): %s\n", IMG_GetError());
		return false;
	}
	SDL_FreeSurface(imageSurface);
	return true;
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


/* Image::Image(void){}
   Image::~Image(void){} */

void Image::textureFromSurface() {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLenum format = GL_RGBA;
	if (surface->format->BytesPerPixel == 3) {
		format = GL_RGB;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	texture = textureID;
}

void Image::Load(char* s) {
	surface = IMG_Load(s);
}

/*void Image::loadImage(char* s) {
	SDL_FreeSurface(surface);
	Load(s);
	textureFromSurface();
}*/


void ImageModify::plotLine(int x0, int y0, int x1, int y1, Image im) {
	if (abs(y1 - y0) < abs(x1 - x0)) {
		if (x0 > x1) {
			plotLineLow(x1, y1, x0, y0, im);
		}
		else {
			plotLineLow(x0, y0, x1, y1, im);
		}
	}
	else {
		if (y0 > y1) {
			plotLineHigh(x1, y1, x0, y0, im);
		}
		else {
			plotLineHigh(x0, y0, x1, y1, im);
		}
	}
}

void ImageModify::plotLineLow(int x0, int y0, int x1, int y1, Image im){
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;
	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}

	int D = (2 * dy) - dx;
	int y = y0;

	for (int x = x0; x < x1 ; x++){
		PutPixel32(x, y, (255 << 24) | (0 << 16) | (0 << 8) | 255, im);

		if (D > 0){
			y = y + yi;
			D = D + (2 * (dy - dx));
		}
		else{
			D = D + 2 * dy;
		}
	}
}

void ImageModify::plotLineHigh(int x0,int y0,int x1,int y1, Image im){
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;
	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}

	int D = (2 * dx) - dy;
	int x = x0;

	for (int y=y0;y<y1;y++){
		PutPixel32(x, y, (255 << 24) | (0 << 16) | (0 << 8) | 255, im);

		if (D > 0) {
			x = x + xi;
			D = D + (2 * (dx - dy));
		}
		else {
			D = D + 2 * dx;
		}
	}
}

void ImageModify::PutPixel32(int x, int y, Uint32 color, Image im)
{
	if (SDL_MUSTLOCK(im.getSurface()))
		SDL_LockSurface(im.getSurface());
	PutPixel32_nolock(x, y, color,im);
	if (SDL_MUSTLOCK(im.getSurface()))
		SDL_UnlockSurface(im.getSurface());
}

void ImageModify::PutPixel32_nolock(int x, int y, Uint32 color, Image im)
{
	Uint8* pixel = (Uint8*)im.getSurface()->pixels;
	pixel += (y * im.getSurface()->pitch) + x * im.getSurface()->format->BytesPerPixel;
	*((Uint32*)pixel) = color;
}

Uint32 ImageModify::GetColor(int x, int y,Image im) {
	return *(Uint32*)((Uint8*)im.getSurface()->pixels + y * im.getSurface()->pitch + x * im.getSurface()->format->BytesPerPixel);
}


void RegularModify::CursorPos(float offset) {
	ImGui::SetCursorPosX(0);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
}

Uint8 RegularModify::greyscale(Uint32 pixel, SDL_PixelFormat* format) {
	Uint8 r, g, b;
	SDL_GetRGB(pixel, format, &r, &g, &b);
	return 0.299f * r + 0.587f * g + 0.114f * b;
}

void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);

	m_matProj = glm::perspective(45.0f,		// 90 fokos nyilasszog
		_w / (float)_h,	// ablakmereteknek megfelelo nezeti arany
		0.01f,			// kozeli vagosik
		100.0f);		// tavoli vagosik
}



void CMyApp::ZoomMethod(Image im1) {
	Uint32 red = (255 << 24) | (0 << 16) | (0 << 8) | 255;

	imZoom.setSurface(im1.getSurface()); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! maybe setit to equal size than pixel by pixel?
	//imZoom.blintSurface(im1.getSurface());
	//imZoom.textureFromSurface();

	for (int i = 0; i < imZoom.getSurface()->w; i++) {
		for (int j = 0; j < imZoom.getSurface()->h; j++) {
			ImageModify::PutPixel32(i, j, ImageModify::GetColor(i, j, im1), imZoom);
		}
	}

	for (int i = zoomW * zoomTimes; i > 0; i--) {
		for (int j = zoomH * zoomTimes; j > 0; j--) {
			if (i == 1 || i == std::floor(zoomW * zoomTimes) || j == 1 || j == std::floor(zoomH * zoomTimes)) {
				ImageModify::PutPixel32(imZoom.getSurface()->w - i - bigW - 1, imZoom.getSurface()->h - j - bigH - 1, red, imZoom);
			}
			else {
				ImageModify::PutPixel32(imZoom.getSurface()->w - i - bigW - 1, imZoom.getSurface()->h - j - bigH - 1, ImageModify::GetColor(int((zoomW * zoomTimes - i) / zoomTimes + smallW), int((zoomH * zoomTimes - j) / zoomTimes + smallH), imZoom), imZoom);
			}
		}
	}
	for (int i = 0 + smallW; i < zoomW + smallW; i++) {
		for (int j = 0 + smallH; j < zoomH + smallH; j++) {
			if (i == 0 + smallW || j == 0 + smallH || i == zoomW + smallW - 1 || j == zoomH + smallH - 1) {
				ImageModify::PutPixel32(i, j, red, imZoom);
			}
		}
	}
	ImVec2 small1 = ImVec2(smallW, zoomH + smallH);
	ImVec2 small2 = ImVec2(zoomW + smallW, smallH);
	ImVec2 big1 = ImVec2(imZoom.getSurface()->w - bigW - zoomW * zoomTimes, imZoom.getSurface()->h - bigH);
	ImVec2 big2 = ImVec2(imZoom.getSurface()->w - bigW, imZoom.getSurface()->h - bigH - zoomH * zoomTimes);

	int tempSmall1y = small1.y, tempBig1y = big1.y;

	if (small1.x > big1.x && small1.y < big1.y || small1.x < big1.x && small1.y > big1.y) {
		small1.y = small2.y;
		big1.y = big2.y;
	}

	if (small2.x > big2.x && small2.y < big2.y || small2.x < big2.x && small2.y > big2.y) {
		small2.y = tempSmall1y;
		big2.y = tempBig1y;
	}

	int x0 = small1.x, x1 = big1.x, y0 = small1.y, y1 = big1.y;
	ImageModify::plotLine(x0, y0, x1, y1, imZoom);
	x0 = small2.x, x1 = big2.x, y0 = small2.y, y1 = big2.y;
	ImageModify::plotLine(x0, y0, x1, y1, imZoom);

	imZoom.textureFromSurface();
}

float CMyApp::SSIMmethod(std::vector<std::vector<colorsStruckt>> window, int size, int currCol) {

	float mean1 = 0.f;
	float mean2 = 0.f;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			switch (currCol)
			{
			case 0:
				mean1 += window[i][j].grey1;
				mean2 += window[i][j].grey2;
				break;
			case 1:
				mean1 += window[i][j].red1;
				mean2 += window[i][j].red2;
				break;
			case 2:
				mean1 += window[i][j].green1;
				mean2 += window[i][j].green2;
				break;
			case 3:
				mean1 += window[i][j].blue1;
				mean2 += window[i][j].blue2;
				break;
			}


		}
	}
	mean1 /= size;
	mean2 /= size;

	float var1 = 0.f;
	float var2 = 0.f;
	float covar = 0.f;

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			switch (currCol)
			{
			case 0:
				var1 += (window[i][j].grey1 - mean1) * (window[i][j].grey1 - mean1);
				var2 += (window[i][j].grey2 - mean2) * (window[i][j].grey2 - mean2);
				covar += (window[i][j].grey1 - mean1) * (window[i][j].grey2 - mean2);
				break;
			case 1:
				var1 += (window[i][j].red1 - mean1) * (window[i][j].red1 - mean1);
				var2 += (window[i][j].red2 - mean2) * (window[i][j].red2 - mean2);
				covar += (window[i][j].red1 - mean1) * (window[i][j].red2 - mean2);
				break;
			case 2:
				var1 += (window[i][j].green1 - mean1) * (window[i][j].green1 - mean1);
				var2 += (window[i][j].green2 - mean2) * (window[i][j].green2 - mean2);
				covar += (window[i][j].green1 - mean1) * (window[i][j].green2 - mean2);
				break;
			case 3:
				var1 += (window[i][j].blue1 - mean1) * (window[i][j].blue1 - mean1);
				var2 += (window[i][j].blue2 - mean2) * (window[i][j].blue2 - mean2);
				covar += (window[i][j].blue1 - mean1) * (window[i][j].blue2 - mean2);
				break;

			}
		}
	}
	var1 /= size;
	var2 /= size;
	covar /= size;

	return ((2 * mean1 * mean2 + C1) * (2 * covar + C2)) / ((mean1 * mean1 + mean2 * mean2 + C1) * (var1 + var2 + C2));
}
SDL_Surface* CMyApp::SSIMSurface(Image img1, Image img2, int windowSize) {

	SDL_Surface* ssimSurface = SDL_CreateRGBSurface(0, img1.getSurface()->w, img1.getSurface()->h, img1.getSurface()->format->BitsPerPixel, 0, 0, 0, 0);
	Image seged;
	seged.setSurface(ssimSurface);

	//SDL_LockSurface(ssimSurface);

	std::vector<std::vector<colorsStruckt>> colorVect(windowSize, std::vector<colorsStruckt>(windowSize));

	//std::vector<std::vector<Uint8>> grey1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> grey2(windowSize, std::vector<Uint8>(windowSize));

	//std::vector<std::vector<Uint8>> red1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> red2(windowSize, std::vector<Uint8>(windowSize));

	//std::vector<std::vector<Uint8>> green1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> green2(windowSize, std::vector<Uint8>(windowSize));

	//std::vector<std::vector<Uint8>> blue1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> blue2(windowSize, std::vector<Uint8>(windowSize));

	ssimOsszeg = 0.f;
	int num = 0;

	for (int x = 0; x < seged.getSurface()->w; x += windowSize) {
		for (int y = 0; y < seged.getSurface()->h; y += windowSize) {

			for (int i = 0; i < windowSize; i++) {
				for (int j = 0; j < windowSize; j++) {

					Uint8 r, g, b, a;
					Uint8 r2, g2, b2, a2;
					SDL_GetRGBA(ImageModify::GetColor(x + i, y + j, img1), img1.getSurface()->format, &r, &g, &b, &a);
					SDL_GetRGBA(ImageModify::GetColor(x + i, y + j, img2), img2.getSurface()->format, &r2, &g2, &b2, &a2);

					switch (ssimColor)
					{
					case 0:
						colorVect[i][j].grey1 = RegularModify::greyscale(ImageModify::GetColor(x + i, y + j, img1), img1.getSurface()->format);
						colorVect[i][j].grey2 = RegularModify::greyscale(ImageModify::GetColor(x + i, y + j, img2), img2.getSurface()->format);
						break;
					case 1:
						colorVect[i][j].red1 = r;
						colorVect[i][j].red2 = r2;
						break;
					case 2:
						colorVect[i][j].green1 = g;
						colorVect[i][j].green2 = g2;
						break;
					case 3:
						colorVect[i][j].blue1 = b;
						colorVect[i][j].blue2 = b2;
						break;
					case 4:
						colorVect[i][j].red1 = r;
						colorVect[i][j].red2 = r2;
						colorVect[i][j].green1 = g;
						colorVect[i][j].green2 = g2;
						colorVect[i][j].blue1 = b;
						colorVect[i][j].blue2 = b2;
						break;
					}

					colorVect[i][j].alpha1 = a;
					colorVect[i][j].alpha2 = a2;

				}
			}
			float ssimGrey = 0.f;
			float ssimRed = 0.f;
			float ssimGreen = 0.f;
			float ssimBlue = 0.f;
			Uint8 ymc, rU, gU, bU;

			switch (ssimColor)
			{
			case 0:
				ssimGrey = SSIMmethod(colorVect, windowSize, 0);
				ssimOsszeg += ssimGrey;
				num++;
				ymc = (Uint8)((ssimGrey + 1.0) * 255 / 2);
				break;
			case 1:
				ssimRed = SSIMmethod(colorVect, windowSize, 1);
				ssimOsszeg += ssimRed;
				num++;
				ymc = (Uint8)((ssimRed + 1.0) * 255 / 2);
				break;
			case 2:
				ssimGreen = SSIMmethod(colorVect, windowSize, 2);
				ssimOsszeg += ssimGreen;
				num++;
				ymc = (Uint8)((ssimGreen + 1.0) * 255 / 2);
				break;
			case 3:
				ssimBlue = SSIMmethod(colorVect, windowSize, 3);
				ssimOsszeg += ssimBlue;
				num++;
				ymc = (Uint8)((ssimBlue + 1.0) * 255 / 2);
				break;
			case 4:
				ssimRed = SSIMmethod(colorVect, windowSize, 1);
				ssimOsszeg += ssimRed;
				num++;
				ssimGreen = SSIMmethod(colorVect, windowSize, 2);
				ssimOsszeg += ssimGreen;
				num++;
				ssimBlue = SSIMmethod(colorVect, windowSize, 3);
				ssimOsszeg += ssimBlue;
				num++;
				rU = (Uint8)((ssimRed + 1.0) * 255 / 2);
				gU = (Uint8)((ssimGreen + 1.0) * 255 / 2);
				bU = (Uint8)((ssimBlue + 1.0) * 255 / 2);
				break;
			}
			//red 0 0 255   green 0 255 0   blue 255 0 0     
			//r = -y -m	, g = -y -c , b = -m -c
			//Uint8 ymc = (Uint8)((ssimValue + 1.0) * 255 / 2); 

			for (int i = 0; i < windowSize; i++) {
				for (int j = 0; j < windowSize; j++) {
					if (x + i < seged.getSurface()->w && y + j < seged.getSurface()->h) {

						switch (ssimColor)
						{
						case 0:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, ymc, ymc, ymc, colorVect[i][j].alpha1), seged);
							break;
						case 1:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, ymc, ymc, 255, colorVect[i][j].alpha1), seged);
							break;
						case 2:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, ymc, 255, ymc, colorVect[i][j].alpha1), seged);
							break;
						case 3:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, 255, ymc, ymc, colorVect[i][j].alpha1), seged);
							break;
						case 4:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, bU, gU, rU, colorVect[i][j].alpha1), seged);
							break;
						}

					}
				}
			}
		}
	}

	ssimOsszeg = ssimOsszeg / num;

	//SDL_UnlockSurface(ssimSurface); seperable filter matlab 

	return ssimSurface;
}
void CMyApp::plotLineSSIM(int x, int y, float slope, Image im1) {

	//ImVec2 first, second;

	for (int i = 0; i < imSSIM2.getSurface()->w; i++) {
		for (int j = 0; j < imSSIM2.getSurface()->h; j++) {
			if (j < slope * (i - x) + y) {
				ImageModify::PutPixel32(i, j, ImageModify::GetColor(i, j, imSSIM1), imSSIM2);
			}
			else {
				ImageModify::PutPixel32(i, j, ImageModify::GetColor(i, j, im1), imSSIM2);
			}

			/*if ((j == 0 || i == 0) && j == (int)(slope * (i - x) + y)) {
				first.x = i;
				first.y = j;
			}
			if ((j == ssimSurface2->h - 1 || i == ssimSurface2->w - 1) && j  == (int)(slope * (i - x) + y)) {
				second.x = i;
				second.y = j;
			}*/
		}
	}

	//plotLine(first.x,first.y,second.x,second.y , ssimSurface2);
}


Zoom::Zoom(void) {
	zoomW = 100;
	zoomH = 50;
	zoomTimes = 2.f;
	smallW = 0;
	smallH = 0;
	smallChange = true;
	bigW = 0;
	bigH = 0;
}/*

void Zoom::setZoom(char str1verified[]) {
	imZoom.Load(str1verified);
	imZoom.textureFromSurface();
}

void Zoom::safeZoomHelpingMethod(Image im1) {
	if (zoomW > fmin(im1.getSurface()->w / zoomTimes - 1, im1.getSurface()->w - 1)) { zoomW = fmin(im1.getSurface()->w / zoomTimes - 1, im1.getSurface()->w - 1); } // bigW = im1.getSurface()->w - zoomW * zoomTimes;
	if (zoomH > fmin(im1.getSurface()->h / zoomTimes - 1, im1.getSurface()->h - 1)) { zoomH = fmin(im1.getSurface()->h / zoomTimes - 1, im1.getSurface()->h - 1); } // bigH = im1.getSurface()->h - zoomH * zoomTimes;
}
*/

SSIM::SSIM(void) {
	ssimSize = 1;
	ssimColor = 0;
	slope = 1.3f;
}

float SSIM::SSIMmethod(std::vector<std::vector<colorsStruckt>> window, int size, int currCol) {

	float mean1 = 0.f;
	float mean2 = 0.f;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			switch (currCol)
			{
			case 0:
				mean1 += window[i][j].grey1;
				mean2 += window[i][j].grey2;
				break;
			case 1:
				mean1 += window[i][j].red1;
				mean2 += window[i][j].red2;
				break;
			case 2:
				mean1 += window[i][j].green1;
				mean2 += window[i][j].green2;
				break;
			case 3:
				mean1 += window[i][j].blue1;
				mean2 += window[i][j].blue2;
				break;
			}


		}
	}
	mean1 /= size;
	mean2 /= size;

	float var1 = 0.f;
	float var2 = 0.f;
	float covar = 0.f;

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			switch (currCol)
			{
			case 0:
				var1 += (window[i][j].grey1 - mean1) * (window[i][j].grey1 - mean1);
				var2 += (window[i][j].grey2 - mean2) * (window[i][j].grey2 - mean2);
				covar += (window[i][j].grey1 - mean1) * (window[i][j].grey2 - mean2);
				break;
			case 1:
				var1 += (window[i][j].red1 - mean1) * (window[i][j].red1 - mean1);
				var2 += (window[i][j].red2 - mean2) * (window[i][j].red2 - mean2);
				covar += (window[i][j].red1 - mean1) * (window[i][j].red2 - mean2);
				break;
			case 2:
				var1 += (window[i][j].green1 - mean1) * (window[i][j].green1 - mean1);
				var2 += (window[i][j].green2 - mean2) * (window[i][j].green2 - mean2);
				covar += (window[i][j].green1 - mean1) * (window[i][j].green2 - mean2);
				break;
			case 3:
				var1 += (window[i][j].blue1 - mean1) * (window[i][j].blue1 - mean1);
				var2 += (window[i][j].blue2 - mean2) * (window[i][j].blue2 - mean2);
				covar += (window[i][j].blue1 - mean1) * (window[i][j].blue2 - mean2);
				break;

			}
		}
	}
	var1 /= size;
	var2 /= size;
	covar /= size;

	return ((2 * mean1 * mean2 + C1) * (2 * covar + C2)) / ((mean1 * mean1 + mean2 * mean2 + C1) * (var1 + var2 + C2));
}

void SSIM::Window2AfterColumn(Image im1, Image im2, bool updSSIM, bool currentError[], char str1verified[]) {
	ImFont* imFo = ImGui::GetFont();

	ImGui::NewLine();

	ImGui::Text("Az atmenet meredeksege:");
	ImGui::SameLine();
	RegularModify::CursorPos(200);
	ImGui::PushItemWidth(200);
	ImGui::SliderFloat("##slope", &slope, 1.0f, 10.0f, "%.4f", 3.0f);
	ImGui::PopItemWidth();

	ImGui::Text("Az SSIM szine: "); ImGui::SameLine();
	ImGui::RadioButton("Fekete-Feher", &ssimColor, 0); ImGui::SameLine();
	ImGui::RadioButton("Piros", &ssimColor, 1); ImGui::SameLine();
	ImGui::RadioButton("Zold", &ssimColor, 2);  ImGui::SameLine();
	ImGui::RadioButton("Kek", &ssimColor, 3);  ImGui::SameLine();
	ImGui::RadioButton("Osszes szin", &ssimColor, 4);

	ImGui::Text("Az SSIM szeletek merete:");
	ImGui::SameLine();
	RegularModify::CursorPos(200);
	ImGui::PushItemWidth(100);
	ImGui::InputInt("##Size", &ssimSize,0);
	ImGui::PopItemWidth();
	if (ssimSize < 1) {
		ssimSize = 1;
	}
	if (ssimSize > fmin(im1.getSurface()->w / 2, im1.getSurface()->h / 2)) {
		ssimSize = fmin(im1.getSurface()->w / 2, im1.getSurface()->h / 2);
	}

	ImGui::NewLine();

	imFo->Scale = 1.3f;
	ImGui::PushFont(imFo);
	if (ImGui::Button("SSIM",ImVec2(150,50))) {
		updSSIM = false;

		if (im1.getSurface()->h == im2.getSurface()->h && im1.getSurface()->w == im2.getSurface()->w) {
			imSSIM1.setSurface(SSIMSurface(im1, im2, ssimSize));
			imSSIM1.textureFromSurface();
			imSSIM2.Load(str1verified);
			plotLineSSIM(imSSIM2.getSurface()->w / 2, imSSIM2.getSurface()->h / 2, slope,im1);
			imSSIM2.textureFromSurface();
			currentError[4] = false;
		}
		else currentError[4] = true;
	}
	imFo->Scale = 1.f;
	ImGui::PopFont();

	if (currentError[4]) {
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
		ImGui::Text("A kepek meretenek meg kell egyeznie");
		ImGui::PopStyleColor();
	}

	ImGui::NewLine();

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::Image((void*)(intptr_t)imSSIM2.getTexture(), ImVec2(imSSIM2.getSurface()->w, imSSIM2.getSurface()->h));
	if (ImGui::IsItemHovered())
	{
		if (updSSIM) {
			plotLineSSIM(io.MousePos.x - pos.x, io.MousePos.y - pos.y, slope,im1);
			imSSIM2.textureFromSurface();
		}
	}
	if (ImGui::IsItemClicked()) {
		updSSIM = !updSSIM;
	}

	ImGui::Text("Osszesitett SSIM ertek: %f", ssimOsszeg);

}

SDL_Surface* SSIM::SSIMSurface(Image img1, Image img2, int windowSize) {

	SDL_Surface* ssimSurface = SDL_CreateRGBSurface(0, img1.getSurface()->w, img1.getSurface()->h, img1.getSurface()->format->BitsPerPixel, 0, 0, 0, 0);
	Image seged;
	seged.setSurface(ssimSurface);

	//SDL_LockSurface(ssimSurface);

	std::vector<std::vector<colorsStruckt>> colorVect(windowSize, std::vector<colorsStruckt>(windowSize));

	//std::vector<std::vector<Uint8>> grey1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> grey2(windowSize, std::vector<Uint8>(windowSize));

	//std::vector<std::vector<Uint8>> red1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> red2(windowSize, std::vector<Uint8>(windowSize));

	//std::vector<std::vector<Uint8>> green1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> green2(windowSize, std::vector<Uint8>(windowSize));

	//std::vector<std::vector<Uint8>> blue1(windowSize, std::vector<Uint8>(windowSize));
	//std::vector<std::vector<Uint8>> blue2(windowSize, std::vector<Uint8>(windowSize));

	ssimOsszeg = 0.f;
	int num = 0;

	for (int x = 0; x < seged.getSurface()->w; x += windowSize) {
		for (int y = 0; y < seged.getSurface()->h; y += windowSize) {

			for (int i = 0; i < windowSize; i++) {
				for (int j = 0; j < windowSize; j++) {

					Uint8 r, g, b, a;
					Uint8 r2, g2, b2, a2;
					SDL_GetRGBA(ImageModify::GetColor(x + i, y + j, img1), img1.getSurface()->format, &r, &g, &b,&a);
					SDL_GetRGBA(ImageModify::GetColor(x + i, y + j, img2), img2.getSurface()->format, &r2, &g2, &b2,&a2);

					switch (ssimColor)
					{
					case 0:
						colorVect[i][j].grey1 = RegularModify::greyscale(ImageModify::GetColor(x + i, y + j, img1), img1.getSurface()->format);
						colorVect[i][j].grey2 = RegularModify::greyscale(ImageModify::GetColor(x + i, y + j, img2), img2.getSurface()->format);
						break;
					case 1:
						colorVect[i][j].red1 = r;
						colorVect[i][j].red2 = r2;
						break;
					case 2:
						colorVect[i][j].green1 = g;
						colorVect[i][j].green2 = g2;
						break;
					case 3:
						colorVect[i][j].blue1 = b;
						colorVect[i][j].blue2 = b2;
						break;
					case 4:
						colorVect[i][j].red1 = r;
						colorVect[i][j].red2 = r2;
						colorVect[i][j].green1 = g;
						colorVect[i][j].green2 = g2;
						colorVect[i][j].blue1 = b;
						colorVect[i][j].blue2 = b2;
						break;
					}

					colorVect[i][j].alpha1 = a;
					colorVect[i][j].alpha2 = a2;
					
				}
			}
			float ssimGrey = 0.f;
			float ssimRed = 0.f;
			float ssimGreen = 0.f;
			float ssimBlue = 0.f;
			Uint8 ymc,rU,gU,bU;

			switch (ssimColor)
			{
			case 0:
				ssimGrey = SSIMmethod(colorVect, windowSize,0);
				ssimOsszeg += ssimGrey;
				num++;
				ymc = (Uint8)((ssimGrey + 1.0) * 255 / 2);
				break; 
			case 1:
				ssimRed = SSIMmethod(colorVect, windowSize,1);
				ssimOsszeg += ssimRed;
				num++;
				ymc = (Uint8)((ssimRed + 1.0) * 255 / 2);
				break;
			case 2:
				ssimGreen = SSIMmethod(colorVect, windowSize,2);
				ssimOsszeg += ssimGreen;
				num++;
				ymc = (Uint8)((ssimGreen + 1.0) * 255 / 2);
				break;
			case 3:
				ssimBlue = SSIMmethod(colorVect, windowSize,3);
				ssimOsszeg += ssimBlue;
				num++;
				ymc = (Uint8)((ssimBlue + 1.0) * 255 / 2);
				break;
			case 4:
				ssimRed = SSIMmethod(colorVect, windowSize,1);
				ssimOsszeg += ssimRed;
				num++;
				ssimGreen = SSIMmethod(colorVect, windowSize,2);
				ssimOsszeg += ssimGreen;
				num++;
				ssimBlue = SSIMmethod(colorVect, windowSize,3);
				ssimOsszeg += ssimBlue;
				num++;
				rU = (Uint8)((ssimRed + 1.0) * 255 / 2);
				gU = (Uint8)((ssimGreen + 1.0) * 255 / 2);
				bU = (Uint8)((ssimBlue + 1.0) * 255 / 2);
				break;
			}
			//red 0 0 255   green 0 255 0   blue 255 0 0     
			//r = -y -m	, g = -y -c , b = -m -c
			//Uint8 ymc = (Uint8)((ssimValue + 1.0) * 255 / 2); 

			for (int i = 0; i < windowSize; i++) {
				for (int j = 0; j < windowSize; j++) {
					if (x + i < seged.getSurface()->w && y + j < seged.getSurface()->h) {

						switch (ssimColor)
						{
						case 0:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, ymc, ymc, ymc, colorVect[i][j].alpha1),seged);
							break;
						case 1:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, ymc, ymc, 255, colorVect[i][j].alpha1), seged);
							break;
						case 2:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, ymc, 255, ymc, colorVect[i][j].alpha1), seged);
							break;
						case 3:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, 255, ymc, ymc, colorVect[i][j].alpha1), seged);
							break;
						case 4:
							ImageModify::PutPixel32(x + i, y + j, SDL_MapRGBA(seged.getSurface()->format, bU, gU, rU, colorVect[i][j].alpha1), seged);
							break;
						}
						
					}
				}
			}
		}
	}

	ssimOsszeg =  ssimOsszeg / num;

	//SDL_UnlockSurface(ssimSurface); seperable filter matlab 

	return ssimSurface;
}

void SSIM::plotLineSSIM(int x, int y, float slope, Image im1) {

	//ImVec2 first, second;

	for (int i = 0; i < imSSIM2.getSurface()->w; i++) {
		for (int j = 0; j < imSSIM2.getSurface()->h; j++) {
			if (j < slope * (i - x) + y) {
				ImageModify::PutPixel32(i, j, ImageModify::GetColor(i, j, imSSIM1), imSSIM2);
			}
			else {
				ImageModify::PutPixel32(i, j, ImageModify::GetColor(i, j, im1), imSSIM2);
			}

			/*if ((j == 0 || i == 0) && j == (int)(slope * (i - x) + y)) {
				first.x = i;
				first.y = j;
			}
			if ((j == ssimSurface2->h - 1 || i == ssimSurface2->w - 1) && j  == (int)(slope * (i - x) + y)) {
				second.x = i;
				second.y = j;
			}*/
		}
	}

	//plotLine(first.x,first.y,second.x,second.y , ssimSurface2);
}

void SSIM::saveSSIM(char outstr2[]) {
	IMG_SavePNG(imSSIM2.getSurface(), outstr2);
}

void SSIM::setSSIM(Image im1, Image im2, char str1verified[]) {
	imSSIM1.setSurface(SSIMSurface(im1, im2, ssimSize));
	imSSIM1.textureFromSurface();
	imSSIM2.Load(str1verified);
	plotLineSSIM(imSSIM2.getSurface()->w / 2, imSSIM2.getSurface()->h / 2, 1.3f,im1);
	imSSIM2.textureFromSurface();
}