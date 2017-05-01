// Source file for the scene converter program



// Include files 

#include "R3Graphics/R3Graphics.h"


// Program arguments

static const char *input_scene_name = NULL;
static const char *output_grid_name = NULL;
static double grid_spacing = 0.005;
static double grid_boundary_radius = 0.05;
static int grid_max_resolution = 1000;
int label_num = 0;
char obj_name[3000][100];
int label[3000];




////////////////////////////////////////////////////////////////////////
// I/O STUFF
////////////////////////////////////////////////////////////////////////

static R3Scene *
ReadScene(const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Allocate scene
  R3Scene *scene = new R3Scene();
  if (!scene) {
    fprintf(stderr, "Unable to allocate scene for %s\n", filename);
    return NULL;
  }

  // Read scene from file
  if (!scene->ReadFile(filename)) {
    delete scene;
    return NULL;
  }

  // Return scene
  return scene;
}

////////////////////////////////////////////////////////////////////////
// GRID CREATION FUNCTIONS
////////////////////////////////////////////////////////////////////////

static void
RasterizeTriangles(R3Grid *grid, R3Scene *scene, R3SceneNode *node, const R3Affine& parent_transformation)
{
  // Update transformation
  R3Affine transformation = R3identity_affine;
  transformation.Transform(parent_transformation);
  transformation.Transform(node->Transformation());
  // Rasterize triangles
  if(node->NReferences()>0)
    for (int l = 0; l < node->NReferences(); l++) 
    {
      R3SceneNode *node_new = node->Reference(l)->ReferencedScene()->Node(0);
      int label_mark1 = 0;
      for (int i = 0; i < label_num; i++)
        if (strcmp(node->Reference(l)->ReferencedScene()->Name(),obj_name[i]) == 0)
          label_mark1 = label[i];

      for (int i = 0; i < node_new->NElements(); i++) 
      {
        R3SceneElement *element = node_new->Element(i);
        const RNRgb& rn = element->Material()->Brdf()->Ambient();
        RNBoolean f = element->Material()->IsTextured();
        R2Image *img = new R2Image();
        if(f)
        {
          *img = *element->Material()->Texture()->Image();
        }
        double r =(double)rn.R();
	    double g =(double)rn.G();
	    double b =(double)rn.B();
        RNRgb rgb = RNRgb(r, g, b);
        for (int j = 0; j < element->NShapes(); j++) 
        {
          R3Shape *shape = element->Shape(j);
          if (shape->ClassID() == R3TriangleArray::CLASS_ID()) 
          {
            R3TriangleArray *triangles = (R3TriangleArray *) shape;
            for (int k = 0; k < triangles->NTriangles(); k++) 
            {
              R3Triangle *triangle = triangles->Triangle(k);
              R3TriangleVertex *v0 = triangle->V0();
              R3TriangleVertex *v1 = triangle->V1();
              R3TriangleVertex *v2 = triangle->V2();
              R3Point p0 = v0->Position();
              R3Point p1 = v1->Position();
              R3Point p2 = v2->Position();
              p0.Transform(transformation);
              p1.Transform(transformation);
              p2.Transform(transformation);
              R2Point t0 = R2Point();
              R2Point t1 = R2Point();
              R2Point t2 = R2Point();
              if(f)
              {
                t0 = v0->TextureCoords();
                t1 = v1->TextureCoords();
                t2 = v2->TextureCoords();
              }
              grid->RasterizeWorldTriangleMat(p0, p1, p2, t0, t1, t2, 1.0, label_mark1,rgb,img);
            }
          }
        }
        for (int i = 0; i < node_new->NChildren(); i++) 
        {
          R3SceneNode *child = node_new->Child(i);
          RasterizeTriangles(grid, scene, child, transformation);
        }
      }
    }
  else
  {
    int label_mark2 = 0;
    for (int i = 0; i < label_num; i++)
      if (strcmp(node->Name(),obj_name[i]) == 0)
        label_mark2 = label[i];

    for (int i = 0; i < node->NElements(); i++) 
    {
      R3SceneElement *element = node->Element(i);
      const RNRgb& rn = element->Material()->Brdf()->Ambient();
      RNBoolean f = element->Material()->IsTextured();
      R2Image *img = new R2Image();
      if(f)
      {
        *img = *element->Material()->Texture()->Image();
      }
      double r =(double)rn.R();
	  double g =(double)rn.G();
	  double b =(double)rn.B();
      RNRgb rgb = RNRgb(r, g, b);
      for (int j = 0; j < element->NShapes(); j++) 
      {
        R3Shape *shape = element->Shape(j);
        if (shape->ClassID() == R3TriangleArray::CLASS_ID()) 
        {
          R3TriangleArray *triangles = (R3TriangleArray *) shape;
          for (int k = 0; k < triangles->NTriangles(); k++) 
          {
            R3Triangle *triangle = triangles->Triangle(k);
            R3TriangleVertex *v0 = triangle->V0();
            R3TriangleVertex *v1 = triangle->V1();
            R3TriangleVertex *v2 = triangle->V2();
            R3Point p0 = v0->Position();
            R3Point p1 = v1->Position();
            R3Point p2 = v2->Position();
            p0.Transform(transformation);
            p1.Transform(transformation);
            p2.Transform(transformation);
            R2Point t0 = R2Point();
            R2Point t1 = R2Point();
            R2Point t2 = R2Point();
            if(f)
            {
              t0 = v0->TextureCoords();
              t1 = v1->TextureCoords();
              t2 = v2->TextureCoords();
            }
            grid->RasterizeWorldTriangleMat(p0, p1, p2, t0, t1, t2, 1.0,label_mark2,rgb,img);
          }
        }
      }
    }
    // Rasterize children
    for (int i = 0; i < node->NChildren(); i++) 
    {
      R3SceneNode *child = node->Child(i);
      RasterizeTriangles(grid, scene, child, transformation);
    }
  }
}


