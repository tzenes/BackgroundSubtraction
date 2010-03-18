// Taken, in part, from OpenCV Sample Application: facedetect.c

// Include header files
#include "cv.h"
#include "highgui.h"
#include "CvPixelBackgroundGMM.h"
#include "Toonsville.h"
#include <image.h>
#include <misc.h>
#include <filter.h>
#include "segment-graph.h"
#include "segment-image.h"

extern "C" 
{ 
#include <potracelib.h>
#include "backend_svg.h"
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

//this assumes 32 bit architecture
#define ARCHITECTURE (sizeof(unsigned long)*8)
#define PlacePoPixel(poobject,i,j) (poobject->map + j*poobject->dy)[i/ARCHITECTURE] |= (1 << (ARCHITECTURE-1-i%ARCHITECTURE))
#define GetPoPixel(poobject,i,j) ((poobject->map + j*poobject->dy)[i/ARCHITECTURE] & (1 << (ARCHITECTURE-1-i%ARCHITECTURE))) ? 1 : 0

// Outputfile
static FILE *OutFile;

// Create memory for calculations
static CvMemStorage* storage = 0;

// More memory for calculations
CvPixelBackgroundGMM* pGMM=0;

// Function prototype for detecting and drawing an object from an image
void detect_and_draw( IplImage* image );
int FindonList(int element, int* list, int size);

// Create a new Haar classifier
static CvHaarClassifierCascade* cascade = 0;

// Create a string that contains the cascade name
const char* cascade_name =
    "haarcascade_frontalface_alt.xml";
/*    "haarcascade_profileface.xml";*/

potrace_state_t** TraceSegments(potrace_bitmap_t** bitmaps,const int size);
IplImage* QuantizeImage(IplImage* in);
universe* SegmentImage(IplImage* input, const CvPoint pt1, const CvPoint pt1, int* NumberOfSegments, bool inside);
potrace_bitmap_t** bmpalloc(const int size, const int width, const int height);
void FindFaces(IplImage* input, CvPoint* pt1, CvPoint* pt2);

bool singleframe = false;

// Main function, defines the entry point for the program.
int main( int argc, char** argv )
{

    // Structure for getting video from camera or avi
    CvCapture* capture = 0;

    // Images to capture the frame from video or camera or from file
    IplImage *frame, *frame_copy = 0;

    // Input file name for avi or image file.
    const char* input_name;

    // Check for the correct usage of the command line
    if( argc <= 2 )
        input_name = argv[1];
    else
    {
        fprintf( stderr,
        "Usage: BSubtraction Filename\n" );
        system ("pause"); // MS-DOS pause command
        return -1;
        /*input_name = argc > 1 ? argv[1] : 0;*/
    }

    // Configure output file
    OutFile = fopen("svgout.svg", "w+");
    
    
    // Initialize Face detection
    // Load the HaarClassifierCascade
    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    
    
    // Allocate the memory storage
    storage = cvCreateMemStorage(0);
    
    // Find whether to detect the object from file or from camera.
    if( !input_name || (isdigit(input_name[0]) && input_name[1] == '\0') )
        capture = cvCaptureFromCAM( !input_name ? 0 : input_name[0] - '0' );
    else
        capture = cvCaptureFromAVI( input_name ); 

    // Create a new named window with title: result
    cvNamedWindow( "result", 1 );
    cvNamedWindow("original", 1);

    // Find if the capture is loaded successfully or not.

    // If loaded succesfully, then:
    if( capture )
    {
        
        // Capture from the camera.
        for(;;)
        {
            // Capture the frame and load it in IplImage
            if( !cvGrabFrame( capture ))
                break;
            frame = cvRetrieveFrame( capture );

            // If the frame does not exist, quit the loop
            if( !frame )
                break;

            // Allocate framecopy as the same size of the frame
            if( !frame_copy )
                frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
                                            IPL_DEPTH_8U, frame->nChannels );

            // Reserve Memory for background subtraction if you haven't already
            pGMM = (pGMM==0) ? cvCreatePixelBackgroundGMM(frame->width,frame->height)
                           : pGMM;
            pGMM->fAlphaT = .005f;

            // Check the origin of image. If top left, copy the image frame to frame_copy. 
            if( frame->origin == IPL_ORIGIN_TL )
                cvCopy( frame, frame_copy, 0 );
            // Else flip and copy the image
            else
                cvFlip( frame, frame_copy, 0 );
            
            // Call the function to detect and draw the facees
            detect_and_draw( frame_copy );
            
            //system ("pause"); // MS-DOS pause command
            
            // Wait for a while before proceeding to the next frame
            if( cvWaitKey( 10 ) >= 0 )
                break;
        }

        // Release the images, and capture memory
        cvReleaseImage( &frame_copy );
        cvReleaseCapture( &capture );
    }else{// Assume the image to be lena.jpg, or the input_name specified
        const char* filename = input_name ? input_name : (char*)"test.jpg";
        singleframe = true;
        // Load the image from that filename
        IplImage* image = cvLoadImage( filename, 1 );
        // If Image is loaded succesfully, then:
        if( image ){
            pGMM = (pGMM==0) ? cvCreatePixelBackgroundGMM(image->width,image->height)
                           : pGMM;
            pGMM->fAlphaT = .005f;
            // Detect and draw the face
            detect_and_draw( image );

            // Release the image memory
            cvReleaseImage( &image );
        }
    }      

    
    // Destroy the window previously created with filename: "result"
    cvDestroyWindow("result");
    cvDestroyWindow("original");
    
    // release the background subtraction structure
    //cvReleasePixelBackgroundGMM(&pGMM);
    
    // write out terminal data and close your svg file
    page_svg_close(OutFile);    
    fclose(OutFile);    

    // return 0 to indicate successfull execution of the program
    return 0;
}

