///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();


	//texture collector
	for (int i = 0; i < 16; i++) //limit number of textures
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// override the opengl textures
	DestroyGLTextures();
}





/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}




/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	// matrix math for calculating the final model matrix
	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		// pass the model matrix into the shader
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		// pass the color values into the shader
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/





/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}


/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures() //todo update this block with textures
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/wood.jpg", "wood");
	bReturn = CreateGLTexture(
		"textures/candle.jpg", "candle");
	bReturn = CreateGLTexture(
		"textures/er.jpg", "er");
	bReturn = CreateGLTexture(
		"textures/flame.jpg", "flame");
	bReturn = CreateGLTexture(
		"textures/artbook.jpg", "artbook");
	bReturn = CreateGLTexture(
		"textures/hlartbook.jpg", "hlartbook");
	bReturn = CreateGLTexture(
		"textures/botwartbook.jpg", "botwartbook");
	bReturn = CreateGLTexture(
		"textures/drink.jpg", "drink");
	bReturn = CreateGLTexture(
		"textures/cantop.jpg", "cantop");
	bReturn = CreateGLTexture(
		"textures/botw_spine.jpg", "botw_spine");
	bReturn = CreateGLTexture(
		"textures/pages.jpg", "pages");
	bReturn = CreateGLTexture(
		"textures/y_paint.jpg", "y_paint");
	bReturn = CreateGLTexture(
		"textures/b_paint.jpg", "b_paint");
	bReturn = CreateGLTexture(
		"textures/r_paint.jpg", "r_paint");
	bReturn = CreateGLTexture(
		"textures/erspine2.jpg", "erspine2");
	bReturn = CreateGLTexture(
		"textures/painting1.jpg", "painting1");
	bReturn = CreateGLTexture(
		"textures/curtain_test.jpg", "curtain");
	bReturn = CreateGLTexture(
		"textures/w_paint.jpg", "w_paint");
	bReturn = CreateGLTexture(
		"textures/headphones.jpg", "headphones");
	bReturn = CreateGLTexture(
		"textures/pbhandle.jpg", "pbhandle");



	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}





/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f); //adjust shine to be less harsh
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);


	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	backdropMaterial.ambientStrength = 0.6f;
	backdropMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.1f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 0.0;
	backdropMaterial.tag = "backdrop";

}



/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}


/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}



/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//overhead lamp with wider reach and neutral/slightly warm toned light
	m_pShaderManager->setVec3Value("lightSources[0].position", -8.0f, 6.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.65f, 0.55f, 0.35f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.55f, 0.55f, 0.55f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 35.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 5.50f);

	
	// light from candle, smaller, specular with a warmer tone
	m_pShaderManager->setVec3Value("lightSources[1].position", 17.0f, 2.15f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.95f, 0.85f, 0.35f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.95f, 0.85f, 0.35f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 15.0f);

	/*m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 3.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f);*/
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();

	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	//load additional shape meshes for replicating the 2D image
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	//m_basicMeshes->LoadTapperedCylinder();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}