static R3Grid *
CreateGrid(R3Scene *scene,int x,int y,int z)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Get bounding box
  R3Box bbox = scene->BBox();
  if (grid_boundary_radius > 0) {
    bbox[0] -= R3Vector(grid_boundary_radius, grid_boundary_radius, grid_boundary_radius);
    bbox[1] += R3Vector(grid_boundary_radius, grid_boundary_radius, grid_boundary_radius);
  }

  // Compute grid spacing
  RNLength diameter = bbox.LongestAxisLength();
  RNLength min_grid_spacing = (grid_max_resolution > 0) ? diameter / grid_max_resolution : RN_EPSILON;
  if (grid_spacing == 0) grid_spacing = diameter / 256;
  if (grid_spacing < min_grid_spacing) grid_spacing = min_grid_spacing;

  // Compute grid resolution
  int xres = (int) (bbox.XLength() / grid_spacing + 0.5); if (xres == 0) xres = 1;
  int yres = (int) (bbox.YLength() / grid_spacing + 0.5); if (yres == 0) yres = 1;
  int zres = (int) (bbox.ZLength() / grid_spacing + 0.5); if (zres == 0) zres = 1;
  if (x>0) xres = x;
  if (y>0) yres = y;
  if (z>0) zres = z;
  // Allocate grid
  R3Grid *grid = new R3Grid(xres, yres, zres, bbox);
  if (!grid) {
    fprintf(stderr, "Unable to allocate grid\n");
    return NULL;
  }

  // Rasterize scene into grid
  RasterizeTriangles(grid, scene, scene->Root(), R3identity_affine);

  // Threshold grid (to compensate for possible double rasterization)
  grid->Threshold(0.5, 0.0, 1.0);
  // Return grid

  return grid;
}


////////////////////////////////////////////////////////////////////////
// PROGRAM ARGUMENT PARSING
////////////////////////////////////////////////////////////////////////

void GetLabel(const char* name)
{
  FILE *fp = fopen(name, "r");
  char buffer[100];
  label_num = 0;
  while (fgets(buffer, 99, fp))
  {
    sscanf(buffer,"%s %d",obj_name[label_num],&label[label_num]);
    label_num++;
  }
  fclose(fp);
}


////////////////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////////////////

double* ans;
double* res;

extern "C" double * get_data(const char * s,int * b,int x,int y,int z,const char * label_file)
{
  input_scene_name=s;
  output_grid_name="/home/papa/sung/object/41/41.txt";

  if(label_num==0)
    GetLabel(label_file);
  // Read scene
  R3Scene *scene = ReadScene(input_scene_name);
  if (!scene) exit(-1);

  // Create grid
  R3Grid *grid = CreateGrid(scene,x,y,z);
  if (!grid) exit(-1);;
  int grid_resolution2 = grid->Resolution(2);
  int grid_resolution1 = grid->Resolution(1);
  int grid_resolution0 = grid->Resolution(0);
  int grid_size = grid->NEntries();
  if (grid_size>50000000) grid_size =(int) grid_size*0.3;
  ans = (double*)malloc(sizeof(double)*grid_size*7);
  double * ans_p = ans;
  int num = 0;
  for (int k = 0; k < grid_resolution2-1; k++) 
  {
    for (int j = 0; j < grid_resolution1-1; j++) 
    {
	  for (int i = 0; i < grid_resolution0; i++) 
      {
        float f = (float)grid->GridValue(i,j,k);
	    if (f<=0) continue;
        RNRgb RGB = grid->RgbValue(i,j,k);
        *ans_p = (double)i;
        ans_p++;
        *ans_p = (double)j;
        ans_p++;
        *ans_p = (double)k;
        ans_p++;
        *ans_p = (double)RGB.R();
        ans_p++;
        *ans_p = (double)RGB.G();
        ans_p++;
        *ans_p = (double)RGB.B();
        ans_p++;
        *ans_p = (double)grid->LabelValue(i,j,k);
        ans_p++;
        num++;
	  }
	}
  }

  res = (double*)malloc(sizeof(double)*num*7);
  for(int i=0;i<num*7;i++)
    res[i] = ans[i];
  free(ans);
  const char *file_name = strrchr(input_scene_name, '/');
  printf("%s %d\n",++file_name,grid->NEntries());

  *b=num;
  return res;
}

