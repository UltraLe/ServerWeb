/* compile with gcc 'solo funzione.c' `pkg-config --cflags --libs MagickWand` -O2 -Wall  -o slf
*/

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <MagickWand/MagickWand.h>


typedef struct image_blob_info{
   unsigned char* buff;
   size_t size; //in byte
   char format[15];
} im_info_t;



im_info_t* resizer(char* path, int width, int height, int qualita, char* format);


int main(int argc, char *argv[]){
   char path[127];
   char format[15]; // which shuold be JPEG, or PNG or NULL   
   int width, height, quality;

   printf("Path: \n");         //start retrieving info for the resizing
   scanf("%127s", path);
   printf("Width: \n");
   scanf("%d", &width);
   printf("Height: \n");
   scanf("%d", &height);
   printf("Quality: \n");
   scanf("%d", &quality);
   printf("format: \n");         
   scanf("%15s", format);     //end of retrv infos

/*test*/    printf("alive \n");

   im_info_t* im = resizer(path, width, height, quality, format);
   MagickWand* nm_wand= NewMagickWand();
   
   MagickReadImageBlob(nm_wand, (void*)im->buff,im->size);
   MagickWriteImage(nm_wand,"finito");	
   MagickWandTerminus();
   return 0;
}




/* il path deve essere dato nella forma images/image,
   e non /images/image
*/


im_info_t* resizer(char* path, int width, int height, int quality, char* format){
   size_t temp_size;
   unsigned char* temp_buff;
   im_info_t* im_info;
   MagickWand* m_wand = NULL;
   char* temp_format;
   
   MagickBooleanType status; //variable used to check on function results

   MagickWandGenesis();  //function of the api that does an obscure work to make the api work
	
   m_wand = NewMagickWand(); // I make the wand object, the core of these api

   status= MagickReadImage(m_wand, path); // Loading the Image in the wand so it can be manipulated
   if (status == MagickFalse){
       perror("file not Found");
       setting.error = true;
       strcpy(setting.statusCode, NF);
       return NULL;
   }

   status= MagickAdaptiveResizeImage(m_wand,setting.width,setting.height); // resizing the iage in the wand
    perror("Error in reading the image");
    setting.error = true;
    strcpy(setting.statusCode, ISE );
    return NULL;
   }
   
   status= MagickSetImageCompressionQuality(m_wand, setting.quality);//TODO MODIFICARE IN INT   // Set the compression quality to quality (high quality = low compression)
   if (status == MagickFalse){
       perror("Error in reading the image");
       setting.error = true;
       strcpy(setting.statusCode, ISE );
       return NULL;
   }
   
   /* fixing the format*/
   if(strcmp(setting.type, JPEG)==0){
   MagickSetImageFormat(m_wand, "JPEG");  
   } 
   else if(strcmp(setting.type, PNG)==0){
   MagickSetImageFormat(m_wand, "PNG");  
   }

   //TODO Settare il tipo in base all'immagine

   /*writing image ina a buffer 
     and preparing struct to return*/

   //temp_buff = MagickGetImageBlob(m_wand,&temp_size); // writing image in ram as a stream of byte
    imageToInsert. = MagickGetImageBlob(m_wand,&temp_size); // writing image in ram as a stream of byte


im_info= (im_info_t*)malloc(sizeof(im_info_t)); //allocating the struct I'll return
 
   im_info->size= temp_size;

   im_info->buff= (unsigned char*)malloc(temp_size); //allocating memory for the image

   bcopy((void*)temp_buff, (void*)im_info->buff, temp_size); //writing the image in the allocated memory

   temp_format= MagickGetImageFormat(m_wand);

   sprintf(im_info->format, "%s\n", temp_format); //writing image format in the struct

   /* Clean up */
   MagickRelinquishMemory((void*)temp_buff);
   if(m_wand)m_wand = DestroyMagickWand(m_wand); //try: after destoy, if(m_wand)=?maybe false
   MagickWandTerminus();
   return im_info;
}



















