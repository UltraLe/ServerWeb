/* compile with gcc 'solo funzione.c' `pkg-config --cflags --libs MagickWand` -O2 -Wall  -o slf
*/

#include <ImageMagick-7/MagickWand/MagickWand.h>

typedef struct image_blob_info{
    unsigned char* buff;
    size_t size; //in byte
    char format[15];
} im_info_t;

/* il path deve essere dato nella forma images/image,
   e non /images/image
*/


im_info_t* resizer(char* path){
    size_t temp_size;
    unsigned char* temp_buff;
    im_info_t* im_info;
    MagickWand* m_wand = NULL;
    char* temp_format;

    MagickBooleanType status; //variable used to check on function results

    MagickWandGenesis();  //function of the api that does an obscure work to make the api work

    m_wand = NewMagickWand(); // I make the wand object, the core of these api

    status= MagickReadImage(m_wand, path+1); // Loading the Image in the wand so it can be manipulated
    if (status == MagickFalse){
        perror("file not Found");
        setting.error = true;
        strcpy(setting.statusCode, NF);
        return NULL;
    }

    status = MagickAdaptiveResizeImage(m_wand,setting.width,setting.height); // resizing the iage in the wand
    if (status == MagickFalse){
        perror("Error in resizing");
        setting.error = true;
        strcpy(setting.statusCode, ISE );
        return NULL;
    }

    status= MagickSetImageCompressionQuality(m_wand, (int)(setting.quality*100)); // Set the compression quality to quality (high quality = low compression)
    if (status == MagickFalse){
        perror("Error in compression");
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

    temp_buff = MagickGetImageBlob(m_wand,&temp_size); // writing image in ram as a stream of byte

    bcopy((void*)temp_buff, (void*)imageToInsert.imageBytes, temp_size); //writing the image in the allocated memory

    imageToInsert.imageSize = temp_size;

    temp_format= MagickGetImageFormat(m_wand);

    if(strcmp(temp_format, "JPEG")==0){
        imageToInsert.isPng = 0;
    }
    else if(strcmp(temp_format, "PNG")==0){
        imageToInsert.isPng = 1;
    }

    /* Clean up */
    MagickRelinquishMemory((void*)temp_buff);
    if(m_wand)m_wand = DestroyMagickWand(m_wand); //try: after destoy, if(m_wand)=?maybe false
    MagickWandTerminus();
    return im_info;
}