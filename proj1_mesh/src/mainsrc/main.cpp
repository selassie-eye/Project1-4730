// main.cpp
//
//
// For this project, we use OpenGL, GLUT
// and GLEW (to load OpenGL extensions)
//
#include "stglew.h"

#include <stdio.h>
#include <string.h>
#include<map>

//
// Globals used by this application.
// As a rule, globals are Evil, but this is a small application
// and the design of GLUT makes it hard to avoid them.
//
int globalCount;

typedef struct {
  int i1;
  int i2;
  int i3;
} TriangleIndices;


// Window size, kept for screenshots
static int gWindowSizeX = 0;
static int gWindowSizeY = 0;

// File locations
std::string vertexShader;
std::string fragmentShader;
std::string normalMap;
std::string displacementMap;
std::string meshOBJ;

// Light source attributes
static float ambientLight[]  = {0.20, 0.20, 0.20, 1.0};
static float diffuseLight[]  = {1.00, 1.00, 1.00, 1.0};
static float specularLight[] = {1.00, 1.00, 1.00, 1.0};

float lightPosition[] = {10.0f, 15.0f, 10.0f, 1.0f};

STImage   *surfaceNormImg;
STTexture *surfaceNormTex;

STImage   *surfaceDisplaceImg;
STTexture *surfaceDisplaceTex;

STShaderProgram *shader;

STVector3 mPosition;
STVector3 mLookAt;
STVector3 mRight;
STVector3 mUp;

// Stored mouse position for camera rotation, panning, and zoom.
int gPreviousMouseX = -1;
int gPreviousMouseY = -1;
int gMouseButton = -1;
bool mesh = false; // draw mesh
bool smooth = true; // smooth/flat shading for mesh
bool normalMapping = true; // true=normalMapping, false=displacementMapping
bool proxyType=false; // false: use cylinder; true: use sphere

std::vector<STTriangleMesh*> gTriangleMeshes;
STPoint3 gMassCenter;
std::pair<STPoint3,STPoint3> gBoundingBox;

STTriangleMesh* gManualTriangleMesh = 0;

int TesselationDepth = 100;

void SetUpAndRight()
{
    mRight = STVector3::Cross(mLookAt - mPosition, mUp);
    mRight.Normalize();
    mUp = STVector3::Cross(mRight, mLookAt - mPosition);
    mUp.Normalize();
}

void resetCamera()
{
    mLookAt=STVector3(0.f,0.f,0.f);
    mPosition=STVector3(0.f,5.f,15.f);
    mUp=STVector3(0.f,1.f,0.f);

    SetUpAndRight();
}

void resetUp()
{
    mUp = STVector3(0.f,1.f,0.f);
    mRight = STVector3::Cross(mLookAt - mPosition, mUp);
    mRight.Normalize();
    mUp = STVector3::Cross(mRight, mLookAt - mPosition);
    mUp.Normalize();
}

