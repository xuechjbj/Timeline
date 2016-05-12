#include "GLStuff.h"
#include "AssimpModel.h"
#include <assert.h>
#include <stdexcept>
#include <glm/ext.hpp>

const int MATERIAL_BINDING_POINT = 1;

void CopyaiMat(const aiMatrix4x4 *from, glm::mat4 &to);

AssimpModel::AssimpModel()
{
	mCCamera = 0;
	//theModel->cPath = _strdup(file);
	//theModel->sName = (string)file;
	
}

int AssimpModel::load(const std::string& path)
{
	xMin = INT_MAX, xMax = INT_MIN;
	yMin = INT_MAX, yMax = INT_MIN;
	zMin = INT_MAX, zMax = INT_MIN;

	std::string::size_type slashInd = path.find_last_of("/");
	if (slashInd == std::string::npos) {
		sModelDir = ".";
	}
	else if (slashInd == 0) {
		sModelDir = "/";
	}
	else {
		sModelDir = path.substr(0, slashInd);
	}

	m_Scene = importScene(path);
	if (!m_Scene) {
		fprintf(stderr, "Couldn't load model file.");
		return -1;
	}

	program = createShaderProgram("data/shaders/assimp_model.vert", "data/shaders/assimp_model.frag");

	projMatLoc = glGetUniformLocation(program, "projMatrix");
	viewMatLoc = glGetUniformLocation(program, "viewMatrix");
	modelMatLoc = glGetUniformLocation(program, "modelMatrix");

	vertexLoc = glGetAttribLocation(program, "position");
	normalLoc = glGetAttribLocation(program, "normal");
	texCoordLoc = glGetAttribLocation(program, "texCoord");

	glUniform1i(glGetUniformLocation(program, "texUnit"), 0);
	glUniform1i(glGetUniformLocation(program, "normTexUnit"), 1);

	glUniformBlockBinding(program, glGetUniformBlockIndex(program, "Material"), MATERIAL_BINDING_POINT);

	//initGLModelData();
	loadAssimpMode();
	aiReleaseImport(m_Scene);

	return 0;
}

AssimpModel::~AssimpModel()
{
	
	delete mCCamera;
}

const aiScene* AssimpModel::getScene() const
{
	return m_Scene;
}

const aiAnimation* AssimpModel::getAnimation(const std::string& name) const
{
	std::map<std::string, const aiAnimation*>::const_iterator it = m_LUTAnimation.find(name);
	if (it == m_LUTAnimation.end())
		return 0;
	return it->second;
}

const MeshGLData* AssimpModel::getMeshGLData(int idx) const
{
	if (idx >= m_MeshData.size())
		return 0;
	return m_MeshData[idx];
}

const aiScene* AssimpModel::importScene(const std::string& path)
{
	const aiScene* assimpScene = 0;
	assimpScene = aiImportFile(path.c_str(),
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType); // aiProcess_FlipUVs
	return assimpScene;
}

void AssimpModel::renderer(glm::mat4 &projMatrix, glm::mat4 &viewMatrix)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glUseProgram(program);

	
#if 0
	if (mCCamera->getCamera() == 0)
	{
		glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(projMatrix));
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(viewMatrix));
	}
	else
	{
		glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(mCCamera->getProjectionMatrix()));
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(mCCamera->getViewMatrix()));
	}
#else
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(projMatrix));
	glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(viewMatrix));
#endif
	glm::mat4 modelMatrix(1.0f);
	recursiveRender(assimpRootNode);

	glUseProgram(0);
	glDisable(GL_CULL_FACE);
}

void AssimpModel::recursiveRender(struct AssimpNode *nd)
{
	const glm::mat4 modelMatrix = nd->modelMat4;
	//glmMatPrint<glm::mat4>(newModelMat, 4, "newModelMat");

	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, (GLfloat*)glm::value_ptr(modelMatrix));

	// draw all meshes assigned to this node
	for (unsigned int n = 0; n < nd->meshes.size(); ++n) {
		struct MyMesh &myMesh = nd->meshes[n];

		// bind material uniform
		glBindBufferBase(GL_UNIFORM_BUFFER, MATERIAL_BINDING_POINT, myMesh.uniformBlockIndex);
		//glBindBufferRange(GL_UNIFORM_BUFFER, materialUniLoc, myMesh.uniformBlockIndex, 0, sizeof(struct MyMaterial));
		
		// bind texture
		if (myMesh.matNorm != 0)
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, myMesh.matNorm);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		if (myMesh.matTex != 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, myMesh.matTex);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		// bind VAO
		glBindVertexArray(myMesh.vao);
		// draw
		glDrawElements(GL_TRIANGLES, myMesh.numFaces * 3, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
	}

	// draw all children
	for (unsigned int n = 0; n < nd->children.size(); ++n) {
		recursiveRender(nd->children[n]);
	}
}

void AssimpModel::loadAssimpMode() 
{
	assimpRootNode = new struct AssimpNode();
	glm::mat4 modelMatrix(1.0f);
	recursiveLoad(m_Scene->mRootNode, modelMatrix, assimpRootNode);
}