// Function to detect and draw any faces that is present in an image
void detect_and_draw( IplImage* img )
{
    potrace_state_t** states;
    potrace_state_t** fstates;
    int tmp, num_ccs, face_ccs, *compcolor, *pixeldistribution;
    int *segmentsize, *facesize, *facecolor, *facelist, *complist, scale = 1;
    universe *u, *fu;  //fu is the face universe, get your mind out of the gutter
    potrace_bitmap_t** VGbmps, **Facebmps;
    static int framenumber = 0;
    CvPoint pt1, pt2;
    
    // Create a new image based on the input image
    IplImage* quantized;
    IplImage* temp = cvCreateImage( cvSize(img->width/scale,img->height/scale), 8, 3 );

    if(framenumber==0)page_svg_open(OutFile, img->width, img->height);

    if(framenumber==37)system("pause");

    cvShowImage( "original", img );
    
    //find all faces
    FindFaces(img, &pt1, &pt2);
    
    // Quantize our input
    quantized = QuantizeImage(img);
    
    /*
    system("pause");
    if(!cvSaveImage("output.bmp",quantized)){ printf("Could not save: %s\n","output.bmp");
    system("pause");}
    return;*/
    // do segmentation
    u = SegmentImage(quantized, pt1, pt2, &num_ccs, false);
    fu = SegmentImage(quantized, pt1, pt2, &face_ccs, true);
    
    complist = (int*) malloc(sizeof(int)*num_ccs);
    compcolor = (int*) malloc(sizeof(int)*num_ccs);
    facelist = (int*) malloc(sizeof(int)*face_ccs);
    facecolor = (int*) malloc(sizeof(int)*face_ccs);

    /*************************************************************************/
    // this is the workforce code, it does all the cool stuff
    
    
    //allocate bitmaps and associated data
    VGbmps = (potrace_bitmap_t**)malloc(sizeof(potrace_bitmap_t*)*num_ccs);
    //figure out distributions for background sutraction
    segmentsize = (int*) malloc(sizeof(int)*num_ccs);
    facesize = (int*) malloc(sizeof(int)*face_ccs);
    pixeldistribution = (int*) malloc(sizeof(int)*num_ccs);
    VGbmps = bmpalloc(num_ccs, img->width, img->height);
    for(int k=0; k<num_ccs; k++)compcolor[k] = complist[k] = segmentsize[k] = pixeldistribution[k] = 0;
    Facebmps = bmpalloc(face_ccs, img->width, img->height);
    for(int k=0; k<face_ccs; k++)facecolor[k] = facelist[k] = facesize[k] = 0;

    
    
    // find the background
    cvUpdatePixelBackgroundGMM(pGMM,(unsigned char*)quantized->imageData,
                                              (unsigned char*)temp->imageData);
    int comp;
    //Find number of pixels in each segment which are foreground
    for(int j = 0; j<img->height; j++){
    for(int i = 0; i<img->width; i++){
            int k = j*img->width+i;
            
            if(!(((i>pt1.x)&&(i<pt2.x))&&((j>pt1.y)&&(j<pt2.y)))){
                comp = u->find(k);
                comp = FindonList(comp,complist,num_ccs);
                //incrament its segment
                segmentsize[comp]++;
                //increase its distribution if its forground
                pixeldistribution[comp] += (temp->imageData[k*3]==0)?0:1;
                //note temp is grayscale so it doesn't matter which color we take
                // just take one for every 3
            }else{
                comp = fu->find(k);                
                comp = FindonList(comp,facelist,face_ccs); 
                facesize[comp]++; 
            }//don't bother with the face pixels they're getting 1's anyway
    }}


    double *R = (double*)malloc(sizeof(double)*num_ccs);
    double *G = (double*)malloc(sizeof(double)*num_ccs);
    double *B = (double*)malloc(sizeof(double)*num_ccs);
    for(int k = 0; k<num_ccs; k++)R[k] = G[k] = B[k] = 0.0;
    
    double *fR = (double*)malloc(sizeof(double)*face_ccs);
    double *fG = (double*)malloc(sizeof(double)*face_ccs);
    double *fB = (double*)malloc(sizeof(double)*face_ccs);
    for(int k = 0; k<face_ccs; k++)fR[k] = fG[k] = fB[k] = 0.0;
    
    
    
    for(int j = 0; j<img->height; j++){
    for(int i = 0; i<img->width; i++){
            int r,g,b;
            int k = j*img->width+i;
            if(!(((i>pt1.x)&&(i<pt2.x))&&((j>pt1.y)&&(j<pt2.y)))){
                 comp = u->find(k);
                 comp = FindonList(comp,complist,num_ccs);
                 r = (quantized->imageData[k*3+2]&0xFF);
                 g = (quantized->imageData[k*3+1]&0xFF);
                 b = (quantized->imageData[k*3+0]&0xFF);
                 R[comp] += r/double(segmentsize[comp]);
                 G[comp] += g/double(segmentsize[comp]);
                 B[comp] += b/double(segmentsize[comp]);
                 compcolor[comp] = (r&0xFF)<<16 | (g&0xFF)<<8 | b&0xFF;
                 PlacePoPixel(VGbmps[comp],i,j);
                 
            }else{
                 comp = fu->find(k);
                 comp = FindonList(comp,facelist,face_ccs);
                 r = (quantized->imageData[k*3+2]&0xFF);
                 g = (quantized->imageData[k*3+1]&0xFF);
                 b = (quantized->imageData[k*3+0]&0xFF);
                 fR[comp] += r/double(facesize[comp]);
                 fG[comp] += g/double(facesize[comp]);
                 fB[comp] += b/double(facesize[comp]);
                 facecolor[comp] = (r&0xFF)<<16 | (g&0xFF)<<8 | b&0xFF;
                 PlacePoPixel(Facebmps[comp],i,j); 
            }
    }
    }
    for(int k = 0; k<num_ccs; k++) compcolor[k] =(int(R[k])&0xFF)<<16 | (int(G[k])&0xFF)<<8 | int(B[k])&0xFF;     
    for(int k = 0; k<face_ccs; k++) facecolor[k] =(int(fR[k])&0xFF)<<16 | (int(fG[k])&0xFF)<<8 | int(fB[k])&0xFF;     

    //for(int k = 0; k<num_ccs; k++) compcolor[k] =(int(rand()%255)&0xFF)<<16 | (int(rand()%255)&0xFF)<<8 | int(rand()%255)&0xFF;     
    //for(int k = 0; k<face_ccs; k++) facecolor[k] =(int(rand()%255)&0xFF)<<16 | (int(rand()%255)&0xFF)<<8 | int(rand()%255)&0xFF;     

    
    /*************************************************************************/


    float percentage;
    //generalize segement color
    for(int j = 0; j<img->height; j++){
    for(int i = 0; i<img->width; i++){
            int k = j*img->width+i;
            if(!(((i>pt1.x)&&(i<pt2.x))&&((j>pt1.y)&&(j<pt2.y)))){
                 comp = u->find(k);
                 comp = FindonList(comp,complist,num_ccs);
                 percentage = ((float)pixeldistribution[comp])/((float)segmentsize[comp]);
                 if(percentage > -1.f){
                       quantized->imageData[k*3+2] = (compcolor[comp]>>16)&0xFF;
                       quantized->imageData[k*3+1] = (compcolor[comp]>>8)&0xFF;
                       quantized->imageData[k*3+0] = (compcolor[comp])&0xFF;
                 }else{
                       quantized->imageData[k*3+2] = (0>>16)&0xFF;
                       quantized->imageData[k*3+1] = (0>>8)&0xFF;
                       quantized->imageData[k*3+0] = (0)&0xFF;
                       
                 }      
            }else{
                 comp = fu->find(k);
                 comp = FindonList(comp,facelist,face_ccs);
                 quantized->imageData[k*3+2] = (facecolor[comp]>>16)&0xFF;
                 quantized->imageData[k*3+1] = (facecolor[comp]>>8)&0xFF;
                 quantized->imageData[k*3+0] = (facecolor[comp])&0xFF; 
            }
    }
    }
    //printf("test %d\n", VGbmps[0]->map[2]);
    int current, g, currenttemp, something;
    
    
    
    //Show the image in the window named "result"
    cvShowImage( "result", quantized );


    
    // Do the tracing on each color segment
    states = TraceSegments(VGbmps,num_ccs);
    fstates = TraceSegments(Facebmps,face_ccs);
    
    
    //write out this frame to the svg file.
    if(singleframe)fprintf(OutFile,"<g>\n"); else fprintf(OutFile, "<g display=\"none\">\n");
    for(int k=0; k<num_ccs; k++)page_svg_insert(OutFile, states[k]->plist, compcolor[k], ((float)pixeldistribution[k])/((float)segmentsize[k]));
    for(int k=0; k<face_ccs; k++)page_svg_insert(OutFile, fstates[k]->plist, facecolor[k], 1.f);
    if(!singleframe)fprintf(OutFile, "<animate attributeType=\"CSS\" attributeName=\"display\" values=\"inline;none\" begin=\"%ds\" dur=\"2s\" fill=\"freeze\"/>\n", framenumber/4);
    fprintf(OutFile, "</g>\n");
    
    
    framenumber++;  
    
    cvReleaseImage( &temp );
    cvReleaseImage( &quantized );
    
    for(int k=0; k<num_ccs; k++){        
            free(VGbmps[k]->map);
            delete VGbmps[k];
            potrace_state_free(states[k]);
    };
    for(int k=0; k<face_ccs; k++){        
            free(Facebmps[k]->map);
            delete Facebmps[k];
            potrace_state_free(fstates[k]);
    };
    free(fstates);
    free(states);
    free(VGbmps);
    free(Facebmps);
    free(R);
    free(G);
    free(B);
    free(compcolor);
    free(facecolor);
    free(facelist);
    free(pixeldistribution);
    free(segmentsize);
    free(complist);
    delete u;
    delete fu;
    
    return;
}

