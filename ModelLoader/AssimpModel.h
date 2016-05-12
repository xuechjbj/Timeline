
#ifndef SCENE_H
#define SCENE_H

#include <assimp/cimport.h>
#include <assimp/scene.h> 
#include <assimp/postprocess.h>
#include <assimp/types.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <fstream>

//This is for a shader uniform block
//This struct holds all the variables pertaining to materials
struct MyMaterial {
	float diff[4];
	float amb[4];
	float spec[4];
	float emis[4];
	float shininess;
	int texCount;
	int normTexCount;
};

// Information to render each assimp node
struct MyMesh {

	GLuint vao;
	//GLuint texIndex;
	GLuint uniformBlockIndex;
	int numFaces;

	GLuint matTex; //the diffuse texture
	GLuint matNorm; // the Normal texture (if it has any)

	//unsigned int tangents;
	//unsigned int bitangents;
	unsigned int tcoord0;
	unsigned int tcoord1;
	unsigned int tcoord2;
	unsigned int tcoord3;

	unsigned int boneIndices; //indices to bones affecting a vertex
	unsigned int weights; //bone weights
};



// This is for a shader uniform block
/*struct MyMaterial {

	float diffuse[4];
	float ambient[4];
	float specular[4];
	float emissive[4];
	float shininess;
	int texCount;
};*/

/*
aiScene have aiMeshes and aiAnimations
aiMesh have aiBones.
aiBones in aiMeshes point to aiNodes, which define the coordinate system.
An aiAnimation have an array of channels of type aiNodeAnim which
each animate an aiNode, including bones.
*/

/* All this OpenGL data is constant during animation */
struct MeshGLData
{
	unsigned int vao;
	/* vertex buffer objects */
	unsigned int vertices;
	unsigned int normals;
	unsigned int tangents;
	unsigned int bitangents;
	unsigned int tcoord0;
	unsigned int tcoord1;
	unsigned int tcoord2;
	unsigned int tcoord3;
	unsigned int indices; //vertex indices to glDrawElements 
	unsigned int boneIndices; //indices to bones affecting a vertex
	unsigned int weights; //bone weights
	unsigned int numElements; //number of faces * 3
							  /* uniforms */
							  //std::vector<aiMatrix4x4> bones; //final bones after transformation
	//struct Material material;
	GLuint uniformBlockIndex;
	int numFaces;
};

struct AnimGLData;
class AssimpModel;

class BaseRenderer
{
public:
	virtual void renderer(float t) {};
};

/* Abstract away rendering in AnimGLData so we can use custom render
* functions and multiple passes */
struct AnimRenderer
{
	AnimRenderer();
	friend class AnimGLData;
protected:
	void drawBegin(unsigned int shader, int idx);
	void drawEnd(int idx);

private:
	void setParent(AnimGLData* parent);
	void setScene(const AssimpModel* scene);
	//virtual void draw();
	virtual void draw(int idx);
	int m_CurrentMesh;
	AnimGLData* m_Parent;
	const AssimpModel* m_Scene;
};

/* This OpenGL data is dynamic during animation. This struct lets us
* create multiple instances of an animation with different time offsets. */
struct AnimGLData
{
	friend class AssimpModel;
	const AssimpModel* m_Scene;
	//pointer to the animation data (constant)
	const aiAnimation* m_Animation;
	std::map<int, AnimRenderer*> m_Renderer;
	//2D array of uniform matrices for bones for every mesh (changes every frame)
	std::vector<std::vector<aiMatrix4x4> > m_Bones;
	//One worldspace matrix for every mesh
	std::vector<aiMatrix4x4> m_ModelView;
	//time of animation
	float m_Time;
	aiMatrix4x4 m_Camera;

	//add renderer to model with index 'modelIndex'. Returns 'modelIndex'
	int addRenderer(AnimRenderer* renderer, int modelIndex);
	//add renderer to mode with name 'modelName'. Returns index of model
	int addRenderer(AnimRenderer* renderer, const std::string modelName);
	//Removes renderer attached to model with index 'modelIndex'
	void removeRenderer(int modelIndex);
	void stepAnimation(float t); //step one frame forwards
	void render(float t);
	void setCamera(const aiMatrix4x4& camera);
private:
	void recursiveUpdate(aiNode* node, const aiMatrix4x4& parentMatrix);
	void interpolateTranslation(const aiNodeAnim* nodeAnim, aiVector3D& translation);
	void interpolateScale(const aiNodeAnim* nodeAnim, aiVector3D& scale);
	void interpolateRotation(const aiNodeAnim* nodeAnim, aiQuaternion& rotation);

};