void CreateYourOwnMesh()
{
    float leftX   = -2.0f;
    float rightX  = -leftX;
    float nearZ   = -2.0f;
    float farZ    = -nearZ;

    gManualTriangleMesh= new STTriangleMesh();
    for (int i = 0; i < TesselationDepth+1; i++){
        for (int j = 0; j < TesselationDepth+1; j++) {
            float s0 = (float) i / (float) TesselationDepth;
            float x0 =  s0 * (rightX - leftX) + leftX;
            float t0 = (float) j / (float) TesselationDepth;
            float z0 = t0 * (farZ - nearZ) + nearZ;

            gManualTriangleMesh->AddVertex(x0,(x0*x0+z0*z0)*0.0f,z0,s0,t0);
        }
    }
    for (int i = 0; i < TesselationDepth; i++){
        for (int j = 0; j < TesselationDepth; j++) {
            unsigned int id0=i*(TesselationDepth+1)+j;
            unsigned int id1=(i+1)*(TesselationDepth+1)+j;
            unsigned int id2=(i+1)*(TesselationDepth+1)+j+1;
            unsigned int id3=i*(TesselationDepth+1)+j+1;
            gManualTriangleMesh->AddFace(id0,id2,id1);
            gManualTriangleMesh->AddFace(id0,id3,id2);
        }
    }
    gManualTriangleMesh->Build();
    gManualTriangleMesh->mMaterialAmbient[0]=0.2f;
    gManualTriangleMesh->mMaterialAmbient[1]=0.2f;
    gManualTriangleMesh->mMaterialAmbient[2]=0.6f;
    gManualTriangleMesh->mMaterialDiffuse[0]=0.2f;
    gManualTriangleMesh->mMaterialDiffuse[1]=0.2f;
    gManualTriangleMesh->mMaterialDiffuse[2]=0.6f;
    gManualTriangleMesh->mMaterialSpecular[0]=0.6f;
    gManualTriangleMesh->mMaterialSpecular[1]=0.6f;
    gManualTriangleMesh->mMaterialSpecular[2]=0.6f;
    gManualTriangleMesh->mShininess=8.0f;
}
//
// Initialize the application, loading all of the settings that
// we will be accessing later in our fragment shaders.
//
void Setup()
{
    // Set up lighting variables in OpenGL
    // Once we do this, we will be able to access them as built-in
    // attributes in the shader (see examples of this in normalmap.frag)
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_SPECULAR,  specularLight);
    glLightfv(GL_LIGHT0, GL_AMBIENT,   ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,   diffuseLight);

    surfaceNormImg = new STImage(normalMap);
    surfaceNormTex = new STTexture(surfaceNormImg);

    surfaceDisplaceImg = new STImage(displacementMap);
    surfaceDisplaceTex = new STTexture(surfaceDisplaceImg);

    shader = new STShaderProgram();
    shader->LoadVertexShader(vertexShader);
    shader->LoadFragmentShader(fragmentShader);

    resetCamera();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);



    STTriangleMesh::LoadObj(gTriangleMeshes,meshOBJ);
    gMassCenter=STTriangleMesh::GetMassCenter(gTriangleMeshes);
    std::cout<<"Mass Center: "<<gMassCenter<<std::endl;
    gBoundingBox=STTriangleMesh::GetBoundingBox(gTriangleMeshes);
    std::cout<<"Bounding Box: "<<gBoundingBox.first<<" - "<<gBoundingBox.second<<std::endl;




    CreateYourOwnMesh();
}

void CleanUp()
{
    for(unsigned int id=0;id<gTriangleMeshes.size();id++)delete gTriangleMeshes[id];
    if(gManualTriangleMesh!=0)
        delete gManualTriangleMesh;
}

/**
 * Camera adjustment methods
 */
void RotateCamera(float delta_x, float delta_y)
{
    float yaw_rate=1.f;
    float pitch_rate=1.f;

    mPosition -= mLookAt;
    STMatrix4 m;
    m.EncodeR(-1*delta_x*yaw_rate, mUp);
    mPosition = m * mPosition;
    m.EncodeR(-1*delta_y*pitch_rate, mRight);
    mPosition = m * mPosition;

    mPosition += mLookAt;
}

void ZoomCamera(float delta_y)
{
    STVector3 direction = mLookAt - mPosition;
    float magnitude = direction.Length();
    direction.Normalize();
    float zoom_rate = 0.f*magnitude < 0.5f ? .1f*magnitude : .5f;
    if(delta_y * zoom_rate + magnitude > 0)
    {
        mPosition += (delta_y * zoom_rate) * direction;
    }
}

void StrafeCamera(float delta_x, float delta_y)
{
    float strafe_rate = 0.05f;

    mPosition -= strafe_rate * delta_x * mRight;
    mLookAt   -= strafe_rate * delta_x * mRight;
    mPosition += strafe_rate * delta_y * mUp;
    mLookAt   += strafe_rate * delta_y * mUp;
}