//Find a component on a list, if component doesn't exist, add it to nearest 0
int FindonList(int element, int *list,  int size){
     int i;
     for(i=0; i<size; i++){
             if(list[i]==element)
                           return i;//it exists, don't worry        
             if(list[i]==0)//we've come to the end of the list
                           break;
     }
     if(i==size)
             printf("reached end of list, there is no room, you screwed up the size\n");
     
     if(list[i] == 0){
                list[i] = element; //insert at the end of the list  
     }else{
                printf("FAIL CASE in FindonList\n");     
     }
     return i;
}

// TraceSegments
// takes bitmap pointer array and size
// returns array of potrace_state_t pointers
potrace_state_t** TraceSegments(potrace_bitmap_t** bitmaps,const int size){
    static potrace_param_t *parameters = potrace_param_default(); //freed when program terminates
    potrace_state_t** states;

    states = (potrace_state_t**)malloc(sizeof(potrace_state_t*)*size);
    for(int k=0; k<size; k++){
        states[k] = potrace_trace(parameters,bitmaps[k]);
        if (!states[k] || states[k]->status != POTRACE_STATUS_OK)printf("Comp %d failed to create a vector respresentation\n", k);
    }
    
    return states;  //this must be freed later        
}

// QuantizeImage takes input image and returns output
IplImage* QuantizeImage(IplImage* in){
    IplImage* out = cvCreateImage( cvSize(in->width,in->height), 8, 3 );
    int *RGBtemp;      
    //remember we are BGR not RGB
    for(int k = 0; k<in->width*in->height*3; k+=3){
            RGBtemp = quantize(in->imageData[k+2], in->imageData[k+1],
                                                      in->imageData[k]);
            out->imageData[k] = RGBtemp[2];
            out->imageData[k+1] = RGBtemp[1];
            out->imageData[k+2] = RGBtemp[0];
            free(RGBtemp);
    }
    return out; //this needs to be freed later         
}