void AssimpModel::recursiveLoad(aiNode *nd, const glm::mat4 &modelMatrix, struct AssimpNode *aNode)
{
	struct MyMesh aMesh;
	GLuint buffer;

	aiMatrix4x4 m = nd->mTransformation;

	glm::mat4 newModelMat(1.0f);
	CopyaiMat(&m, newModelMat);
	newModelMat = modelMatrix * newModelMat;

	aNode->modelMat4 = newModelMat;

	aiMatrix3x3 newModelMat3;
	newModelMat3.a1 = newModelMat[0][0]; newModelMat3.a2 = newModelMat[1][0]; newModelMat3.a3 = newModelMat[2][0];
	newModelMat3.b1 = newModelMat[0][1]; newModelMat3.b2 = newModelMat[1][1]; newModelMat3.b3 = newModelMat[2][1];
	newModelMat3.c1 = newModelMat[0][2]; newModelMat3.c2 = newModelMat[1][2]; newModelMat3.c3 = newModelMat[2][2];
	// For each mesh
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n)
	{
		memset((void*)&aMesh, '\0', sizeof(aMesh));
		const aiMesh* mesh = m_Scene->mMeshes[nd->mMeshes[n]];

		// create array with faces
		// have to convert from Assimp format to array
		unsigned int *faceArray = (unsigned int *)malloc(sizeof(unsigned int) * mesh->mNumFaces * 3);
		unsigned int faceIndex = 0;

		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];

			memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}
		aMesh.numFaces = mesh->mNumFaces;

		// generate Vertex Array for mesh
		glGenVertexArrays(1, &(aMesh.vao));
		glBindVertexArray(aMesh.vao);

		// buffer for faces
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);

		// buffer for vertex positions
		if (mesh->HasPositions()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(vertexLoc);
			glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, 0, 0, 0);
			for (int i = 0; i < mesh->mNumVertices; i++)
			{
				aiVector3D p = newModelMat3 * mesh->mVertices[i];
				xMin = std::min(xMin, p.x);
				xMax = std::max(xMax, p.x);
				yMin = std::min(yMin, p.y);
				yMax = std::max(yMax, p.y);
				zMin = std::min(zMin, p.z);
				zMax = std::max(zMax, p.z);
				//printf("(%.2f, %.2f, %.2f)\n", p->x, p->y, p->z);
			}
		}

		// buffer for vertex normals
		if (mesh->HasNormals()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
			glEnableVertexAttribArray(normalLoc);
			glVertexAttribPointer(normalLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		if (mesh->HasTangentsAndBitangents()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D),
				mesh->mTangents, GL_STATIC_DRAW);
			glEnableVertexAttribArray(tangentLoc);
			glVertexAttribPointer(tangentLoc, 3, GL_FLOAT, 0, 0, 0);

			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D),
				mesh->mBitangents, GL_STATIC_DRAW);
			glEnableVertexAttribArray(bitangentsLoc);
			glVertexAttribPointer(bitangentsLoc, 3, GL_FLOAT, 0, 0, 0);
		}
		// buffer for vertex texture coordinates
		if (mesh->HasTextureCoords(0)) {
			float *texCoords = (float *)malloc(sizeof(float) * 2 * mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {

				texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
				texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;

			}
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(texCoordLoc);
			glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, 0, 0, 0);
		}

		unsigned int numUVMaps = mesh->GetNumUVChannels();
		if (numUVMaps > AssimpModel::MAX_UVMAPS) numUVMaps = MAX_UVMAPS;
		//assert(numUVMaps > 0);

		switch (numUVMaps) {
		case 4:
			aMesh.tcoord3 = createVBO(mesh->mTextureCoords[3], mesh->mNumVertices);
		case 3:
			aMesh.tcoord2 = createVBO(mesh->mTextureCoords[2], mesh->mNumVertices);
		case 2:
			aMesh.tcoord1 = createVBO(mesh->mTextureCoords[1], mesh->mNumVertices);
		case 1:
			aMesh.tcoord0 = createVBO(mesh->mTextureCoords[0], mesh->mNumVertices);
		}

		/* How to compute the indices to the matrices and the weights?
		We know that each mesh has its own skeleton, if any. It's
		either a sibling of the mesh node, or a child of the mesh
		node.

		In order words, it's guaranteed that each mesh only has one
		skeleton (no instancing of meshes with armatures).

		So look up the bones in
		m_Scene->mMeshes[i]->mBones[j]. At this point, the bone
		node isn't important. mBones[j] has a aiVertexWeight array.
		It stores mVertexId and mWeight for each vertex. So the
		steps become:

		1.) Iterate over the mBones[j] and add 'j' to boneIndices[k]
		if mVertexID is the current vertex. Add the weight to
		weights[k] as well.
		2.) When later computing bone matrices, associate 'j' with
		the node belonging to mBones[j]
		*/
		if (mesh->HasBones())
		{
			initGLBoneData(&aMesh, n);
		}
		
		// create material uniform buffer
		aiMaterial *mtl = m_Scene->mMaterials[mesh->mMaterialIndex];

		if (mtl != 0)
		{
			loadMat(mtl, aMesh);
		}

		aNode->meshes.push_back(aMesh);

		free(faceArray);
		// unbind buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	// load all children
	for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
		struct AssimpNode *newNode = new struct AssimpNode();
		recursiveLoad(nd->mChildren[n], newModelMat, newNode);
		aNode->children.push_back(newNode);
	}
}