//
// Display the output image from our vertex and fragment shaders
//
void DisplayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SetUpAndRight();

    gluLookAt(mPosition.x,mPosition.y,mPosition.z,
              mLookAt.x,mLookAt.y,mLookAt.z,
              mUp.x,mUp.y,mUp.z);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Texture 0: surface normal map
    glActiveTexture(GL_TEXTURE0);
    surfaceNormTex->Bind();

    // Texture 1: surface normal map
    glActiveTexture(GL_TEXTURE1);
    surfaceDisplaceTex->Bind();

    // Bind the textures we've loaded into openGl to
    // the variable names we specify in the fragment
    // shader.
    shader->SetTexture("normalTex", 0);
    shader->SetTexture("displacementTex", 1);
	shader->SetTexture("colorTex", 2);

    // Invoke the shader.  Now OpenGL will call our
    // shader programs on anything we draw.
    shader->Bind();

    if(mesh)
    {
        shader->SetUniform("normalMapping", -1.0);
        shader->SetUniform("displacementMapping", -1.0);
		shader->SetUniform("colorMapping", 1.0);

        glPushMatrix();
        // Pay attention to scale
        STVector3 size_vector=gBoundingBox.second-gBoundingBox.first;
        float maxSize=(std::max)((std::max)(size_vector.x,size_vector.y),size_vector.z);
		glScalef(3.0f/maxSize,3.0f/maxSize,3.0f/maxSize);
		glTranslatef(-gMassCenter.x,-gMassCenter.y,-gMassCenter.z);
        for(unsigned int id=0;id<gTriangleMeshes.size();id++) {
			gTriangleMeshes[id]->Draw(smooth);
		}
        glPopMatrix();
    }
    else
    {
        if(normalMapping){
            shader->SetUniform("displacementMapping", -1.0);
            shader->SetUniform("normalMapping", 1.0);
			shader->SetUniform("colorMapping", -1.0);
        }
        else{
            shader->SetUniform("displacementMapping", 1.0);
            shader->SetUniform("normalMapping", -1.0);
			shader->SetUniform("colorMapping", -1.0);
            shader->SetUniform("TesselationDepth", TesselationDepth);
        }
        if(gManualTriangleMesh)
            gManualTriangleMesh->Draw(smooth);
        if(gTriangleMeshes.size()) {
            for(unsigned int id=0;id<gTriangleMeshes.size();id++) {
                gTriangleMeshes[id]->Draw(smooth);
            }
        }
    }

    shader->UnBind();

    glActiveTexture(GL_TEXTURE0);
    surfaceNormTex->UnBind();

    glActiveTexture(GL_TEXTURE1);
    surfaceDisplaceTex->UnBind();

    glutSwapBuffers();
}

