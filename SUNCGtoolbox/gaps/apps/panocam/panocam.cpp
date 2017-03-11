// Source file for the scene camera creation program



////////////////////////////////////////////////////////////////////////
// Include files 
////////////////////////////////////////////////////////////////////////

#include "R3Graphics/R3Graphics.h"
#ifdef USE_MESA
#  include "GL/osmesa.h"
#else
#  include "fglut/fglut.h" 
#  define USE_GLUT
#endif


////////////////////////////////////////////////////////////////////////
// Program variables
////////////////////////////////////////////////////////////////////////

// Filename program variables

static char *input_scene_filename = NULL;
static char *output_cameras_filename = NULL;
static char *input_categories_filename = NULL;
static char *output_camera_extrinsics_filename = NULL;
static char *output_camera_intrinsics_filename = NULL;
static char *output_camera_names_filename = NULL;


// Camera parameter variables

static int width = 160;
static int height = 256;
static double focal_length = 80;
static double eye_height = 1.5;
static double downward_tilt_angle = 0;
static double min_distance_between_panorama = 1.5;
static double min_distance_from_obstacle = 0.5;
static int num_directions_per_panorama = 4;

// Camera scoring variables

static int scene_scoring_method = 0;
static int object_scoring_method = 0;
static double min_visible_objects = 2;
static double min_visible_fraction = 0.01;


// Rendering variables

static int glut = 1;
static int mesa = 0;

// Informational program variables

static int print_verbose = 0;



////////////////////////////////////////////////////////////////////////
// Internal type definitions
////////////////////////////////////////////////////////////////////////



struct Camera : public R3Camera {
public:
  Camera(void) : R3Camera(), name(NULL) {};
  Camera(const Camera& camera) : R3Camera(camera), name((name) ? strdup(name) : NULL) {};
  Camera(const R3Camera& camera, const char *name) : R3Camera(camera), name((name) ? strdup(name) : NULL) {};
  Camera(const R3Point& origin, const R3Vector& towards, const R3Vector& up, RNAngle xfov, RNAngle yfov, RNLength neardist, RNLength fardist)
    : R3Camera(origin, towards, up, xfov, yfov, neardist, fardist), name(NULL) {};
  ~Camera(void) { if (name) free(name); }
  char *name;
};


////////////////////////////////////////////////////////////////////////
// Internal variables
////////////////////////////////////////////////////////////////////////

// State variables

static R3Scene *scene = NULL;
static RNArray<Camera *> cameras;


// Image types

static const int NODE_INDEX_IMAGE = 0;


////////////////////////////////////////////////////////////////////////
// Input/output functions
////////////////////////////////////////////////////////////////////////

static R3Scene *
ReadScene(char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Allocate scene
  scene = new R3Scene();
  if (!scene) {
    fprintf(stderr, "Unable to allocate scene for %s\n", filename);
    return NULL;
  }

  // Read scene from file
  if (!scene->ReadFile(filename)) {
    delete scene;
    return NULL;
  }

  // Remove references and transformations
  scene->RemoveReferences();
  scene->RemoveTransformations();

  // Print statistics
  if (print_verbose) {
    printf("Read scene from %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Nodes = %d\n", scene->NNodes());
    printf("  # Lights = %d\n", scene->NLights());
    printf("  # Materials = %d\n", scene->NMaterials());
    printf("  # Brdfs = %d\n", scene->NBrdfs());
    printf("  # Textures = %d\n", scene->NTextures());
    printf("  # Referenced models = %d\n", scene->NReferencedScenes());
    fflush(stdout);
  }

  // Return scene
  return scene;
}

static int
ReadCategories(const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Read file
  if (!scene->ReadSUNCGModelFile(filename)) return 0;

  // Print statistics
  if (print_verbose) {
    printf("Read categories from %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    fflush(stdout);
  }

  // Return success
  return 1;
} 



static int
WriteCameras(const RNArray<Camera *>& cameras, const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Open file
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Unable to open cameras file %s\n", filename);
    return 0;
  }

  // Write file
  for (int i = 0; i < cameras.NEntries(); i++) {
    Camera *camera = cameras.Kth(i);
    R3Point eye = camera->Origin();
    R3Vector towards = camera->Towards();
    R3Vector up = camera->Up();
    fprintf(fp, "%g %g %g  %g %g %g  %g %g %g  %g %g  %g\n",
      eye.X(), eye.Y(), eye.Z(),
      towards.X(), towards.Y(), towards.Z(),
      up.X(), up.Y(), up.Z(),
      camera->XFOV(), camera->YFOV(),
      camera->Value());
  }

  // Close file
  fclose(fp);

  // Print statistics
  if (print_verbose) {
    printf("Wrote cameras to %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Cameras = %d\n", cameras.NEntries());
    fflush(stdout);
  }

  // Return success
  return 1;
}