//SegmentImage
//takes quantized image, face locations(2), and a reference to number-of-segments
//returns a segment universe
universe* SegmentImage(IplImage* input, const CvPoint pt1, const CvPoint pt2, int* NumberOfSegments, bool inside){
    universe* u;
    int dxdt = (pt2.x - pt1.x)*(pt2.y - pt1.y);
    
    //first we format the image
    image<rgb> *formatted_image = new image<rgb>(input->width,input->height);
    for(int y=0; y<input->height; y++)
            for(int x=0; x<input->width*3; x+=3){
                    if(((x/3>pt1.x)&&(x/3<pt2.x))&&((y>pt1.y)&&(y<pt2.y))){//inside of face
                          imRef(formatted_image,x/3,y).r = !inside?0:input->imageData[input->width*3*y+x+2];
                          imRef(formatted_image,x/3,y).g = !inside?0:input->imageData[input->width*3*y+x+1];;
                          imRef(formatted_image,x/3,y).b = !inside?0:input->imageData[input->width*3*y+x+0];    
                    }else{
                          imRef(formatted_image,x/3,y).r = inside?0:input->imageData[input->width*3*y+x+2];
                          imRef(formatted_image,x/3,y).g = inside?0:input->imageData[input->width*3*y+x+1];
                          imRef(formatted_image,x/3,y).b = inside?0:input->imageData[input->width*3*y+x+0];
                    }
            }
    
    u = segment_image(formatted_image, .5, 50, int(sqrt(double(dxdt))/4.0), NumberOfSegments);
    //u->find(y * width + x); returns the segmented group # as an int for
    //pixel[x][y]
    delete formatted_image;    
    (*NumberOfSegments)++;
    
    return u;                    
}