void AssimpModel::loadMat(const aiMaterial* tm, struct MyMesh &aMesh) {

	struct MyMaterial theMat;

	//create 4 aiColor4D objects to represent(R,G,B,A) values of each component
	aiColor4D theDiff, theAmb, theSpec, theEmis;
	//Check for diffuse and assign to the mat struct, if not load default values
	if (AI_SUCCESS == aiGetMaterialColor(tm, AI_MATKEY_COLOR_DIFFUSE, &theDiff)) {
		memcpy(theMat.diff, &theDiff, sizeof(GLfloat) * 4);
	}
	else {
		theMat.diff[0] = 0.5f; theMat.diff[1] = 0.5f; theMat.diff[2] = 0.5f; theMat.diff[3] = 1.0f;
	}

	//Check for ambient and assign to the mat struct, if not load default values
	if (AI_SUCCESS == aiGetMaterialColor(tm, AI_MATKEY_COLOR_AMBIENT, &theAmb)) {
		memcpy(theMat.amb, &theAmb, sizeof(GLfloat) * 4);
	}
	else {
		theMat.amb[0] = 0.2f; theMat.amb[1] = 0.2f; theMat.amb[2] = 0.2f; theMat.amb[3] = 1.0f;
	}

	//Check for specular and assign to the mat struct, if not load default values
	if (AI_SUCCESS == aiGetMaterialColor(tm, AI_MATKEY_COLOR_SPECULAR, &theSpec)) {
		memcpy(theMat.spec, &theSpec, sizeof(GLfloat) * 4);
	}
	else {
		theMat.spec[0] = 0.0f; theMat.spec[1] = 0.0f; theMat.spec[2] = 0.0f; theMat.spec[3] = 1.0f;
	}

	//Check for emisive and assign to the mat struct, if not load default values
	if (AI_SUCCESS == aiGetMaterialColor(tm, AI_MATKEY_COLOR_EMISSIVE, &theEmis)) {
		memcpy(theMat.emis, &theEmis, sizeof(GLfloat) * 4);
	}
	else {
		theMat.emis[0] = 0.0f; theMat.emis[1] = 0.0f; theMat.emis[2] = 0.0f; theMat.emis[3] = 1.0f;
	}

	float shininess = 0.0;
	unsigned int max;
	//finally get the shininess array
	aiGetMaterialFloatArray(tm, AI_MATKEY_SHININESS, &shininess, &max);
	theMat.shininess = shininess;

	theMat.texCount = 0;
	//if the texture count is greater than 0, then....
	if (tm->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		aiString path;
		//if we can get get a material...
		if (tm->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
			//create a string with the correct path
			std::string fp = sModelDir + "/" + path.data;
			//print->loading("Texture Diffuse ", fp);
			//printf("Loading Texture, diffuse - %s\n", fp.c_str());
			//use the SOIL library to load the texture into memory
			aMesh.matTex = loadTexture(fp);
			//if success then continue, else print error
			if (aMesh.matTex == 0) {	
				printf("ERROR, failed to load texture: %s\n", fp.c_str());
			}
			else
			{
				theMat.texCount = 1;;
				printf("Succesfully loaded diffuse %s\n", fp.c_str());
			}
		}
	}

	theMat.normTexCount = 0;
	// If the model has normal maps grab em and load em yo
	if (tm->GetTextureCount(aiTextureType_HEIGHT) > 0) {
		aiString path;
		if (tm->GetTexture(aiTextureType_HEIGHT, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
			std::string fp = sModelDir + "/" + path.data;
			//print->loading("Texture Normal ", fp);
			//printf("Loading Texture, normal - %s\n", fp.c_str());
			aMesh.matNorm = loadTexture(fp);
			if (aMesh.matNorm == 0) {
				theMat.normTexCount = 1;
				printf("ERROR, failed to load texture: %s\n", fp.c_str());
			}
			else
				//print->loadingComp();
				printf("Succesfully loaded normal texture %s\n", fp.c_str());
		}
	}

	glGenBuffers(1, &(aMesh.uniformBlockIndex));
	glBindBuffer(GL_UNIFORM_BUFFER, aMesh.uniformBlockIndex);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(theMat), (void *)(&theMat), GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void AssimpModel::initGLBoneData(struct MyMesh* gldata, int meshID)
{
	std::vector<std::vector<float> > weightArray; //one weight per bone
	std::vector<std::vector<unsigned int> > boneArray; //one index per bone found

	const aiMesh* mesh = m_Scene->mMeshes[meshID];
	weightArray.resize(mesh->mNumVertices);
	boneArray.resize(mesh->mNumVertices);
	assert(mesh->mNumBones <= AssimpModel::MAXBONESPERMESH);

	int numBones = mesh->mNumBones;
	if (numBones > AssimpModel::MAXBONESPERMESH)
		numBones = AssimpModel::MAXBONESPERMESH;
	for (int i = 0; i < numBones; ++i) {
		const aiBone* bone = mesh->mBones[i];
		for (int j = 0; j < bone->mNumWeights; ++j) {
			int vertexIdx = bone->mWeights[j].mVertexId;
			float weight = bone->mWeights[j].mWeight;
			boneArray[vertexIdx].push_back(i);
			weightArray[vertexIdx].push_back(weight);
		}
		//Associate bone 'i' with its node so we can later easily
		//check if a bone needs updating. NOTE! This assumes that
		//multiple bones can not have the same name or refer to the
		//same node!
		//Update: Got a model where a node refer to a bone shared by several meshes
		//m_LUTBone now refers to an array of nmbis
		NodeMeshBoneIndex nmbi;
		nmbi.meshIndex = meshID;
		nmbi.boneIndex = i;
		const aiNode* boneNode = m_Scene->mRootNode->FindNode(bone->mName);
		assert(boneNode != 0);
		//m_LUTBone.insert(std::make_pair(boneNode, nmbi));
		//A node which refers to a bone, can refer to a bone shared by
		//several meshes. If several meshes share a bone, the bone can
		//have different indices (different bone array ordering)
		m_LUTBone[boneNode].push_back(nmbi);
	}

	assert(boneArray.size() == mesh->mNumVertices);
	assert(weightArray.size() == mesh->mNumVertices);

	// Assert for greater bones-per-vertex limit than allowed
#if 0
	std::vector<float> cmpVal;
	cmpVal.resize(AssimpModel::MAX_BONES_PER_VERTEX);
	assert(
		std::upper_bound(
			weightArray.begin(), weightArray.end(), cmpVal,
			[](const std::vector<float>& v1, const std::vector<float>& v2) -> bool {
		return v1.size() < v2.size();
	}) == weightArray.end());

#endif

	std::vector<unsigned int> boneArrayFinal;
	std::vector<float> weightArrayFinal;

	boneArrayFinal.resize(mesh->mNumVertices * AssimpModel::MAX_BONES_PER_VERTEX);
	weightArrayFinal.resize(mesh->mNumVertices * AssimpModel::MAX_BONES_PER_VERTEX);

	/* Finally, create a flat array for OpenGL
	OBS: Currently this code expects AssimpModel::MAXBONESPERVERTEX to be 4!
	*/
	for (int i = 0; i < mesh->mNumVertices; ++i) {
		int idx = i*AssimpModel::MAX_BONES_PER_VERTEX;
		const std::vector<unsigned int>& ba = boneArray[i];
		const std::vector<float>& wa = weightArray[i];
		assert(ba.size() <= AssimpModel::MAX_BONES_PER_VERTEX);
		assert(wa.size() <= AssimpModel::MAX_BONES_PER_VERTEX);
		assert(ba.size() == wa.size());
		int len = ba.size();

		for (int j = 0; j < AssimpModel::MAX_BONES_PER_VERTEX; ++j) {
			boneArrayFinal[idx + j] = 0;
			weightArrayFinal[idx + j] = 0;
		}
		for (int j = 0; j < len; ++j) {
			boneArrayFinal[idx + j] = ba[j];
			weightArrayFinal[idx + j] = wa[j];
		}
	}
	gldata->boneIndices = createVBO(&boneArrayFinal[0], mesh->mNumVertices * 4);
	gldata->weights = createVBO(&weightArrayFinal[0], mesh->mNumVertices * 4);
}

AnimGLData* AssimpModel::createAnimation(const std::string& name, const aiMatrix4x4& camera)
{
	AnimGLData* animation = new AnimGLData;
	animation->m_Scene = this;
	animation->m_Animation = 0;
	//animation->m_Renderer = 0;
	animation->m_Bones.resize(m_Scene->mNumMeshes);
	animation->m_ModelView.resize(m_Scene->mNumMeshes);
	animation->m_Time = 0.0f;
	animation->m_Camera = camera;

	/* Linear search for animation name */
	for (int i = 0; i < m_Scene->mNumAnimations; ++i) {
		std::string animName(m_Scene->mAnimations[i]->mName.C_Str());
		if (animName == name) {
			animation->m_Animation = m_Scene->mAnimations[i];
			break;
		}
	}
	if (!animation->m_Animation)
		return 0;
	assert(animation->m_Animation != 0);


	/* Resize bone array for each model properly */
	for (int i = 0; i < m_Scene->mNumMeshes; ++i) {
		//Set the number of bones to the max limit
		//GLSL requires an array of constant size
		int numBones = AssimpModel::MAXBONESPERMESH; //m_Scene->mMeshes[i]->mNumBones;
		animation->m_Bones[i].resize(numBones);
	}

	//Initial update of all the matrices in the node structure, so the
	//first rendered frame works. Implicitly reads animation->m_Time,
	//which starts at 0.0f
	aiMatrix4x4 rootMatrix;
	animation->recursiveUpdate(m_Scene->mRootNode, rootMatrix);
	return animation;
}


AnimGLData* AssimpModel::createAnimation(unsigned int anim, const aiMatrix4x4& camera)
{
	AnimGLData* animation = new AnimGLData;
	animation->m_Scene = this;
	animation->m_Animation = 0;
	//animation->m_Renderer = 0;
	animation->m_Bones.resize(m_Scene->mNumMeshes);
	animation->m_ModelView.resize(m_Scene->mNumMeshes);
	animation->m_Time = 0.0f;
	animation->m_Camera = camera;
	animation->m_Animation = m_Scene->mAnimations[anim];

	assert(animation->m_Animation != 0);

	/* Resize bone array for each model properly */
	for (int i = 0; i < m_Scene->mNumMeshes; ++i) {
		//Set the number of bones to the max limit
		//GLSL requires an array of constant size
		int numBones = AssimpModel::MAXBONESPERMESH; //m_Scene->mMeshes[i]->mNumBones;
		animation->m_Bones[i].resize(numBones);
	}

	//Initial update of all the matrices in the node structure, so the
	//first rendered frame works. Implicitly reads animation->m_Time,
	//which starts at 0.0f
	aiMatrix4x4 rootMatrix;
	animation->recursiveUpdate(m_Scene->mRootNode, rootMatrix);
	return animation;
}


/****************************************************************************************
********************************* AnimRenderer *****************************************
****************************************************************************************/
AnimRenderer::AnimRenderer() : m_Parent(0), m_Scene(0), m_CurrentMesh(-1) {

}

#if 0
void AnimRenderer::drawObjectBegin(unsigned int shader, const std::string& objname)
{
	int i;
	const aiScene* sceneData = m_Scene->m_Scene;
	// TODO: Do this faster than O(n). Maybe add a hashmap 
	for (i = 0; i < sceneData->mNumMeshes; ++i) {
		std::string meshName(sceneData->mMeshes[i]->mName.C_Str());
		if (meshName == objname)
			break;
	}
	assert(i < sceneData->mNumMeshes);
	m_CurrentMesh = i;
	const MeshGLData* meshData = m_Scene->getMeshGLData(m_CurrentMesh);

	glUseProgram(shader);
	bindVAO(meshData->vao);
	bindVBOFloat(shader, "sc_vertex", meshData->vertices, 3);
	bindVBOFloat(shader, "sc_normal", meshData->normals, 3);
	bindVBOFloat(shader, "sc_tangent", meshData->tangents, 3);
	bindVBOFloat(shader, "sc_bitangent", meshData->bitangents, 3);
	bindVBOFloat(shader, "sc_tcoord0", meshData->tcoord0, 3);
	bindVBOFloat(shader, "sc_tcoord1", meshData->tcoord1, 3);
	bindVBOFloat(shader, "sc_tcoord2", meshData->tcoord2, 3);
	bindVBOFloat(shader, "sc_tcoord3", meshData->tcoord3, 3);
	bindVBOUint(shader, "sc_index", meshData->boneIndices, 4);
	bindVBOFloat(shader, "sc_weight", meshData->weights, 4);

	//Bone uniform array changes every frame
	//so it's stored in struct AnimGLData, this AnimRenderer's parent
	int numBones = m_Parent->m_Bones[i].size();
	const std::vector<aiMatrix4x4>& bones = m_Parent->m_Bones[m_CurrentMesh];
	bindUniformMatrix4Array(shader, "sc_bones", numBones, &bones[0]);
	bindUniformMatrix4(shader, "sc_modelview", m_Parent->m_ModelView[m_CurrentMesh]);
	bindUniformMatrix4(shader, "sc_camera", m_Parent->m_Camera);
	//Finally, bind the face indices
	bindVBOIndices(shader, meshData->indices);
}

void AnimRenderer::drawObjectEnd()
{
	assert(m_CurrentMesh != -1);
	const MeshGLData* meshData = m_Scene->getMeshGLData(m_CurrentMesh);
	glDrawElements(GL_TRIANGLES, meshData->numElements, GL_UNSIGNED_INT, 0);
	m_CurrentMesh = -1;
}

void AnimRenderer::drawAllObjects(unsigned int shader)
{
	glUseProgram(shader);
	const aiScene* sceneData = m_Scene->m_Scene;
	for (int i = 0; i < sceneData->mNumMeshes; ++i) {
		m_CurrentMesh = i;
		const MeshGLData* meshData = m_Scene->getMeshGLData(m_CurrentMesh);
		bindVAO(meshData->vao);
		bindVBOFloat(shader, "sc_vertex", meshData->vertices, 3);
		bindVBOFloat(shader, "sc_normal", meshData->normals, 3);
		bindVBOFloat(shader, "sc_tangent", meshData->tangents, 3);
		bindVBOFloat(shader, "sc_bitangent", meshData->bitangents, 3);
		bindVBOFloat(shader, "sc_tcoord0", meshData->tcoord0, 3);
		bindVBOFloat(shader, "sc_tcoord1", meshData->tcoord1, 3);
		bindVBOFloat(shader, "sc_tcoord2", meshData->tcoord2, 3);
		bindVBOFloat(shader, "sc_tcoord3", meshData->tcoord3, 3);
		bindVBOUint(shader, "sc_index", meshData->boneIndices, 4);
		bindVBOFloat(shader, "sc_weight", meshData->weights, 4);

		//Bone uniform array changes every frame
		//so it's stored in struct AnimGLData, this AnimRenderer's parent
		int numBones = m_Parent->m_Bones[i].size();
		const std::vector<aiMatrix4x4>& bones = m_Parent->m_Bones[i];
		bindUniformMatrix4Array(shader, "sc_bones", numBones, &bones[0]);
		bindUniformMatrix4(shader, "sc_modelview", m_Parent->m_ModelView[m_CurrentMesh]);
		bindUniformMatrix4(shader, "sc_camera", m_Parent->m_Camera);


		const aiMatrix4x4& c = m_Parent->m_Camera;
		const float* cp = c[0];

		//Finally, bind the face indices
		bindVBOIndices(shader, meshData->indices);
		glDrawElements(GL_TRIANGLES, meshData->numElements, GL_UNSIGNED_INT, 0);
	}
	m_CurrentMesh = -1;
}

#endif

void AnimRenderer::drawBegin(unsigned int shader, int idx)
{
	glUseProgram(shader);
	const aiScene* sceneData = m_Scene->m_Scene;
	m_CurrentMesh = idx;
	const MeshGLData* meshData = m_Scene->getMeshGLData(m_CurrentMesh);
	bindVAO(meshData->vao);
	bindVBOFloat(shader, "sc_vertex", meshData->vertices, 3);
	bindVBOFloat(shader, "sc_normal", meshData->normals, 3);
	bindVBOFloat(shader, "sc_tangent", meshData->tangents, 3);
	bindVBOFloat(shader, "sc_bitangent", meshData->bitangents, 3);
	bindVBOFloat(shader, "sc_tcoord0", meshData->tcoord0, 3);
	bindVBOFloat(shader, "sc_tcoord1", meshData->tcoord1, 3);
	bindVBOFloat(shader, "sc_tcoord2", meshData->tcoord2, 3);
	bindVBOFloat(shader, "sc_tcoord3", meshData->tcoord3, 3);
	bindVBOUint(shader, "sc_index", meshData->boneIndices, 4);
	bindVBOFloat(shader, "sc_weight", meshData->weights, 4);

	//Bone uniform array changes every frame
	//so it's stored in struct AnimGLData, this AnimRenderer's parent
	int numBones = m_Parent->m_Bones[m_CurrentMesh].size();
	const std::vector<aiMatrix4x4>& bones = m_Parent->m_Bones[m_CurrentMesh];
	bindUniformMatrix4Array(shader, "sc_bones", numBones, &bones[0]);
	bindUniformMatrix4(shader, "sc_modelview", m_Parent->m_ModelView[m_CurrentMesh]);
	bindUniformMatrix4(shader, "sc_camera", m_Parent->m_Camera);


	const aiMatrix4x4& c = m_Parent->m_Camera;

	//Finally, bind the face indices
	bindVBOIndices(shader, meshData->indices);
	glDrawElements(GL_TRIANGLES, meshData->numElements, GL_UNSIGNED_INT, 0);
}

void AnimRenderer::drawEnd(int idx)
{
	const MeshGLData* meshData = m_Scene->getMeshGLData(idx);
	glDrawElements(GL_TRIANGLES, meshData->numElements, GL_UNSIGNED_INT, 0);
}

void AnimRenderer::draw(int idx)
{
	//override this and use drawObjectBegin()/drawObjectEnd and drawAllObjects() as needed
}

void AnimRenderer::setParent(AnimGLData* parent)
{
	m_Parent = parent;
}

void AnimRenderer::setScene(const AssimpModel* scene)
{
	m_Scene = scene;
}



/****************************************************************************************
********************************* AnimGLData** *****************************************
****************************************************************************************/
/*
void AnimGLData::addRenderer(AnimRenderer* renderer)
{
renderer->setParent(this);
renderer->setScene(m_Scene);
m_Renderer = renderer;
}

void AnimGLData::removeRenderer()
{
m_Renderer = 0; //cleanup outside of this renderer
}

*/

//add renderer to model with index 'modelIndex'. Returns 'modelIndex'
int AnimGLData::addRenderer(AnimRenderer* renderer, int modelIndex)
{
	renderer->setParent(this);
	renderer->setScene(m_Scene);
	if (modelIndex < m_Renderer.size())
		m_Renderer[modelIndex] = renderer;
	return modelIndex;
}
//add renderer to mode with name 'modelName'. Returns index of model
int AnimGLData::addRenderer(AnimRenderer* renderer, const std::string modelName)
{
	renderer->setParent(this);
	renderer->setScene(m_Scene);
	const aiScene* scene = m_Scene->m_Scene;
	unsigned int idx = 0;
	for (; idx < scene->mNumMeshes; ++idx) {
		std::string name = scene->mMeshes[idx]->mName.C_Str();
		if (name == modelName) break;
	}

	if (idx >= scene->mNumMeshes) return -1;
	m_Renderer[idx] = renderer;
	return idx;
}
//Removes renderer attached to model with index 'modelIndex'
void AnimGLData::removeRenderer(int modelIndex)
{
	m_Renderer[modelIndex] = 0;
	//m_Renderer.erase(ModelIndex)
}


void AnimGLData::stepAnimation(float t) //step one frame forwards
{
	const aiScene* sceneData = m_Scene->m_Scene;
	float step;
	if (m_Animation->mTicksPerSecond != 0.0f)
		step = m_Animation->mTicksPerSecond;
	else
		step = 32.0f;

	m_Time = t * step; //Used as time position by recursiveUpdate

					   //Run recursive node updates here, with current camera
	recursiveUpdate(sceneData->mRootNode, m_Camera);
}

void AnimGLData::render(float t)
{
	stepAnimation(t);
	//if(m_Renderer)
	//	m_Renderer->draw();
}

void AnimGLData::setCamera(const aiMatrix4x4& camera)
{
	m_Camera = camera;
}


//For an animated node (an aiNodeAnim channel), get the interpolated position
void AnimGLData::interpolateTranslation(const aiNodeAnim* nodeAnim, aiVector3D& translation)
{
	bool positionFound = false;
	for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; ++i) {
		const aiVectorKey& key1 = nodeAnim->mPositionKeys[i];
		const aiVectorKey& key2 = nodeAnim->mPositionKeys[i + 1];
		if (key1.mTime <= m_Time && m_Time <= key2.mTime) {
			float tDelta = key2.mTime - key1.mTime;
			float t = (m_Time - key1.mTime) / tDelta;
			//float t = m_Time - key1.mTime;
			translation[0] = key1.mValue[0] + (key2.mValue[0] - key1.mValue[0])*t;
			translation[1] = key1.mValue[1] + (key2.mValue[1] - key1.mValue[1])*t;
			translation[2] = key1.mValue[2] + (key2.mValue[2] - key1.mValue[2])*t;
			positionFound = true;
			break;
		}
	}
	// If the time isn't inside the animation channel timeframe, snap to the
	//   closest frame, which is either the first or the last
	//   frame.
	// Another solution would be to compute the length, and do a
	//   modulo on the time so it repeats
	if (!positionFound) {
		float firstFrameTime = nodeAnim->mPositionKeys[0].mTime;
		float lastFrameTime = nodeAnim->mPositionKeys[nodeAnim->mNumPositionKeys - 1].mTime;
		const aiVector3D& firstFrameValue = nodeAnim->mPositionKeys[0].mValue;
		const aiVector3D& lastFrameValue = nodeAnim->mPositionKeys[nodeAnim->mNumPositionKeys - 1].mValue;
		if (m_Time < firstFrameTime) {
			translation = firstFrameValue;
		}
		else if (m_Time > lastFrameTime) {
			translation = lastFrameValue;
		}
		else {
			//Shouldn't get here. Either we find a time between
			//two frames, or we get to pick the first or last frame
			assert(false);
		}
	}
}

//For an animated node (an aiNodeAnim channel), get the interpolated scale
void AnimGLData::interpolateScale(const aiNodeAnim* nodeAnim, aiVector3D& scale)
{
	bool scaleFound = false;
	for (int i = 0; i < nodeAnim->mNumScalingKeys - 1; ++i) {
		const aiVectorKey& key1 = nodeAnim->mScalingKeys[i];
		const aiVectorKey& key2 = nodeAnim->mScalingKeys[i + 1];
		if (key1.mTime <= m_Time && m_Time <= key2.mTime) {
			float tDelta = key2.mTime - key1.mTime;
			float t = (m_Time - key1.mTime) / tDelta;
			//float t = m_Time - key1.mTime;
			scale[0] = key1.mValue[0] + (key2.mValue[0] - key1.mValue[0])*t;
			scale[1] = key1.mValue[1] + (key2.mValue[1] - key1.mValue[1])*t;
			scale[2] = key1.mValue[2] + (key2.mValue[2] - key1.mValue[2])*t;
			scaleFound = true;
			break;
		}
	}
	// If the time isn't inside the animation channel timeframe, snap to the
	//   closest frame, which is either the first or the last
	//   frame.
	// Another solution would be to compute the length, and do a
	//   modulo on the time so it repeats
	if (!scaleFound) {
		float firstFrameTime = nodeAnim->mScalingKeys[0].mTime;
		float lastFrameTime = nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1].mTime;
		const aiVector3D& firstFrameValue = nodeAnim->mScalingKeys[0].mValue;
		const aiVector3D& lastFrameValue = nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1].mValue;
		if (m_Time < firstFrameTime) {
			scale = firstFrameValue;
		}
		else if (lastFrameTime) {
			scale = lastFrameValue;
		}
		else {
			//Shouldn't get here. Either we find a time between
			//two frames, or we get to pick the first or last frame
			assert(false);
		}
	}
}