static int
WriteCameraExtrinsics(const RNArray<Camera *>& cameras, const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Open file
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Unable to open camera extrinsics file %s\n", filename);
    return 0;
  }

  // Write file
  for (int i = 0; i < cameras.NEntries(); i++) {
    Camera *camera = cameras.Kth(i);
    const R3CoordSystem& cs = camera->CoordSystem();
    R4Matrix matrix = cs.Matrix();
    fprintf(fp, "%g %g %g %g   %g %g %g %g  %g %g %g %g\n",
      matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3], 
      matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3], 
      matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]);
  }

  // Close file
  fclose(fp);

  // Print statistics
  if (print_verbose) {
    printf("Wrote camera extrinsics to %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Cameras = %d\n", cameras.NEntries());
    fflush(stdout);
  }

  // Return success
  return 1;
}



static int
WriteCameraIntrinsics(const RNArray<Camera *>& cameras, const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Open file
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Unable to open camera intrinsics file %s\n", filename);
    return 0;
  }

  // Get center of image
  RNScalar cx = 0.5 * width;
  RNScalar cy = 0.5 * height;

  // Write file
  for (int i = 0; i < cameras.NEntries(); i++) {
    Camera *camera = cameras.Kth(i);
    RNScalar fx = 0.5 * width / atan(camera->XFOV());
    RNScalar fy = 0.5 * height / atan(camera->YFOV());
    fprintf(fp, "%g 0 %g   0 %g %g  0 0 1\n", fx, cx, fy, cy);
  }

  // Close file
  fclose(fp);

  // Print statistics
  if (print_verbose) {
    printf("Wrote camera intrinsics to %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    fflush(stdout);
  }

  // Return success
  return 1;
}



static int
WriteCameraNames(const RNArray<Camera *>& cameras, const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Open file
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Unable to open camera names file %s\n", filename);
    return 0;
  }

  // Write file
  for (int i = 0; i < cameras.NEntries(); i++) {
    Camera *camera = cameras.Kth(i);
    fprintf(fp, "%s\n", (camera->name) ? camera->name : "-");
  }

  // Close file
  fclose(fp);

  // Print statistics
  if (print_verbose) {
    printf("Wrote camera names to %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    fflush(stdout);
  }

  // Return success
  return 1;
}

static int
WriteCameras(void)
{
  // Write cameras 
  if (output_cameras_filename) {
    if (!WriteCameras(cameras, output_cameras_filename)) exit(-1);
  }
  
  // Write camera extrinsics 
  if (output_camera_extrinsics_filename) {
    if (!WriteCameraExtrinsics(cameras, output_camera_extrinsics_filename)) exit(-1);
  }
  
  // Write camera intrinsics 
  if (output_camera_intrinsics_filename) {
    if (!WriteCameraIntrinsics(cameras, output_camera_intrinsics_filename)) exit(-1);
  }

  // Write camera names 
  if (output_camera_names_filename) {
    if (!WriteCameraNames(cameras, output_camera_names_filename)) exit(-1);
  }

  // Return success
  return 1;
}


////////////////////////////////////////////////////////////////////////
// OpenGL image capture functions
////////////////////////////////////////////////////////////////////////

static int
CaptureScalar(R2Grid& scalar_image)
{
  // Capture rgb pixels
  unsigned char *pixels = new unsigned char [ 3 * width * height ];
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

  // Fill scalar image
  unsigned char *pixelp = pixels;
  for (int iy = 0; iy < height; iy++) {
    for (int ix = 0; ix < width; ix++) {
      unsigned int value = 0;
      value |= (*pixelp++ << 16) & 0xFF0000;
      value |= (*pixelp++ <<  8) & 0x00FF00;
      value |= (*pixelp++      ) & 0x0000FF;
      scalar_image.SetGridValue(ix, iy, value);
    }
  }

  // Delete rgb pixels
  delete [] pixels;
  
  // Return success
  return 1;
}