//
// Reshape the window and record the size so
// that we can use it for screenshots.
//
void ReshapeCallback(int w, int h)
{
	gWindowSizeX = w;
    gWindowSizeY = h;

    glViewport(0, 0, gWindowSizeX, gWindowSizeY);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	// Set up a perspective projection
    float aspectRatio = (float) gWindowSizeX / (float) gWindowSizeY;
	gluPerspective(30.0f, aspectRatio, .1f, 10000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void SpecialKeyCallback(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_LEFT:
            StrafeCamera(10,0);
            break;
        case GLUT_KEY_RIGHT:
            StrafeCamera(-10,0);
            break;
        case GLUT_KEY_DOWN:
            StrafeCamera(0,-10);
            break;
        case GLUT_KEY_UP:
            StrafeCamera(0,10);
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

/**
 * Mouse event handler
 */
void MouseCallback(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON
        || button == GLUT_RIGHT_BUTTON
        || button == GLUT_MIDDLE_BUTTON)
    {
        gMouseButton = button;
    } else
    {
        gMouseButton = -1;
    }

    if (state == GLUT_UP)
    {
        gPreviousMouseX = -1;
        gPreviousMouseY = -1;
    }
}

/**
 * Mouse active motion callback (when button is pressed)
 */
void MouseMotionCallback(int x, int y)
{
    if (gPreviousMouseX >= 0 && gPreviousMouseY >= 0)
    {
        //compute delta
        float deltaX = x-gPreviousMouseX;
        float deltaY = y-gPreviousMouseY;
        gPreviousMouseX = x;
        gPreviousMouseY = y;

        //orbit, strafe, or zoom
        if (gMouseButton == GLUT_LEFT_BUTTON)
        {
            RotateCamera(deltaX, deltaY);
        }
        else if (gMouseButton == GLUT_MIDDLE_BUTTON)
        {
            StrafeCamera(deltaX, deltaY);
        }
        else if (gMouseButton == GLUT_RIGHT_BUTTON)
        {
            ZoomCamera(deltaY);
        }

    } else
    {
        gPreviousMouseX = x;
        gPreviousMouseY = y;
    }

}


//-------------------------------------------------
// Inits the TriangleIndices
//-------------------------------------------------
TriangleIndices makeTIndices(int a, int b, int c)
{
    TriangleIndices T;
    T.i1 = a;
    T.i2 = b;
    T.i3 = c;
    return(T);
}

//-----------------------------------------------
// offset each point to the sphere surface
//-----------------------------------------------
int offset(STVector3 p, std::vector<STVector3> *vertices,double radius)
{

    double length = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);

    vertices->push_back(STVector3(p.x*radius/length, p.y*radius/length, p.z*radius/length));

    std::cout<<"    OFFSET!\n";
    return(globalCount + 1);
}



//-----------------------------------------------------------------------
// TO DO:Returns index of smallest point
// Find the middle point at each edge, remove duplicates
// call the offset() function to offset the point from the icocahedron to the
// sphere surface
//-----------------------------------------------------------------------
int midPoint(int p1, int p2, std::multimap<long, int> *midPointIndices, std::vector<STVector3> *vertices)
{
        int larger = p1 > p2 ? p1 : p2;
        std::cout<<"    larger = " << larger << "\n";
        int smaller = p1 < p2 ? p1 : p2;
        std::cout<<"    Smaller = " << smaller << "\n";
        long key = (smaller << 16) + larger;
        std::cout<<"    Key generated, key = " << key <<"\n";
        int index = 0;

        if (midPointIndices->count(key) == 0){
          std::cout<<"    Midpoint not found; beginning calculation\n";
          STVector3 point1 = vertices->at(larger);
          STVector3 point2 = vertices->at(smaller);
          STVector3 mid = STVector3((point1.x - point2.x)/2, (point1.y - point2.y)/2, (point1.z - point2.z)/2);

          std::cout<<"    Offset starts here\n";
          offset(mid, vertices, 1);
          index = vertices->size()-1;
          midPointIndices->insert(std::pair<long, int>(key, index));
          std::cout<<"    Midpoint added, index = " << index << "\n";
        }
        else{
          index = midPointIndices->find(key)->second;
          std::cout<<"    Midpoint found, index = " << index << "\n";
        }
        return(index);
}


//----------------------------------------------------------
// TO DO: Using the CreateYourOwnMesh as an example
// Store your mesh data in the triangleMesh
//-----------------------------------------------------------
void createMySphereMesh(STTriangleMesh  *tmesh, TriangleIndices face, std::vector<STVector3> *vertices)
{
  STVector3 temp;
  //std::cout<<"  Temp vertex created.\n";
  for (int i = 0; tmesh->mVertices.size() != vertices->size(); i++){
    // std::cout<<"  Number of vertices in tmesh: " << tmesh->mVertices.size() << "\n";
    // std::cout<<"  Number of vertices in total: " << vertices->size() << "\n";
    temp = vertices->at(i);
    // std::cout<<"  Temp assigned\n";
    tmesh->AddVertex(temp.x, temp.y, temp.z);
    // std::cout<<"  Vertex added to mesh\n";
  }
  tmesh->AddFace(face.i1, face.i2, face.i3);
  // std::cout<<"  Face added to mesh\n";


  tmesh->Build();
  tmesh->mMaterialAmbient[0]=0.2f;
  tmesh->mMaterialAmbient[1]=0.2f;
  tmesh->mMaterialAmbient[2]=0.6f;
  tmesh->mMaterialDiffuse[0]=0.2f;
  tmesh->mMaterialDiffuse[1]=0.2f;
  tmesh->mMaterialDiffuse[2]=0.6f;
  tmesh->mMaterialSpecular[0]=0.6f;
  tmesh->mMaterialSpecular[1]=0.6f;
  tmesh->mMaterialSpecular[2]=0.6f;
  tmesh->mShininess=8.0f;
}



//---------------------------------------------------------
// Spliting them into 4 smaller triangles
//----------------------------------------------------------
void subDivideTriangles(int level, std::vector<TriangleIndices> *facesIn,  std::vector<TriangleIndices> *facesOut, std::vector<STVector3> *vertices)
{
    std::multimap<long, int> midPointIndices;
    int nFaces = facesIn->size();
    std::cout<<"  Midpoint map, face counter initialized\n";

    std::cout<<"  Beginning midpoint finding\n";
    for (int i = 0; i < level; i++) {

          for(int j = 0; j < nFaces; ++j) {

              int a = midPoint((*facesIn)[j].i1, (*facesIn)[j].i2, &midPointIndices, vertices);
              int b = midPoint((*facesIn)[j].i2, (*facesIn)[j].i3, &midPointIndices, vertices);
              int c = midPoint((*facesIn)[j].i3, (*facesIn)[j].i1, &midPointIndices, vertices);

              facesOut->push_back(makeTIndices((*facesIn)[j].i1, a, c));
              facesOut->push_back(makeTIndices((*facesIn)[j].i2, b, a));
              facesOut->push_back(makeTIndices((*facesIn)[j].i3, c, b));
              facesOut->push_back(makeTIndices(a, b, c));
          }
          (*facesIn) = (*facesOut);
          nFaces=facesIn->size();
    }
}


//-------------------------------------------------------
// TO DO: Create faces for the Iscohedron
// Each TriangleIndices stores the counter clocksise vertex indices
// See definition of TriangleIndices
//-------------------------------------------------------
void initFaces(std::vector<TriangleIndices> *faces)
{

  // 5 faces around point 0
  faces->push_back(makeTIndices(0, 11, 5));
  faces->push_back(makeTIndices(0, 5, 1));
  faces->push_back(makeTIndices(0, 1, 7));
  faces->push_back(makeTIndices(0, 7, 10));
  faces->push_back(makeTIndices(0, 10, 11));

  // 5 adjacent faces
  faces->push_back(makeTIndices(1, 5, 9));
  faces->push_back(makeTIndices(5, 11, 4));
  faces->push_back(makeTIndices(11, 10, 2));
  faces->push_back(makeTIndices(10, 7, 6));
  faces->push_back(makeTIndices(7, 1, 8));

  // 5 faces around point 3
  faces->push_back(makeTIndices(3, 9, 4));
  faces->push_back(makeTIndices(3, 4, 2));
  faces->push_back(makeTIndices(3, 2, 6));
  faces->push_back(makeTIndices(3, 6, 8));
  faces->push_back(makeTIndices(3, 8, 9));

  // 5 adjacent faces
  faces->push_back(makeTIndices(4, 9, 5));
  faces->push_back(makeTIndices(2, 4, 11));
  faces->push_back(makeTIndices(6, 2, 10));
  faces->push_back(makeTIndices(8, 6, 7));
  faces->push_back(makeTIndices(9, 8, 1));

  // faces->push_back(makeTIndices(10, 0, 7));
  // faces->push_back(makeTIndices(11, 0, 10));
  // faces->push_back(makeTIndices(5, 0, 11));
  // faces->push_back(makeTIndices(1, 0, 5));
  // faces->push_back(makeTIndices(7, 0, 1));
  // faces->push_back(makeTIndices(2, 11, 10));
  // faces->push_back(makeTIndices(2, 10, 6));
  // faces->push_back(makeTIndices(2, 6, 3));
  // faces->push_back(makeTIndices(2, 3, 4));
  // faces->push_back(makeTIndices(2, 4, 11));
  // faces->push_back(makeTIndices(9, 1, 5));
  // faces->push_back(makeTIndices(9, 5, 4));
  // faces->push_back(makeTIndices(9, 4, 3));
  // faces->push_back(makeTIndices(9, 3, 8));
  // faces->push_back(makeTIndices(9, 8, 1));
  // faces->push_back(makeTIndices(5, 11, 4));
  // faces->push_back(makeTIndices(1, 8, 7));
  // faces->push_back(makeTIndices(6, 7, 8));
  // faces->push_back(makeTIndices(6, 10, 7));
  // faces->push_back(makeTIndices(6, 8, 3));
}


// Initialize 12 verticies for an iscohedron
// centered at zero. See icosphere_visual.pdf in the docs/folder
void initVertices(std::vector<STVector3> *vertices)
{

    float pos = (1.0 + sqrtf(5.0))/2.0;

    vertices->push_back(STVector3(-1,  pos,  0));
    vertices->push_back(STVector3(1,   pos,  0));
    vertices->push_back(STVector3(-1, -pos,  0));
    vertices->push_back(STVector3(1,  -pos,  0));

    vertices->push_back(STVector3(0, -1,  pos));
    vertices->push_back(STVector3(0,  1,  pos));
    vertices->push_back(STVector3(0, -1, -pos));
    vertices->push_back(STVector3(0,  1, -pos));

    vertices->push_back(STVector3(pos,   0, -1));
    vertices->push_back(STVector3(pos,   0,  1));
    vertices->push_back(STVector3(-pos,  0, -1));
    vertices->push_back(STVector3(-pos,  0,  1));
}



//----------------------------------------------------------------
// TO DO: Complete this function to create your sphere
// The sphere is a unit iscosphere centered at the origin (0,0,0).
// First add a key press event in KeyCallback that will call this function
//----------------------------------------------------------------
void createSphere(void)
{
    globalCount = 0; //for subdivisions // TO DO: Optional remove this as a global var

    // triangle mesh object
    std::vector<STTriangleMesh *> gTriangleMeshes_sphere;
    std::cout<<"Mesh initialized\n";

    // vertices
    std::vector<STVector3> vertices;
    std::cout<<"Vertices initialized\n";

    // Creates the initial verticies
    initVertices(&vertices);
    std::cout<<"Vertices assigned\n";

    //-----------------------------------------------------
    // TO DO: create the faces of the intIcosahedron
    // Place your code in the function initFaces()
    //-----------------------------------------------------
    std::vector<TriangleIndices> faces;
    std::cout<<"Faces initialized\n";
    initFaces(&faces);
    std::cout<<"Faces assigned\n";


    //---------------------------------------------------------------
    // TO DO: Recursively split each triangle into four triangles
    // See images in docs/icosahedron/
    // Determine the levels for the recursion by changing the value in levels
    //----------------------------------------------------------------
    std::vector<TriangleIndices> newfaces;
    int levels = 3;
    std::cout<<"Beginning subdivision\n";
    subDivideTriangles(levels, &faces, &newfaces, &vertices);
    std::cout<<"Subdivision Complete\n";

    //-----------------------------------------------------------------
    // TO DO: Once faces are generated, add each triangle to the
    // call createMySphereMesh here
    // mesh. Place your code in createMySphereMesh()
    //-----------------------------------------------------------------

    std::cout<<"Beginning createMySphereMesh\n";
    STTriangleMesh *temp = new STTriangleMesh();
    for (std::vector<TriangleIndices>::iterator it = newfaces.begin(); it != newfaces.end(); ++it){
      createMySphereMesh(temp, *it, &vertices);
      gTriangleMeshes_sphere.push_back(temp);
    }
    std::cout<<"CreateMySphereMesh complete\n";

    gTriangleMeshes_sphere.back()->Build();
    gTriangleMeshes_sphere.back()->mMaterialAmbient[0]=0.2f;
    gTriangleMeshes_sphere.back()->mMaterialAmbient[1]=0.2f;
    gTriangleMeshes_sphere.back()->mMaterialAmbient[2]=0.6f;
    gTriangleMeshes_sphere.back()->mMaterialDiffuse[0]=0.2f;
    gTriangleMeshes_sphere.back()->mMaterialDiffuse[1]=0.2f;
    gTriangleMeshes_sphere.back()->mMaterialDiffuse[2]=0.6f;
    gTriangleMeshes_sphere.back()->mMaterialSpecular[0]=0.6f;
    gTriangleMeshes_sphere.back()->mMaterialSpecular[1]=0.6f;
    gTriangleMeshes_sphere.back()->mMaterialSpecular[2]=0.6f;
    gTriangleMeshes_sphere.back()->mShininess=8.0f;

    // save the result sphere
    std::cout<<"Mesh size: " << gTriangleMeshes_sphere.size() << "\n";
    //for(unsigned int id=0;id<gTriangleMeshes_sphere.size(); id++)
        //gTriangleMeshes_sphere[id]->Write("../../data/meshes/mysphere.obj");
        gTriangleMeshes_sphere.back()->Write("../../data/meshes/mysphere.obj");


        std::cout<<"Sphere saved, globalCount = " << globalCount << "\n";
}



void usage()
{
    // TO DO
	printf("usage: proj1_mesh vertShader fragShader objMeshFile normalMappingTexture displacementMappingTexture\n");
}

void KeyCallback(unsigned char key, int x, int y)
{
    // TO DO: Any new key press events must be added to this function

    switch(key) {
    case 's': {
            //
            // Take a screenshot, and save as screenshot.jpg
            //
            STImage* screenshot = new STImage(gWindowSizeX, gWindowSizeY);
            screenshot->Read(0,0);
            screenshot->Save("../../data/images/screenshot.jpg");
            delete screenshot;
        }
        break;
    case 'r':
        resetCamera();
        break;
    case 'u':
        resetUp();
        break;
    case 'm': // switch between the mesh you create and the mesh from file
        mesh = !mesh;
        break;
    case 'n': // switch between normalMapping and displacementMapping
        normalMapping = !normalMapping;
        break;
    // TO:DO do loop subdivision, uncomment this code
    // modify it so that it works with your sphere
    // simply make sure you are calling the functions
    // on the correct mesh.
    //---------------------------------
    //case 'l':
    //    if(mesh){
    //        gTriangleMesh->LoopSubdivide();
	//   if(proxyType) gTriangleMesh->CalculateTextureCoordinatesViaSphericalProxy();
	//		else gTriangleMesh->CalculateTextureCoordinatesViaCylindricalProxy(-
    case 'f': // switch between smooth shading and flat shading
        smooth = !smooth;
        break;
    case 'w':
        for(unsigned int id=0;id<gTriangleMeshes.size();id++)
            gTriangleMeshes[id]->Write("output.obj");
        break;
    case 'a':
        for(unsigned int id=0;id<gTriangleMeshes.size();id++)
            gTriangleMeshes[id]->mDrawAxis=!gTriangleMeshes[id]->mDrawAxis;
        if(gManualTriangleMesh!=0)
            gManualTriangleMesh->mDrawAxis=!gManualTriangleMesh->mDrawAxis;
        break;
    case '1':
        createSphere();
        break;
    case '2':
        CreateYourOwnMesh();
        break;
	case 'q':
		exit(0);
    default:
        break;
    }

    glutPostRedisplay();
}

//-------------------------------------------------
// main program loop
//-------------------------------------------------
int main(int argc, char** argv)
{
    //---------------------------------------------------------------------------
    // TO DO: Change this file name to change the .obj model that is loaded
    // Optional: read in the file name from the command line > proj1_mesh myfile.obj
    //--------------------------------------------------------------------------
    meshOBJ        = std::string("../../data/meshes/");
    meshOBJ        += argv[1];


    vertexShader   = std::string("kernels/default.vert");
	fragmentShader = std::string("kernels/phong.frag");
	normalMap      = std::string("../../data/images/normalmap.png");
	displacementMap= std::string("../../data/images/displacementmap.jpeg");

    //
    // Initialize GLUT.
    //
    glutInit(&argc, argv);
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(20, 20);
    glutInitWindowSize(640, 480);
    glutCreateWindow("proj1_mesh");

    //
    // Initialize GLEW.
    //
#ifndef __APPLE__
    glewInit();
    if(!GLEW_VERSION_2_0) {
        printf("Your graphics card or graphics driver does\n"
			   "\tnot support OpenGL 2.0, trying ARB extensions\n");

        if(!GLEW_ARB_vertex_shader || !GLEW_ARB_fragment_shader) {
            printf("ARB extensions don't work either.\n");
            printf("\tYou can try updating your graphics drivers.\n"
				   "\tIf that does not work, you will have to find\n");
            printf("\ta machine with a newer graphics card.\n");
            exit(1);
        }
    }
#endif

    // Be sure to initialize GLUT (and GLEW for this assignment) before
    // initializing your application.

    Setup();

    glutDisplayFunc(DisplayCallback);
    glutReshapeFunc(ReshapeCallback);
    glutSpecialFunc(SpecialKeyCallback);
    glutKeyboardFunc(KeyCallback);
    glutMouseFunc(MouseCallback);
    glutMotionFunc(MouseMotionCallback);
    glutIdleFunc(DisplayCallback);

    glutMainLoop();

    // Cleanup code should be called here.
    CleanUp();

    return 0;
}