//For an animated node (an aiNodeAnim channel), get the interpolated rotation
void AnimGLData::interpolateRotation(const aiNodeAnim* nodeAnim, aiQuaternion& rotation)
{
	bool rotationFound = false;
	// TODO: Make this faster than O(n)
	for (int i = 0; i < nodeAnim->mNumRotationKeys - 1; ++i) {
		const aiQuatKey& key1 = nodeAnim->mRotationKeys[i];
		const aiQuatKey& key2 = nodeAnim->mRotationKeys[i + 1];
		//printf("rot key timestamps: %lf %lf\n", key1.mTime, key2.mTime);
		if (key1.mTime <= m_Time && m_Time <= key2.mTime) {
			float tDelta = key2.mTime - key1.mTime;
			float t = (m_Time - key1.mTime) / tDelta;
			//float t = m_Time - key1.mTime;
			aiQuaternion::Interpolate(rotation, key1.mValue, key2.mValue, t);
			//rotation = key1.mValue;
			rotationFound = true;
			break;
		}
	}
	// If the time isn't inside the channel timeframe, snap to the
	//   closest frame, which is either the first or the last
	//   frame.
	// Another solution would be to compute the length, and do a
	//   modulo on the time so it repeats
	if (!rotationFound) {
		float firstFrameTime = nodeAnim->mRotationKeys[0].mTime;
		float lastFrameTime = nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1].mTime;
		const aiQuaternion& firstFrameValue = nodeAnim->mRotationKeys[0].mValue;
		const aiQuaternion& lastFrameValue = nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1].mValue;
		/*
		aiQuaternion firstFrameValue = nodeAnim->mRotationKeys[0].mValue;
		aiQuaternion lastFrameValue = nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1].mValue;
		firstFrameValue.Normalize();
		lastFrameValue.Normalize();
		*/
		if (m_Time < firstFrameTime) {
			rotation = firstFrameValue;
		}
		else if (m_Time > lastFrameTime) {
			rotation = lastFrameValue;
		}
		else {
			//Shouldn't get here. Either we find a time between
			//two frames, or we get to pick the first or last frame
			assert(false);
		}
	}
}