static void 
DrawNodeWithOpenGL(R3Scene *scene, R3SceneNode *node, R3SceneNode *selected_node, int image_type)
{
  // Set color based on node index
  RNFlags draw_flags = R3_DEFAULT_DRAW_FLAGS;
  if (image_type == NODE_INDEX_IMAGE) {
    draw_flags = R3_SURFACES_DRAW_FLAG;
    unsigned int node_index = node->SceneIndex() + 1;
    unsigned char color[4];
    color[0] = (node_index >> 16) & 0xFF;
    color[1] = (node_index >>  8) & 0xFF;
    color[2] = (node_index      ) & 0xFF;
    glColor3ubv(color);
  }
  
  // Draw elements
  if (!selected_node || (selected_node == node)) {
    for (int i = 0; i < node->NElements(); i++) {
      R3SceneElement *element = node->Element(i);
      element->Draw(draw_flags);
    }
  }

  // Draw references 
  if (!selected_node || (selected_node == node)) {
    for (int i = 0; i < node->NReferences(); i++) {
      R3SceneReference *reference = node->Reference(i);
      reference->Draw(draw_flags);
    }
  }

  // Draw children
  for (int i = 0; i < node->NChildren(); i++) {
    R3SceneNode *child = node->Child(i);
    DrawNodeWithOpenGL(scene, child, selected_node, image_type);
  }
}

static void 
RenderImageWithOpenGL(R2Grid& image, const R3Camera& camera, R3Scene *scene, R3SceneNode *root_node, R3SceneNode *selected_node, int image_type)
{
  // Clear window
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Initialize transformation
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Load camera and viewport
  camera.Load();
  glViewport(0, 0, width, height);

  // Initialize graphics modes  
  glEnable(GL_DEPTH_TEST);

  // Draw scene
  R3null_material.Draw();
  DrawNodeWithOpenGL(scene, root_node, selected_node, image_type);
  R3null_material.Draw();

  // Read frame buffer into image
  CaptureScalar(image);

  // Compensate for rendering background black
  image.Substitute(0, R2_GRID_UNKNOWN_VALUE);
  if (image_type == NODE_INDEX_IMAGE) image.Subtract(1.0);
}



////////////////////////////////////////////////////////////////////////
// Raycasting image capture functions
////////////////////////////////////////////////////////////////////////

static void
RenderImageWithRayCasting(R2Grid& image, const R3Camera& camera, R3Scene *scene, R3SceneNode *root_node,  R3SceneNode *selected_node, int image_type)
{
  // Clear image
  image.Clear(R2_GRID_UNKNOWN_VALUE);

  // Setup viewer
  R2Viewport viewport(0, 0, image.XResolution(), image.YResolution());
  R3Viewer viewer(camera, viewport);

  // Render image with ray casting
  for (int iy = 0; iy < image.YResolution(); iy++) {
    for (int ix = 0; ix < image.XResolution(); ix++) {
      R3Ray ray = viewer.WorldRay(ix, iy);
      R3SceneNode *intersection_node = NULL;
      if (root_node->Intersects(ray, &intersection_node)) {
        if (intersection_node) {
          if (!selected_node || (selected_node == intersection_node)) {
            if (image_type == NODE_INDEX_IMAGE) {
              image.SetGridValue(ix, iy, intersection_node->SceneIndex());
            }
          }
        }
      }
    }
  }
}

  

////////////////////////////////////////////////////////////////////////
// Image capture functions
////////////////////////////////////////////////////////////////////////

static void
RenderImage(R2Grid& image, const R3Camera& camera, R3Scene *scene, R3SceneNode *root_node, R3SceneNode *selected_node, int image_type)
{
  // Check rendering method
  if (glut || mesa) RenderImageWithOpenGL(image, camera, scene, root_node, selected_node, image_type);
  else RenderImageWithRayCasting(image, camera, scene, root_node, selected_node, image_type);
}




////////////////////////////////////////////////////////////////////////
// Camera scoring function
////////////////////////////////////////////////////////////////////////