// function for candle to make moving it around easier
void SceneManager::RenderCandle(glm::vec3 scaleXYZ,
								float XrotationDegrees,
								float YrotationDegrees,
								float ZrotationDegrees,
								glm::vec3 positionXYZ)
{
	// **** TORUS: Candle Jar ************************************************************************** DONE!
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw candle jar
	SetShaderColor(0.95, 0.80, 0.55, 0.98f);
	//SetShaderTexture("candle");
	SetShaderMaterial("wood"); //frosted glass reflects more like wood, not super shiny
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawTorusMesh(); //Candle jar


	// **** CYLINDER: Candle Wax ************************************************************************* DONE!
	const glm::vec3 waxOffsetXYZ = glm::vec3(0.0f, -0.90f, 0.0f);
	//set scale
	scaleXYZ = glm::vec3(1.48, 1.45, 1.48); //inset and shorter than jar
	//rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + waxOffsetXYZ); // move inside the torus (candle jar)
	//set color and draw wax
	//SetShaderColor(0.70, 0.65, 0.65, 1.0f); // lighter cream almost white color
	SetShaderTexture("candle");
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh(); // Cyl 2


	// **** CYLINDER: Wick #1 ************************************************************************* Done!
	const glm::vec3 wickOneOffsetXYZ = glm::vec3(-0.6f, -0.30f, 0.0f);
	// set scale
	scaleXYZ = glm::vec3(0.05, 1.20, 0.05);
	//rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + wickOneOffsetXYZ);
	//set color and draw wick
	SetShaderColor(0.40, 0.25f, 0.11f, 1.0f); // dark brown
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();



	// **** CYLINDER: Wick #2 ************************************************************************* DONE!
	const glm::vec3 wickTwoOffsetXYZ = glm::vec3(0.55f, -0.30f, -0.25f);
	// set scale
	scaleXYZ = glm::vec3(0.05, 1.20, 0.05);
	//rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + wickTwoOffsetXYZ);
	//set color and draw wick
	SetShaderColor(0.40, 0.25f, 0.11f, 1.0f); // dark brown
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();


	// **** CYLINDER: Wick #3 ************************************************************************* DONE!
	const glm::vec3 wickThreeOffsetXYZ = glm::vec3(0.10f, -0.30f, 0.6f);
	// set scale
	scaleXYZ = glm::vec3(0.05, 1.20, 0.05);
	//rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + wickThreeOffsetXYZ);
	//set color and draw wick
	SetShaderColor(0.40, 0.25f, 0.11f, 1.0f); // dark brown
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();


	//  **** Flame #1 *********************************************************************************
	const glm::vec3 flameOneOffsetXYZ = glm::vec3(-0.60f, 0.70f, 0.0f);
	// set scale and check
	scaleXYZ = glm::vec3(0.15f, 0.55f, 0.10f); //
	//rotation not needed for this shape right now
	XrotationDegrees = 0.0f;//make flame uniquely tilt a bit
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + flameOneOffsetXYZ);

	//set texture and draw
	SetShaderTexture("flame");
	SetShaderMaterial("glass");
	SetTextureUVScale(3.0, 2.0);
	m_basicMeshes->DrawConeMesh(); // cone


	//  **** Flame #2 *********************************************************************************
	const glm::vec3 flameTwoOffsetXYZ = glm::vec3(0.55f, 0.70f, -0.25f);
	// set scale and check
	scaleXYZ = glm::vec3(0.15f, 0.55f, 0.10f); //
	//rotation not needed for this shape right now
	XrotationDegrees = 0.0f;//make flame uniquely tilt a bit
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + flameTwoOffsetXYZ);

	//set texture and draw
	SetShaderTexture("flame");
	SetShaderMaterial("glass");
	SetTextureUVScale(3.0, 2.0);
	m_basicMeshes->DrawConeMesh(); // cone


	//  **** Flame #3 *********************************************************************************
	const glm::vec3 flameThreeOffsetXYZ = glm::vec3(0.10f, 0.70f, 0.6f);
	// set scale and check
	scaleXYZ = glm::vec3(0.15f, 0.55f, 0.10f); //
	//rotation not needed for this shape right now
	XrotationDegrees = 0.0f;//make flame uniquely tilt a bit
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + flameThreeOffsetXYZ);

	//set texture and draw
	SetShaderTexture("flame");
	SetShaderMaterial("glass");
	SetTextureUVScale(3.0, 2.0);
	m_basicMeshes->DrawConeMesh(); // cone

	
}



// TODO: Complete RenderHeadphones function
void SceneManager::RenderHeadphones()
{

}


// TODO: Complete RenderController function
void SceneManager::RenderController()
{

}

