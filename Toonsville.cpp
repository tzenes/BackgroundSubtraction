
#include "Toonsville.h"
#include <stdio.h>


int* quantize(char r, char g, char b){
    //convert from uint8 to int32, if you try to pass directly you FAIL!!!
    int tempr, tempb, tempg;
    tempr = r&0xFF;
    tempg = g&0xFF;
    tempb = b&0xFF;
    //printf("%d, %d, %d\n", tempr, tempg, tempb);
    
    float* Qhsv, *HSV = RGBtoHSV(tempr/256.f, tempg/256.f, tempb/256.f);
    int* RGB = (int*)malloc(sizeof(int)*3);
    float h, s, v;
    h = HSV[0];
    s = HSV[1];
    v = HSV[2];
    
   	
	
	//clamp h
	if (((h >= -15.0f) && (h < 15.0f)) || ((h >= 345.0f) && (h < 375.0f))){
	    h = 0.f;
	} else if ((h >= 15.0f) && (h < 45.0f)){
	    h = 30.0f;
	} else if ((h >= 45.0f) && (h < 75.0f)){    
	    h = 60.0f;
	} else if ((h >= 75.0f) && (h < 105.0f)){
	    h = 90.0f;
	} else if ((h >= 105.0f) && (h < 135.0f)){
	    h = 120.0f;
	} else if ((h >= 135.0f) && (h < 165.0f)){
	    h = 150.0f;
	} else if ((h >= 165.0f) && (h < 195.0f)){
		h = 180.0f;
	} else if ((h >= 195.0f) && (h < 225.0f)){
		h = 210.0f;
    } else if ((h >=225.0f) && (h < 255.0f)) {
		h = 240.0f;
    } else if ((h >= 255.0f) && (h < 285.0f)){
		h = 270.0f;
    } else if ((h >=285.0f) && (h < 315.0f)){
		h = 300.0f;
    } else if ((h >= 315.0f) && (h < 345.0f)){
		h = 330.0f;
	}
 
        


	//clamp s
       
    // SMR: My modified clamping to handle boundery conditions
	/*	 if(s < .15)
            s = .0f;
        else if(s >= .15 && s < .45)
            s = .3f;
        else if(s >= .45 && s < .75)
            s = .6f;
        else
            s = 1.f;
	*/
    if(v < .15)
         v = .0f;
    else if(v >= .15 && v < .45)
         v = .3f;
    else if(v >= .45 && v < .75)
         v = .6f;
    else
         v = 1.f;
        
	
	Qhsv = HSVtoRGB(h,s,v);
	
	RGB[0] = (int)Qhsv[0];
	RGB[1] = (int)Qhsv[1];
	RGB[2] = (int)Qhsv[2];
	
	free(HSV);
	free(Qhsv);
	return RGB;
 }	




// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
float* RGBtoHSV( float r, float g, float b ){
	float imax, imin, h, s, v;
	float* HSV = (float*)malloc(sizeof(float)*3);
        
	// Calculate the value component
	if (r > g) {
	    imax = r;
	    imin = g;
	}
	else {
	    imin = r;
	    imax = g;
	}
	if (b > imax) 
	    imax = b;
	if (b < imin)
	    imin = b;
	v = imax;				// v


	//Calculate the saturation component
	if( imax != 0 )
	    s = (imax - imin) / imax;		// s
	else {
		s = 0;
	}

	//Calculate the hue
	if (s == 0) 
	    h = -1;
	else {
	    float delta = imax - imin;
	    
	    if( r == imax )
		h = ( g - b ) / delta;		// between yellow & magenta
	    else if( g == imax )
		h = 2 + ( b - r ) / delta;	// between cyan & yellow
	    else
		h = 4 + ( r - g ) / delta;	// between magenta & cyan
	}
	h *= 60;				// degrees
	while( h < 0 )                          //SMR: Add until greater than 0
		h += 360;
        while(h > 360)
                h -= 360;                       //SMR: Subtract till less than 360
        
	HSV[0] = h;
	HSV[1] = s;
	HSV[2] = v;
	return HSV;
}

//SMR: input 0-1, output 0-255
float* HSVtoRGB(float h, float s, float v ){
    int i;
    float f, p, q, t, r = 0, g = 0, b = 0;
    float* rgb = (float*)malloc(sizeof(float)*3);
    float fHue, fSat, fVal;

    if( s == 0 ) {
	// achromatic (grey)
	r = g = b = v*255;
	
    }
    else {
	if (h == 360)
	    h = 0;
	fHue = h/60;
	//i = (int)fHue; //SMR: superfluous
	i = (int)floor(fHue);
	f = fHue - (float)i;			// factorial part of h

        /*  SMR: no need for this conversion v and s are 0-1
	fVal = (float)v/255;
	fSat = (float)s/255;
        */
        fVal = v;
        fSat = s;

	p = fVal * ( 1 - fSat);
	q = fVal * ( 1 - (fSat * f) );
	t = fVal * ( 1 - (fSat * ( 1 - f ) ));
	switch( i ) {
		case 0:
		        r = fVal * 255;
			g = t * 255;
			b = p * 255;
			break;
		case 1:
			r = q * 255;
			g = fVal * 255;
			b = p * 255;
			break;
	        case 2:
			r = p * 255;
			g = fVal * 255;
			b = t * 255;
			break;
		case 3:
			r = p * 255;
			g = q * 255;
			b = fVal * 255;
			break;
		case 4:
			r = t * 255;
			g = p * 255;
			b = fVal * 255;
			break;
		case 5:
			r = fVal * 255;
			g = p * 255;
			b = q * 255;
			break;
	}
    }
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
	return rgb;
}
	