static RNBoolean
IsObject(R3SceneNode *node)
{
  // Check name
  if (!node->Name()) return 0;
  if (strncmp(node->Name(), "Model#", 6)) return 0;

  // Get category identifier
  R3SceneNode *ancestor = node;
  const char *category_identifier = NULL;
  while (!category_identifier && ancestor) {
    category_identifier = node->Info("empty_struct_obj");
    ancestor = ancestor->Parent();
  }

  // Check category identifier
  if (category_identifier) {
    if (strcmp(category_identifier, "2")) return 0;
  }

  // Passed all tests
  return 1;
}

static RNScalar
SceneCoverageScore(const R3Camera& camera, R3Scene *scene, R3SceneNode *room_node = NULL)
{
  // Allocate image for scoring
  R2Grid image(width, height);

  // Compute maximum number of pixels in image
  int max_pixel_count = width * height;
  if (max_pixel_count == 0) return 0;

  // Compute minimum number of pixels per object
  int min_pixel_count_per_object = min_visible_fraction * max_pixel_count;
  if (min_pixel_count_per_object == 0) return 0;

  // Render image
  RenderImage(image, camera, scene, scene->Root(), NULL, NODE_INDEX_IMAGE);

  // Allocate buffer for counting visible pixels of nodes
  int *node_pixel_counts = new int [ scene->NNodes() ];
  for (int i = 0; i < scene->NNodes(); i++) node_pixel_counts[i] = 0;
  
  // Log counts of pixels visible on each node
  for (int i = 0; i < image.NEntries(); i++) {      
    RNScalar value = image.GridValue(i);
    if (value == R2_GRID_UNKNOWN_VALUE) continue;
    int node_index = (int) (value + 0.5);
    if ((node_index < 0) || (node_index >= scene->NNodes())) continue;
    node_pixel_counts[node_index]++;
  }

  // Compute score
  RNScalar sum = 0;
  int node_count = 0;
  for (int i = 0; i < scene->NNodes(); i++) {
    R3SceneNode *node = scene->Node(i);
    if (!IsObject(node)) continue;
    if (room_node && !node->IsDecendent(room_node)) continue;
    if (node_pixel_counts[i] <= min_pixel_count_per_object) continue;
    sum += log(node_pixel_counts[i] / min_pixel_count_per_object);
    node_count++;
  }

  // Compute score (log of product of number of pixels visible in each object)
  RNScalar score = (node_count > min_visible_objects) ? sum : 0;

  // Delete pixel counts
  delete [] node_pixel_counts;

  // Return score
  return score;
}


////////////////////////////////////////////////////////////////////////
// Mask creation functions
////////////////////////////////////////////////////////////////////////

static void
RasterizeIntoZXGrid(R2Grid& grid, R3SceneNode *node, const R3Box& world_bbox)
{
  // Check node name ???
  if (node->Name() && !strncmp(node->Name(), "Ground", 6)) return;
  if (node->Name() && !strncmp(node->Name(), "Ceiling", 7)) return;
  
  // Check bounding box
  R3Box node_bbox = node->BBox();
  if (!R3Intersects(world_bbox, node_bbox)) return;
  
  // Rasterize elements into grid
  for (int j = 0; j < node->NElements(); j++) {
    R3SceneElement *element = node->Element(j);
    for (int k = 0; k < element->NShapes(); k++) {
      R3Shape *shape = element->Shape(k);
      R3Box shape_bbox = shape->BBox();
      if (!R3Intersects(world_bbox, shape_bbox)) continue;
      if (shape->ClassID() == R3TriangleArray::CLASS_ID()) {
        R3TriangleArray *triangles = (R3TriangleArray *) shape;
        for (int m = 0; m < triangles->NTriangles(); m++) {
          R3Triangle *triangle = triangles->Triangle(m);
          R3Box triangle_bbox = triangle->BBox();
          if (!R3Intersects(world_bbox, triangle_bbox)) continue;
          R3TriangleVertex *v0 = triangle->V0();
          R3Point vp0 = v0->Position();
          R2Point p0(vp0.Z(), vp0.X());
          if (!R2Contains(grid.WorldBox(), p0)) continue;
          R3TriangleVertex *v1 = triangle->V1();
          R3Point vp1 = v1->Position();
          R2Point p1(vp1.Z(), vp1.X());
          if (!R2Contains(grid.WorldBox(), p1)) continue;
          R3TriangleVertex *v2 = triangle->V2();
          R3Point vp2 = v2->Position();
          R2Point p2(vp2.Z(), vp2.X());
          if (!R2Contains(grid.WorldBox(), p2)) continue;
          grid.RasterizeWorldTriangle(p0, p1, p2, 1.0);
        }
      }
    }
  }

  // Rasterize children into grid
  for (int j = 0; j < node->NChildren(); j++) {
    R3SceneNode *child = node->Child(j);
    RasterizeIntoZXGrid(grid, child, world_bbox);
  }
}