/* Could have used an std::pair, but a new type is more readable.
* This struct is stored per-node if it is a bone, so we can look up
* the model and bone index */
struct NodeMeshBoneIndex
{
	int meshIndex;
	int boneIndex;
};

class CCamera;

struct AssimpNode
{
	AssimpNode():modelMat4(1.0)
	{
	}
	glm::mat4 modelMat4;
	std::vector<struct MyMesh> meshes;
	std::vector<struct AssimpNode*> children;
};

class AssimpModel
{
public:
	static const int MAX_UVMAPS = 4;
	static const int MAX_BONES_PER_VERTEX = 4;
	static const int MAXBONESPERMESH = 32;

	const aiScene* m_Scene;
	//look up animations by name
	std::map<std::string, const aiAnimation*> m_LUTAnimation;
	//look up bone ID and Mesh ID by node. I.e aiNode* 'node' is the 'i'th bone
	//in the 'j'th mesh.
	std::map<const aiNode*, std::vector<NodeMeshBoneIndex> > m_LUTBone;
	//Constant/static data used by OpenGL for each mesh
	std::vector<MeshGLData*> m_MeshData;
	//Dynamic animation data per animation instance that changes every
	//animation frame
	std::vector<AnimGLData*> m_AnimData;

	//functions
	AssimpModel();
	~AssimpModel();
	int load(const std::string& path);

	const aiScene* getScene() const;
	const aiAnimation* getAnimation(const std::string& name) const;
	const MeshGLData* getMeshGLData(int idx) const;
	size_t getMeshCount() { return m_MeshData.size(); }
	AnimGLData* createAnimation(const std::string& name, const aiMatrix4x4& camera);
	AnimGLData* createAnimation(unsigned int anim, const aiMatrix4x4& camera);

	void renderer(glm::mat4 &projMat, glm::mat4 &viewMat);
	void getModelRange(float &xmin, float&xmax,
		float &ymin, float &ymax,
		float &zmin, float &zmax)
	{
		xmin = xMin, xmax = xMax;
		ymin = yMin, ymax = yMax;
		zmin = zMin, zmax = zMax;
	}

private:
	void loadAssimpMode();
	const aiScene* importScene(const std::string& path);

	void initGLBoneData(struct MyMesh* gldata, int meshID);

	void loadMat(const aiMaterial* tm, struct MyMesh &aMesh);
	void recursiveRender(struct AssimpNode *nd);
	void recursiveLoad(aiNode *nd, const glm::mat4 &modelMatrix, struct AssimpNode *aNode);

	struct AssimpNode *assimpRootNode;

	std::string sModelName, sModelDir;

	GLuint program;
	// Vertex Attribute Locations
	GLuint vertexLoc = 0, normalLoc = 1, texCoordLoc = 2;
	GLuint tangentLoc = 3, bitangentsLoc = 4;

	GLuint projMatLoc, viewMatLoc, modelMatLoc;
	//GLuint matricesUniLoc, materialUniLoc;

	std::vector<struct MyMesh> myMeshes;

	CCamera *mCCamera;

	float xMin, xMax;
	float yMin, yMax;
	float zMin, zMax;
};


class CCamera {
public:
	CCamera();
	//CCamera(const CCamera&);
	//void operator=(const CCamera&);
	CCamera(uint32_t id, aiCamera* camera);
	virtual ~CCamera();
	void translate(glm::vec3& translateVector);
	void translate(glm::vec3&& translateVector);
	const glm::mat4 getViewMatrix() const;
	std::string getName();
	uint32_t getID();

	/// Get aiCamera to set near-far, FOV, aspect, ...
	aiCamera* getCamera();

	void init(aiNode* root);
	const glm::mat4& getProjectionMatrix();
	const glm::mat4& getProjectionViewMatrix();
	void pitch(float angle);
	void roll(float angle);
	void yaw(float angle);
	void pitchYawRoll(float pitchAngle, float yawAngle, float rollAngle);
	void pitchYawRoll(glm::vec3& pyaAngles);
	void pitchYawRoll(glm::vec3&& pyaAngles);

private:
	// The view matrix, which can also be known as the world matrix determines the position of the ‘camera’ in space.
	glm::mat4 m_viewMatrix;
	glm::mat4 m_modelMatrix;
	glm::mat4 m_projectionMatrix;
	glm::mat4 m_projectionViewMatrix;
	glm::vec3 m_eyeGlobal;        // position of camera
	glm::vec3 m_centerGlobal;    // where camera is looking to
	glm::vec3 m_upGlobal;

	glm::vec3 m_xGlob;
	glm::vec3 m_yGlob;
	glm::vec3 m_zGlob;

	aiCamera* m_camera;
	uint32_t m_ID;

	void updateViewMatrix();
	void updateGlobals();
	void updateAxis();
};

#endif