/* Recursively update the coordinate systems of nodes, including bones */
void AnimGLData::recursiveUpdate(aiNode* node, const aiMatrix4x4& parentMatrix)
{
	std::string nodeName(node->mName.C_Str());
	aiMatrix4x4 localMatrix = node->mTransformation;

	//find this current node in the animation
	//TODO: Make this faster than O(n)
	//Note: setting the m_Animation pointer to 0 effectively disables animation
	const aiNodeAnim* nodeAnim = 0;
	for (unsigned int i = 0; i < m_Animation->mNumChannels && m_Animation; ++i) {
		aiNodeAnim* anim = m_Animation->mChannels[i];
		std::string animNodeName(anim->mNodeName.C_Str());
		if (animNodeName == nodeName) {
			nodeAnim = anim;
			break;
		}
	}

	// Animate this node if we found an animation channel for it earlier
	// Replaces localMatrix
	if (nodeAnim) {
		aiVector3D translation;
		aiVector3D scale;
		aiQuaternion rotation;
		aiMatrix4x4 scaleMat, rotMat, transMat;
		//printf("Animated node: %s at time %f\n", node->mName.C_Str(), m_Time);
		interpolateTranslation(nodeAnim, translation);
		interpolateScale(nodeAnim, scale);
		interpolateRotation(nodeAnim, rotation);
		rotMat = aiMatrix4x4(rotation.GetMatrix());
		aiMatrix4x4::Scaling(scale, scaleMat);
		aiMatrix4x4::Translation(translation, transMat);
		localMatrix = transMat * rotMat; // * scaleMat;
	}

	aiMatrix4x4 globalMatrix = parentMatrix * localMatrix;

	/* Look up node in NMBI lookup table. If it is a bone, update the
	i'th bone in the j'th mesh. The "bone" we update is the 2D matrix
	array used by OpenGL as uniforms. Each array in the 2D array
	belongs to a mesh. */
	bool isBone = false;
	const aiScene* sceneData = m_Scene->m_Scene;
	std::map<const aiNode*, std::vector<NodeMeshBoneIndex> >::const_iterator it = m_Scene->m_LUTBone.find(node);
	isBone = (it != m_Scene->m_LUTBone.end());
	if (isBone) {
		const std::vector<NodeMeshBoneIndex>& nmbi = it->second;

		for (unsigned int i = 0; i < nmbi.size(); ++i) {
			const NodeMeshBoneIndex& idx = nmbi[i];
			const aiMatrix4x4& offsetMatrix = sceneData->mMeshes[idx.meshIndex]->mBones[idx.boneIndex]->mOffsetMatrix;
			aiMatrix4x4 boneMatrix = globalMatrix * offsetMatrix;
			//Update the 'nmbi.meshIndex'th mesh, bone number 'nmbi.boneIndex' 
			//OpenGL uses one uniform array for each mesh as bone matrices
			//Now we support that a bone can be shared by multiple meshes

			m_Bones[idx.meshIndex][idx.boneIndex] = boneMatrix;
		}

	}
	for (unsigned int i = 0; i < node->mNumChildren; ++i)
		recursiveUpdate(node->mChildren[i], globalMatrix);

	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		/* Global world transform for meshes in pose mode (no animation running) */
		m_ModelView[node->mMeshes[i]] = globalMatrix;
		AnimRenderer* a = m_Renderer[node->mMeshes[i]];
		if (a) a->draw(node->mMeshes[i]);
	}
}