static int
CreateMask(R2Grid& mask, R3SceneNode *node) 
{
  // Get bounding boxes
  R3Box world_bbox = node->BBox();
  R3Box floor_bbox = world_bbox;
  floor_bbox[RN_HI][RN_Y] = world_bbox[RN_LO][RN_Y] + 0.1;
  R3Box obstacle_bbox = world_bbox;
  obstacle_bbox[RN_LO][RN_Y] = world_bbox[RN_LO][RN_Y] + 0.1;
  obstacle_bbox[RN_HI][RN_Y] = world_bbox[RN_LO][RN_Y] + 2.0;
  R2Box grid_bbox(world_bbox.ZMin(), world_bbox.XMin(), world_bbox.ZMax(), world_bbox.XMax());

  // Create mask
  RNScalar grid_sample_spacing = 0.5 * min_distance_from_obstacle;
  if (grid_sample_spacing == 0) grid_sample_spacing = 0.05;
  int res1 = (int) (grid_bbox.XLength() / grid_sample_spacing);
  int res2 = (int) (grid_bbox.YLength() / grid_sample_spacing);
  if ((res1 < 3) || (res2 < 3)) return 0;
  mask = R2Grid(res1, res2, grid_bbox);

  // Rasterize floor into mask (and then reverse so that 1 is outside and 0 is on floor)
  RasterizeIntoZXGrid(mask, node, floor_bbox);
  mask.Threshold(0.5, 1, 0);

  // Rasterize obstacles into mask
  RasterizeIntoZXGrid(mask, node, obstacle_bbox);

  // Process mask (result has higher values where cameras should be, zero otherwise)
  mask.Dilate(min_distance_from_obstacle / grid_sample_spacing);
  mask.SquaredDistanceTransform();
  
#if 0
  // Debugging
  char buffer[4096];
  sprintf(buffer, "%s_mask.jpg", node->Name());
  mask.WriteFile(buffer);
#endif
  
  // Return success
  return 1;
}