// bmpalloc takes a size for input and returns an array of
// allocated potrace_bitmap_t points
potrace_bitmap_t** bmpalloc(const int size, const int width, const int height){
    potrace_bitmap_t** output = (potrace_bitmap_t**)malloc(sizeof(potrace_bitmap_t*)*size);
    for(int k=0; k<size; k++){
            output[k] = new potrace_bitmap_t();
            output[k]->w = width;
            output[k]->h = height;
            output[k]->dy = width/ARCHITECTURE;
            //printf("test %d", sizeof(int));
            if((height*width)%ARCHITECTURE >0){ //add an extra word
                   output[k]->dy++;//this is probably super bad
                   printf("BAD THINGS");
            }
            output[k]->map = (potrace_word*)malloc((ARCHITECTURE)*output[k]->dy*output[k]->h);
            
            for(int j = 0; j<width/ARCHITECTURE*height; j++)
                 output[k]->map[j] = 0;
            
    }
    return output;
}
//FindFaces takes an input image and 2 CvPoint pointers and input
// modifies them and returns
void FindFaces(IplImage *input, CvPoint* pt1, CvPoint* pt2){
    // Temp code for finding faces
    // Find whether the cascade is loaded, to find the faces. If yes, then:
    pt1->x = 0;
    pt1->y = 0;
    pt2->x = 0;
    pt2->y = 0;
    if( cascade )
    {

        // There can be more than one face in an image. So create a growable sequence of faces.
        // Detect the objects and store them in the sequence
        CvSeq* faces = cvHaarDetectObjects( input, cascade, storage,
                                            1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
                                            cvSize(10, 10) );

        // Loop the number of faces found.
        for(int i = 0; i < (faces ? faces->total : 0); i++ )
        {
           // Create a new rectangle for drawing the face
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );

            // Find the dimensions of the face,and scale it if necessary
            pt1->x = r->x;
            pt2->x = (r->x+r->width);
            pt1->y = r->y-10;
            pt2->y = (r->y+r->height)+10;

            // Draw the rectangle in the input image
            //cvRectangle( img, pt1, pt2, CV_RGB(255,0,0), 3, 8, 0 );
        }
    }
}