//The expressions column major and row major denotes how a matrix is stored in memory. 
//OpenGL and glm use column major, DirectX and Assimp use row major
void CopyaiMat(const aiMatrix4x4 *from, glm::mat4 &to)
{
	to[0][0] = from->a1; to[1][0] = from->a2; to[2][0] = from->a3; to[3][0] = from->a4;
	to[0][1] = from->b1; to[1][1] = from->b2; to[2][1] = from->b3; to[3][1] = from->b4;
	to[0][2] = from->c1; to[1][2] = from->c2; to[2][2] = from->c3; to[3][2] = from->c4;
	to[0][3] = from->d1; to[1][3] = from->d2; to[2][3] = from->d3; to[3][3] = from->d4;
}

CCamera::CCamera() :
	m_viewMatrix(1.0f),
	m_modelMatrix(1.0f),
	m_projectionMatrix(1.0f),
	m_projectionViewMatrix(1.0f),
	m_eyeGlobal(glm::vec3(1.0f, 0.0f, 0.0f)),
	m_centerGlobal(glm::vec3(0.0f, 0.0f, 0.0f)),
	m_upGlobal(glm::vec3(0.0f, 1.0f, 0.0f)),
	m_xGlob(0),
	m_yGlob(0),
	m_zGlob(0),
	m_camera(nullptr),
	m_ID(0)
{
	m_modelMatrix = glm::translate(m_modelMatrix, glm::vec3(0.0f, 0.0f, -1.0f));
}