static int
CreatePanoramicCameras(RNArray<Camera *>& cameras, R3Scene *scene, R3SceneNode *node) 
{
  // Get useful variables
  RNScalar y = node->BBox().YMin() + eye_height;
  RNScalar neardist = 0.01 * scene->BBox().DiagonalRadius();
  RNScalar fardist = 100 * scene->BBox().DiagonalRadius();
  RNScalar xfov = atan(0.5 * width / focal_length);
  RNScalar yfov = atan(0.5 * height / focal_length);
  int npanorama = 0;
  R2Grid mask;

  // Compute viewpoint mask (zx)
  if (!CreateMask(mask, node)) return 0;

  // Sample locations from mask
  while (TRUE) {
    // Find grid position with maximum value
    RNScalar mask_value = 0;
    int mask_ix = -1, mask_iy = -1;
    for (int ix = 0; ix < mask.XResolution(); ix++) {
      for (int iy = 0; iy < mask.YResolution(); iy++) {
        RNScalar value = mask.GridValue(ix, iy);
        if (value > mask_value) {
          mask_ix = ix; mask_iy = iy;
          mask_value = value;
        }
      }
    }

    // Check if found a suitable grid position
    if (mask_value <= 0) break;

    // Compute world viewpoint position
    R2Point zx = mask.WorldPosition(mask_ix, mask_iy);
    RNScalar x = zx[1] + RNRandomScalar() * mask.GridToWorldScaleFactor();
    RNScalar z = zx[0] + RNRandomScalar() * mask.GridToWorldScaleFactor();
    R3Point viewpoint(x, y, z);

    

    // Create set of panoramic cameras at viewpoint
    RNAngle base_angle = 0; //RN_TWO_PI * RNRandomScalar();
    float panoscore = 0;
    RNArray<Camera *> cameras_pano;
    for (int i = 0; i < num_directions_per_panorama; i++) {
      // Compute direction
      RNScalar angle = base_angle + i*RN_TWO_PI/num_directions_per_panorama;
      R2Vector direction = R2posx_vector;
      direction.Rotate(angle);
      direction.Normalize();

      // Create camera
      char name[1024];
      sprintf(name, "%s#%d#%d", node->Name(), npanorama, i);
      R3Vector towards(direction.X(), -downward_tilt_angle, direction.Y());
      towards.Normalize();
      R3Vector right = towards % R3posy_vector;
      right.Normalize();
      R3Vector up = right % towards;
      up.Normalize();
      R3Camera camerar3(viewpoint, towards, up, xfov, yfov, neardist, fardist);
      RNScalar cameraScore =  SceneCoverageScore(camerar3, scene, node);
      camerar3.SetValue(cameraScore);
      Camera *camera = new Camera(camerar3, name);
      cameras_pano.Insert(camera);
      panoscore += cameraScore>0;
      /*
      if (print_verbose) {
          printf("%s %f %f\n",name, panoscore,mask_value);
      }
      */
    }

    if (panoscore>=2){
      for (int i = 0; i < num_directions_per_panorama; i++) {
        cameras.Insert(cameras_pano[i]);
      }
      // Update counter
      npanorama++;
      // Mask nearby positions
      RNScalar mask_radius = min_distance_between_panorama * mask.WorldToGridScaleFactor();
      mask.RasterizeGridCircle(R2Point(mask_ix, mask_iy), mask_radius, 0.0, R2_GRID_REPLACE_OPERATION);
    }else{
      RNScalar mask_radius = 0.2*min_distance_between_panorama * mask.WorldToGridScaleFactor();
      mask.RasterizeGridCircle(R2Point(mask_ix, mask_iy), mask_radius, 0.0, R2_GRID_REPLACE_OPERATION);
    }
    
  }

  // Return success
  return 1;
}


////////////////////////////////////////////////////////////////////////
// Camera creation functions
////////////////////////////////////////////////////////////////////////

static int
CreatePanoramicCameras(RNArray<Camera *>& cameras, R3Scene *scene)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Create cameras for each level node
  for (int i = 0; i < scene->NNodes(); i++) {
    R3SceneNode *node = scene->Node(i);
    if (!node->Name()) continue;
    if (strncmp(node->Name(), "Room#", 5)) continue;
    if (node->BBox().IsEmpty()) continue;
    if (node->BBox().YLength() < eye_height) continue;
    if (!CreatePanoramicCameras(cameras, scene, node)) continue;
  }
  
  // Print statistics
  if (print_verbose) {
    printf("Created cameras ...\n");
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Cameras = %d\n", cameras.NEntries());
    fflush(stdout);
  }

  // Return success
  return 1;
}


////////////////////////////////////////////////////////////////////////
// Create and write functions
////////////////////////////////////////////////////////////////////////

static void
CreateAndWriteCameras(void)
{
  // Create cameras
  // if (create_object_cameras) CreateObjectCameras();
  // if (create_room_cameras) CreateRoomCameras();
  // if (create_world_in_hand_cameras) CreateWorldInHandCameras();
  CreatePanoramicCameras(cameras, scene);

  

  // Write cameras
  WriteCameras();

  // Exit program
  exit(0);
}



static int
CreateAndWriteCamerasWithGlut(void)
{
#ifdef USE_GLUT
  // Open window
  int argc = 1;
  char *argv[1];
  argv[0] = strdup("scn2cam");
  glutInit(&argc, argv);
  glutInitWindowPosition(100, 100);
  glutInitWindowSize(width, height);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH); 
  glutCreateWindow("Scene Camera Creation");

  // Initialize GLUT callback functions 
  glutDisplayFunc(CreateAndWriteCameras);

  // Run main loop  -- never returns 
  glutMainLoop();

  // Return success -- actually never gets here
  return 1;
#else
  // Not supported
  RNAbort("Program was not compiled with glut.  Recompile with make.\n");
  return 0;
#endif
}