// TODO: Complete RenderDrink function
void SceneManager::RenderDrink(glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// ******************************************************************************   DRINK ******************************************************************************
	// main can
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// Set texture and shader and  draw
	//SetShaderColor(0.92, 0.55, 0.84, 1); //set to blue
	SetShaderTexture("drink");
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh                 ************** can top *************
	scaleXYZ = glm::vec3(1.025f, 0.025f, 1.025f);
	// set the XYZ rotation for the mesh
	const glm::vec3 topOffsetXYZ = glm::vec3(0.0f, 5.00f, 0.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 80.0f;
	ZrotationDegrees = 0.0f;
	//// set the XYZ position for the mesh
	//positionXYZ = glm::vec3(0.0f, 5.02f, 10.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + topOffsetXYZ);
	// Set texture and shader and  draw
	SetShaderTexture("cantop");
	SetShaderMaterial("metal");
	SetTextureUVScale(0.8, 0.90);
	m_basicMeshes->DrawCylinderMesh();
}


void SceneManager::RenderPaintTube(glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	const char* textureName)

{

	// ******************************************************************************   TUBE ******************************************************************************
	scaleXYZ = glm::vec3(0.55f, 5.0f, 0.55f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture(textureName);
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawCylinderMesh();

	// ******************************************************************************   CAP ******************************************************************************
	scaleXYZ = glm::vec3(0.45f, 0.25f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// Adjust position with offset
	const glm::vec3 capOffsetXYZ = glm::vec3(0.0f, -5.45f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + capOffsetXYZ);
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.014, 0.014);
	m_basicMeshes->DrawCylinderMesh();

	// ******************************************************************************   CAP NECK ******************************************************************************
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// Adjust position with offset
	const glm::vec3 neckOffsetXYZ = glm::vec3(0.0f, -5.2f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ + neckOffsetXYZ);
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.01, 0.01);
	m_basicMeshes->DrawCylinderMesh();
	

	////original positions for offsets
	//positionXYZ = glm::vec3(7.0f, 5.5f, 3.0f); //tube
	//positionXYZ = glm::vec3(7.0f, 0.3f, 3.0f); //cap
	//positionXYZ = glm::vec3(7.0f, 0.5f, 3.0f); //neck
}




/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	RenderCandle(glm::vec3(1.5f, 1.5f, 5.0f),	//scale		*Torus/Jar only*
				90.0f, 0.0f, 0.0f,				//rotation	*Torus/Jar only
				glm::vec3(17.0f, 1.0f, 5.0f ));	//position	All meshes in candle


	RenderDrink(glm::vec3(1.0f, 5.0f, 1.0f),		//scale		*Cylinder/Can base only*
				0.0f, 125.0f, 0.0f,					//rotation	*Cylinder/Can base only
				glm::vec3(-16.0f, 0.01f, 10.0f));	//position	All meshes in drink

	RenderPaintTube(glm::vec3(0.55f, 5.0f, 0.55f),		//scale		*Cylinder/Can base only*
					180.0f, 200.0f, 0.0f,				//rotation	*Cylinder/Can base only
					glm::vec3(7.50f, 5.5f, 3.0f),		//position	All meshes
					"r_paint");

	RenderPaintTube(glm::vec3(0.55f, 5.0f, 0.55f),		//scale		base only*
					180.0f, 200.0f, 0.0f,				//rotation	base only
					glm::vec3(9.5f, 5.5f, 2.5f),		//position	All meshes
					"y_paint");

	RenderPaintTube(glm::vec3(0.55f, 5.0f, 0.55f),		//scale		base only*
					180.0f, 200.0f, 0.0f,				//rotation	base only
					glm::vec3(11.0f, 5.5f, 3.75f),		//position	All meshes in drink
					"b_paint");
		 
	
//DESK ******************************************************************************
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, -40.0f, 10.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 5.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// set texture and draw
	SetShaderTexture("wood"); 
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawPlaneMesh();
	

//WALL ******************************************************************************
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 100.0f, -20.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -2.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// Set texture and  draw
	//SetShaderColor(0.42, 0.55, 0.84, 1); //set to blue
	SetShaderTexture("curtain");
	SetShaderMaterial("wood");
	SetTextureUVScale(6.0, 1.0);
	m_basicMeshes->DrawPlaneMesh();


// ************************************************************* Single ROTATED paint tube ************************************************************* 
	
	// ********************************************   TUBE ********************************************
	scaleXYZ = glm::vec3(0.55f, 5.0f, 0.55f);
	//rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 60.0f;
	//position
	positionXYZ = glm::vec3(8.95f, 0.75f, 5.0f); 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("w_paint"); //TODO - create and add white lable texture
	SetShaderMaterial("metal");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawCylinderMesh(false, true, true);

	// ********************************************   CAP ********************************************
	scaleXYZ = glm::vec3(0.45f, 0.3f, 0.45f);
	//rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 60.0f;
	//position 
	positionXYZ = glm::vec3(4.35f, 0.75f, 7.65f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.014, 0.014);
	m_basicMeshes->DrawCylinderMesh();

	// ******************************************** CAP NECK ********************************************
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);
	//rotation
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 60.0f;
	//position 
	positionXYZ = glm::vec3(4.65f, 0.75f, 7.45f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.01, 0.01);
	m_basicMeshes->DrawCylinderMesh();



// ****************************************************************************************************** BOOKS ******************************************************************************************************    

// **** Book 1 ******************************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(8.0f, 10.5f, 0.50f);
	//rotation
	XrotationDegrees = 90.0f; 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 65.0f;
	// set and check position 
	positionXYZ = glm::vec3(-9.0f, 0.40f, 5.0f); 
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture of book 1
	SetShaderTexture("artbook");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMesh(); // book 1