CCamera::CCamera(uint32_t id, aiCamera* camera) :
	m_viewMatrix(1.0f),
	m_modelMatrix(),
	m_projectionMatrix(1.0f),
	m_projectionViewMatrix(1.0f),
	m_eyeGlobal(glm::vec3(camera->mLookAt.x, camera->mLookAt.y, camera->mLookAt.z)),
	m_centerGlobal(glm::vec3(camera->mPosition.x, camera->mPosition.y, camera->mPosition.z)),
	m_upGlobal(glm::vec3(camera->mUp.x, camera->mUp.y, camera->mUp.z)),
	m_xGlob(0),
	m_yGlob(0),
	m_zGlob(0),
	m_camera(camera),
	m_ID(id)
{

}

CCamera::~CCamera() {

}

void CCamera::translate(glm::vec3& translateVector)
{
	glm::vec3 l_z = translateVector.z*glm::normalize(m_zGlob);
	glm::vec3 l_y = translateVector.y*glm::normalize(m_yGlob);
	glm::vec3 l_x = translateVector.x*glm::normalize(m_xGlob);

	glm::vec3 t0 = l_z + l_y + l_x;

	m_eyeGlobal += t0;
	m_centerGlobal += t0;

	updateViewMatrix();
	updateAxis();
}

void CCamera::translate(glm::vec3&& translateVector)
{
	glm::vec3 l_z = translateVector.z*glm::normalize(m_zGlob);
	glm::vec3 l_y = translateVector.y*glm::normalize(m_yGlob);
	glm::vec3 l_x = translateVector.x*glm::normalize(m_xGlob);

	glm::vec3 t0 = l_z + l_y + l_x;

	m_eyeGlobal += t0;
	m_centerGlobal += t0;

	updateViewMatrix();
	updateAxis();
}