static int
CreateAndWriteCamerasWithMesa(void)
{
#ifdef USE_MESA
  // Create mesa context
  OSMesaContext ctx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, NULL);
  if (!ctx) {
    fprintf(stderr, "Unable to create mesa context\n");
    return 0;
  }

  // Create frame buffer
  void *frame_buffer = malloc(width * height * 4 * sizeof(GLubyte) );
  if (!frame_buffer) {
    fprintf(stderr, "Unable to allocate mesa frame buffer\n");
    return 0;
  }

  // Assign mesa context
  if (!OSMesaMakeCurrent(ctx, frame_buffer, GL_UNSIGNED_BYTE, width, height)) {
    fprintf(stderr, "Unable to make mesa context current\n");
    return 0;
  }

  // Create cameras
  CreateAndWriteCameras();

  // Delete mesa context
  OSMesaDestroyContext(ctx);

  // Delete frame buffer
  free(frame_buffer);

  // Return success
  return 1;
#else
  // Not supported
  RNAbort("Program was not compiled with mesa.  Recompile with make mesa.\n");
  return 0;
#endif
}



////////////////////////////////////////////////////////////////////////
// Program argument parsing
////////////////////////////////////////////////////////////////////////

static int 
ParseArgs(int argc, char **argv)
{
  // Parse arguments
  argc--; argv++;
  while (argc > 0) {
    if ((*argv)[0] == '-') {
      if (!strcmp(*argv, "-v")) print_verbose = 1;
      else if (!strcmp(*argv, "-glut")) { mesa = 0; glut = 1; }
      else if (!strcmp(*argv, "-mesa")) { mesa = 1; glut = 0; }
      else if (!strcmp(*argv, "-raycast")) { mesa = 0; glut = 0; }
      else if (!strcmp(*argv, "-categories")) { argc--; argv++; input_categories_filename = *argv; }
      else if (!strcmp(*argv, "-output_camera_extrinsics")) { argc--; argv++; output_camera_extrinsics_filename = *argv; }
      else if (!strcmp(*argv, "-output_camera_intrinsics")) { argc--; argv++; output_camera_intrinsics_filename = *argv; }
      else if (!strcmp(*argv, "-output_camera_names")) { argc--; argv++; output_camera_names_filename = *argv; }
      else if (!strcmp(*argv, "-width")) { argc--; argv++; width = atoi(*argv); }
      else if (!strcmp(*argv, "-height")) { argc--; argv++; height = atoi(*argv); }
      else if (!strcmp(*argv, "-focal_length")) { argc--; argv++; focal_length = atof(*argv); }
      else if (!strcmp(*argv, "-eye_height")) { argc--; argv++; eye_height = atof(*argv); }
      else if (!strcmp(*argv, "-downward_tilt_angle")) { argc--; argv++; downward_tilt_angle = atof(*argv); }
      else if (!strcmp(*argv, "-min_distance_between_panorama")) { argc--; argv++; min_distance_between_panorama = atof(*argv); }
      else if (!strcmp(*argv, "-min_distance_from_obstacle")) { argc--; argv++; min_distance_from_obstacle = atof(*argv); }
      else if (!strcmp(*argv, "-num_directions_per_panorama")) { argc--; argv++; num_directions_per_panorama = atoi(*argv); }
      else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
      argv++; argc--;
    }
    else {
      if (!input_scene_filename) input_scene_filename = *argv;
      else if (!output_cameras_filename) { output_cameras_filename = *argv; }
      else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
      argv++; argc--;
    }
  }

  // Check filenames
  if (!input_scene_filename || !output_cameras_filename) {
    fprintf(stderr, "Usage: shurancam inputscenefile outputcamerafile\n");
    return 0;
  }

  // Return OK status 
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  // Parse program arguments
  if (!ParseArgs(argc, argv)) exit(-1);

  // Read scene
  if (!ReadScene(input_scene_filename)) exit(-1);

  // Create cameras

  // Read categories
  if (input_categories_filename) {
    if (!ReadCategories(input_categories_filename)) exit(-1);
  }

  if (mesa) CreateAndWriteCamerasWithMesa();
  else if (glut) CreateAndWriteCamerasWithGlut();
  else CreateAndWriteCameras();

  

  // Return success 
  return 0;
}