// **** Book 2 ******************************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(8.0f, 12.5f, 0.65f);//same for all books
	//rotation not needed for this shape right now
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;
	// set position 
	positionXYZ = glm::vec3(-9.0f, 1.0f, 4.5f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("hlartbook");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh(); // book 2


// **** Book 3 ******************************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(8.0f, 10.0f, 1.25f);//same for all books
	//rotation not needed for this shape right now
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;
	// set position 
	positionXYZ = glm::vec3(-9.0f, 1.90f, 4.0f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ); 
	//set texture and draw
	SetShaderTexture("botwartbook");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh(); // book 3

	//spine
	// set scale and check
	scaleXYZ = glm::vec3(10.0f, 0.25f, 1.25f);//same for all books
	//rotation not needed for this shape right now
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -20.0f;
	// set position 
	positionXYZ = glm::vec3(-7.60f, 1.90f, 7.85f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("botw_spine");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();

	//pages- top
	// set scale and check
	scaleXYZ = glm::vec3(7.8f, 0.20f, 1.15f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -110.0f;
	// set position 
	positionXYZ = glm::vec3(-13.60f, 1.90f, 5.75f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.3, 0.3);
	m_basicMeshes->DrawBoxMesh();
	//pages - bottom
	// set scale and check
	scaleXYZ = glm::vec3(7.8f, 0.20f, 1.15f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -110.0f;
	// set position 
	positionXYZ = glm::vec3(-4.30f, 1.90f, 2.45f); 
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.3, 0.3);
	m_basicMeshes->DrawBoxMesh();
	//pages-side
	// set scale and check
	scaleXYZ = glm::vec3(10.02f, 0.25f, 1.15f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -200.0f;
	// set position 
	positionXYZ = glm::vec3(-10.35f, 1.90f, 0.30f);	//**********************
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.3, 0.3);
	m_basicMeshes->DrawBoxMesh();
	


// **** Book 4 ******************************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(8.0f, 10.0f, 1.0f);//same for all booksd
	//rotation not needed for this shape right now
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 75.5f;
	// set position 
	positionXYZ = glm::vec3(-8.950f, 3.10f, 3.85f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("er");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh(); // book 4

	//pages- top
	// set scale and check
	scaleXYZ = glm::vec3(7.8f, 0.20f, 0.90f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 75.5f;
	// set position 
	positionXYZ = glm::vec3(-13.78f, 3.10f, 5.0f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.3, 0.3);
	m_basicMeshes->DrawBoxMesh();
	
	//pages - bottom
	// set scale and check
	scaleXYZ = glm::vec3(7.8f, 0.20f, 0.9f);
	XrotationDegrees = 90.0f; 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 75.5f;
	// set position 
	positionXYZ = glm::vec3(-4.22, 3.10f, 2.5f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.3, 0.3);
	m_basicMeshes->DrawBoxMesh();
	
	//pages-side
	// set scale and check
	scaleXYZ = glm::vec3(10.02f, 0.25f, 0.90f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -14.50;
	// set position 
	positionXYZ = glm::vec3(-9.99f, 3.10f, 0.08f);	//**********************
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pages");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.3, 0.3);
	m_basicMeshes->DrawBoxMesh();

	//spine
	// set scale and check
	scaleXYZ = glm::vec3(10.0f, 0.25f, 0.98f);//same for all books
	//rotation not needed for this shape right now
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -15.0f;
	// set position 
	positionXYZ = glm::vec3(-7.98f, 3.10f, 7.65f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("erspine2");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();
	

// ******************************************************************************************************    TODO: canvas painting (box) ******************************************************************************************************
//	
// 	//Canvas
	// set scale and check
	scaleXYZ = glm::vec3(5.0f, 3.5f, 0.4f);
	XrotationDegrees = -20.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = -0.0;
	// set position 
	positionXYZ = glm::vec3(2.25, 1.75, 4.5);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("painting1");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();

	//pic standd
	//****************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(0.50f, 0.3f, 1.0f);
	XrotationDegrees = -20.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = -0.0;
	// set position 
	positionXYZ = glm::vec3(0.80, 0.0, 5.0);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderColor(0.52f, 0.4f, 0.24f, 1.0f);
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();
	//****************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(0.50f, 0.3f, 1.0f);
	XrotationDegrees = -20.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = -0.0;
	// set position 
	positionXYZ = glm::vec3(3.80, 0.0, 5.0);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderColor(0.52f, 0.4f, 0.24f, 1.0f);
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();
	//****************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(0.50f, 4.40f, 1.0f);
	XrotationDegrees = -20.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = 0.0;
	// set position 
	positionXYZ = glm::vec3(3.80, 2.0, 3.5);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderColor(0.52f, 0.4f, 0.24f, 1.0f);
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();
	//****************************************************************
	// set scale and check
	scaleXYZ = glm::vec3(0.50f, 4.40f, 1.0f);
	XrotationDegrees = -20.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = 0.0;
	// set position 
	positionXYZ = glm::vec3(0.80, 2.0, 3.5);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderColor(0.52f, 0.4f, 0.24f, 1.0f);
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();






	// **********************************************************************   TODO: HEADPHONES   ********************************************************************** 

// TODO: Create RenderHeadphones function
	// *************************************************************** headband	
	// set scale and check
	scaleXYZ = glm::vec3(2.0f, 2.0f, 1.0f);
	XrotationDegrees = 80.0;
	YrotationDegrees = 10.0;
	ZrotationDegrees = 120.0;
	// set position 
	positionXYZ = glm::vec3(-6.0, 0.55f, 11.4);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
//	SetShaderColor(0.77f, 0.68f, 0.60f, 1.0f);
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawHalfTorusMesh();

	// *************************************************************** L ear muff	
	// set scale and check
	scaleXYZ = glm::vec3(1.20f, 0.800f, 2.0f);
	XrotationDegrees = 90.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = 20.0;
	// set position 
	positionXYZ = glm::vec3(-4.85, 0.50, 13.45);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawTorusMesh();

	// *************************************************************** L Ear cap		
	// set scale and check
	//scaleXYZ = glm::vec3(0.90f, 0.250f, 0.65f);
	scaleXYZ = glm::vec3(1.0f, 0.30f, 0.75f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0;
	ZrotationDegrees = 0.0f;
	// set position 
	positionXYZ = glm::vec3(-4.85, 0.60, 13.45);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();

	// *************************************************************** R ear	
	// set scale and check
	scaleXYZ = glm::vec3(1.20f, 0.800f, 2.0f);
	XrotationDegrees = 90.0;
	YrotationDegrees = 0.0;
	ZrotationDegrees = 40.0;
	// set position 
	positionXYZ = glm::vec3(-3.5, 0.50, 11.25);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawTorusMesh();


	// ***************************************************************  R Ear cap
	// set scale and check
	//scaleXYZ = glm::vec3(1.0f, 0.30f, 0.75f);
	scaleXYZ = glm::vec3(1.0f, 0.30f, 0.75f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -40.0;
	ZrotationDegrees = 0.0f;
	// set position 
	positionXYZ = glm::vec3(-3.5, 0.60, 11.25);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();



	// *************************************************************** headband to R Ear connector	
	// set scale and check
	scaleXYZ = glm::vec3(0.2f, 1.85f, 0.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -50.0f;
	ZrotationDegrees = -65.0f;
	// set position 
	positionXYZ = glm::vec3(-5.30, 0.15, 9.45);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();


// *************************************************************** headband to L Ear connector	
	// set scale and check
	scaleXYZ = glm::vec3(0.2f, 1.65f, 0.2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = -65.0f;
	// set position 
	positionXYZ = glm::vec3(-7.30, 0.35, 12.95);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("headphones");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();




// **********************************************************************   TODO: PAINTBRUSHES   ********************************************************************** 

	// top pb
	scaleXYZ = glm::vec3(0.10f, 6.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 90.0f;
	// set position 
	positionXYZ = glm::vec3(13.7f, 0.250, 4.85f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pbhandle");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();
	//top bristle
	scaleXYZ = glm::vec3(0.10f, 1.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 90.0f;
	// set position 
	positionXYZ = glm::vec3(8.25, 0.250, 7.38);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("candle");
	//SetShaderMaterial("wood");
	m_basicMeshes->DrawConeMesh();




	// mid pb
	scaleXYZ = glm::vec3(0.10f, 6.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;
	// set position 
	positionXYZ = glm::vec3(13.75f, 0.250, 4.85f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pbhandle");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();
	//medium bristle
	scaleXYZ = glm::vec3(0.10f, 1.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 90.0f;
	// set position 
	positionXYZ = glm::vec3(8.55, 0.250, 7.85);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("candle");
	//SetShaderMaterial("wood");
	m_basicMeshes->DrawConeMesh();



	// bottom pb
	scaleXYZ = glm::vec3(0.10f, 6.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 90.0f;
	// set position 
	positionXYZ = glm::vec3(13.7f, 0.250, 4.85f);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("pbhandle");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();
	//bottom bristle
	scaleXYZ = glm::vec3(0.10f, 1.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 90.0f;
	// set position 
	positionXYZ = glm::vec3(8.80, 0.250, 8.30);
	//set transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//set texture and draw
	SetShaderTexture("candle");
	//SetShaderMaterial("wood");
	m_basicMeshes->DrawConeMesh();








}