void CCamera::updateViewMatrix()
{
	m_viewMatrix = glm::lookAt(m_eyeGlobal, m_centerGlobal, m_upGlobal);
	m_projectionViewMatrix = m_projectionMatrix*m_viewMatrix;
}

const glm::mat4 CCamera::getViewMatrix() const
{
	return m_viewMatrix;
}

const glm::mat4& CCamera::getProjectionMatrix()
{
	return m_projectionMatrix;
}

uint32_t CCamera::getID()
{
	return m_ID;
}

std::string CCamera::getName()
{
	return m_camera->mName.data;
}

aiCamera* CCamera::getCamera()
{
	return m_camera;
}

const glm::mat4& CCamera::getProjectionViewMatrix()
{
	return m_projectionViewMatrix;
}

void CCamera::init(aiNode* root)
{
	float fov;
	if (m_camera->mHorizontalFOV < 10)
	{
		fov = 45;
	}
	else
	{
		fov = m_camera->mHorizontalFOV;
	}
	float aspect = m_camera->mAspect;
	if (aspect < 0.01)
	{
		aspect = 1280.0 / 720.0f;
	}
	// Create Projection Matrix
	m_projectionMatrix = glm::perspective(glm::radians(fov), aspect, m_camera->mClipPlaneNear, m_camera->mClipPlaneFar);


	// Create AssimpModel Matrix

	//     // Get the camera matrix for a camera at a specific time
	//     // if the node hierarchy for the camera does not contain
	//     // at least one animated node this is a static computation
	//     get-camera-matrix (node sceneRoot, camera cam) : matrix
	//     {
	//         node   cnd = find-node-for-camera(cam)
	//         matrix cmt = identity()
	//         
	//         // as usual - get the absolute camera transformation for this frame
	//         for each node nd in hierarchy from sceneRoot to cnd
	//             matrix cur
	//             if (is-animated(nd))
	//                 cur = eval-animation(nd)
	//             else cur = nd->mTransformation;
	//                 cmt = mult-matrices( cmt, cur )
	//         end for
	//         
	//         // now multiply with the camera's own local transform
	//         cam = mult-matrices (cam, get-camera-matrix(cmt) )
	//     }

	aiNode* myNode = root->FindNode(m_camera->mName);
	if (myNode == nullptr) {
		m_modelMatrix = glm::mat4(1);
	}
	aiMatrix4x4 m4;
	while (myNode != root) {
		m4 = myNode->mTransformation * m4;
		myNode = myNode->mParent;
	}

	aiMatrix4x4 aim;
	m_camera->GetCameraMatrix(aim);
	m4 = aim * (myNode->mTransformation * m4);

	CopyaiMat(&m4, m_modelMatrix);

	updateGlobals();
	updateAxis();

	updateViewMatrix();
}

void CCamera::updateGlobals()
{
	glm::vec4 eG = m_modelMatrix*glm::vec4(m_eyeGlobal, 0.0f);
	m_eyeGlobal = glm::normalize(glm::vec3(eG.x, eG.y, eG.z));

	glm::vec4 cG = m_modelMatrix*glm::vec4(m_centerGlobal, 0.0f);
	m_centerGlobal = glm::vec3(cG.x, cG.y, cG.z);

	glm::vec4 uG = m_modelMatrix*glm::vec4(m_upGlobal, 0.0f);
	m_upGlobal = glm::normalize(glm::vec3(uG.x, uG.y, uG.z));
}

void CCamera::updateAxis()
{
	m_yGlob = m_upGlobal;
	m_zGlob = m_centerGlobal - m_eyeGlobal;
	m_xGlob = glm::cross(m_yGlob, m_zGlob);
}

// TODO: use quaternions for rotation (but works already fine)
void CCamera::pitchYawRoll(float pitchAngle, float yawAngle, float rollAngle)
{
	pitch(pitchAngle);
	yaw(yawAngle);
	roll(rollAngle);
}

void CCamera::pitchYawRoll(glm::vec3& pyaAngles)
{
	pitch(pyaAngles.x);
	yaw(pyaAngles.y);
	roll(pyaAngles.z);
}

void CCamera::pitchYawRoll(glm::vec3&& pyaAngles)
{
	pitch(pyaAngles.x);
	yaw(pyaAngles.y);
	roll(pyaAngles.z);
}

void CCamera::roll(float angle) // local z
{
	glm::vec3 diffRotZ = glm::rotate(m_yGlob, angle, m_zGlob) - m_yGlob;
	m_upGlobal += diffRotZ;
	updateViewMatrix();
	updateAxis();
}

void CCamera::pitch(float angle) // local X
{
	glm::vec3 diffCenter = glm::rotate(m_zGlob, -angle, m_xGlob) - m_zGlob;
	glm::vec3 diffUp = glm::rotate(m_yGlob, -angle, m_xGlob) - m_yGlob;
	m_centerGlobal += diffCenter;
	m_upGlobal += diffUp;
	updateViewMatrix();
	updateAxis();
}

void CCamera::yaw(float angle) // local Y (up)
{
	glm::vec3 diffLR = glm::rotate(m_zGlob, -angle, m_yGlob) - m_zGlob;
	m_centerGlobal += diffLR;
	updateViewMatrix();
	updateAxis();
}
