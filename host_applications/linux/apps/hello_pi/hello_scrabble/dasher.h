#define MAXDASHER 256


typedef struct {
   int minx, maxx;
   int minz, maxz;
   int scalex, scalez;
   int curx, curz;
   VC_IMAGE_T *cam_img;
   uint8_t *refy;
   int mirror;
   int rotate;
} MOTION_T;

typedef struct dasher_s
{
   int words[MAXLETTER]; // null terminated string
   char letters[MAXDASHER];
   int numletters;
   int width;
   int x,y,z;  /* Current location */
   int tx,ty,tz; /* Target location */
   int velocx, accelx;
   int velocz, accelz;
   MOTION_T motion;
} DASHER_T;

