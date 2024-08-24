///////////////////////////////////////////////////////////////////////////////
// shadermanager.h
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>

/***********************************************************
 *  SceneManager
 *
 *  This class contains the code for preparing and rendering
 *  3D scenes, including the shader settings.
 ***********************************************************/
class SceneManager
{
public:
	// constructor
	SceneManager(ShaderManager *pShaderManager);
	// destructor
	~SceneManager();

	// properties for loaded texture access
	struct TEXTURE_INFO
	{
		std::string tag;
		uint32_t ID;
	};

	// properties for object materials
	struct OBJECT_MATERIAL
	{
		float ambientStrength;
		glm::vec3 ambientColor;
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		float shininess;
		std::string tag;
	};

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// pointer to basic shapes object
	ShapeMeshes *m_basicMeshes;
	// total number of loaded textures
	int m_loadedTextures;
	// loaded textures info
	TEXTURE_INFO m_textureIDs[20]; //adjusted for additional textures
	// defined object materials
	std::vector<OBJECT_MATERIAL> m_objectMaterials;

	// methods for managing OpenGL textures
	bool CreateGLTexture(const char* filename, std::string tag);
	void BindGLTextures();
	void DestroyGLTextures();
	int FindTextureID(std::string tag);
	int FindTextureSlot(std::string tag);

	// find a defined material by tag
	bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);
	
	// set the transformation values 
	// into the transform buffer
	void SetTransformations(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	// set the color values into the shader
	void SetShaderColor(
		float redColorValue,
		float greenColorValue,
		float blueColorValue,
		float alphaValue);

	// set the texture data into the shader
	void SetShaderTexture(
		std::string textureTag);

	// set the texture UV scale into the shader
	void SetTextureUVScale(
		float u, float v);

	//create function to allow for moving entire set of candle meshes
	void RenderCandle(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	//TODO: Function for Headphones
	void RenderHeadphones();

	//TODO: Function for Controller
	void RenderController();

	//TODO: Function for Controller
	void RenderDrink(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	void RenderPaintTube(glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ,
		const char* textureName);

	// set the object material into the shader
	void SetShaderMaterial(
		std::string materialTag);




public:

	/*** The following methods are for the students to ***/
	/*** customize for their own 3D scene              ***/
	void PrepareScene();
	void RenderScene();

	//load texture files
	void LoadSceneTextures();


	// define all the object materials before rendering
	void DefineObjectMaterials();
	// add and define the light sources before rendering
	void SetupSceneLights();
};