#include <ImageMagick-7/MagickWand/MagickWand.h>


int resizer(char* path){
    size_t temp_size;
    unsigned char* temp_buff;
    MagickWand* m_wand = NULL;
    char* temp_format;
    double widthImage, heightImage, widthRapport,heightRapport;
    int widthDesire, heightDesire;

    MagickBooleanType status; //variable used to check on function results

    MagickWandGenesis();  //function of the api that does an obscure work to make the api work

    m_wand = NewMagickWand(); // I make the wand object, the core of these api



    status= MagickReadImage(m_wand, path+1); // Loading the Image in the wand so it can be manipulated
    if (status == MagickFalse){
        setting.error = true;
        strcpy(setting.statusCode, NF);
        return -1;
    }

    widthImage = MagickGetImageWidth(m_wand);
    heightImage = MagickGetImageHeight(m_wand);

    widthRapport = setting.width / widthImage;
    heightRapport = setting.height / heightImage;
    
    printf("Resolution Image is: %f x %f\n",widthImage,heightImage);

    printf("Resolution Phone is: %d x %d\n",setting.width,setting.height);
    printf("Resolution Rapports is: width %f x height %f\n", widthRapport, heightRapport);

    if (widthRapport <= heightRapport && widthRapport < 1){
        widthDesire = (int)(widthImage * widthRapport);
        heightDesire = (int)(heightImage * widthRapport);
        printf("Resolution Desire is: %d x %d in if\n",widthDesire,heightDesire);

    } else if (widthRapport > heightRapport && heightRapport < 1) {
        widthDesire = (int) (widthImage * heightRapport);
        heightDesire = (int) (heightImage * heightRapport);
        printf("Resolution Desire is: %d x %d in else if\n",widthDesire,heightDesire);

    } else {
        widthDesire = (int)widthImage;
        heightDesire = (int)heightImage;
        printf("Resolution Desire is: %d x %d in else\n",widthDesire,heightDesire);

    }
    
    

    status = MagickAdaptiveResizeImage(m_wand,widthDesire,heightDesire); // resizing the iage in the wand

    if (status == MagickFalse){
        perror("Error in resizing");
        setting.error = true;
        strcpy(setting.statusCode, ISE );
        return -1;
    }

    status= MagickSetImageCompressionQuality(m_wand, (int)(setting.quality*100)); // Set the compression quality to quality (high quality = low compression)
    if (status == MagickFalse){
        perror("Error in compression");
        setting.error = true;
        strcpy(setting.statusCode, ISE );
        return -1;
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

    if(temp_size > MAX_IMAGE_DIM){
        perror("Image too big after adaptation");
        setting.error = true;
        strcpy(setting.statusCode, ISE);
        return -1;
    }
    bcopy((void*)temp_buff, (void*)imageToInsert.imageBytes, temp_size); //writing the image in the allocated memory

    imageToInsert.imageSize = temp_size;

    if(imageToInsert.imageSize > MAX_IMAGE_DIM){
        perror("Image too big after adaptation");
        setting.error = true;
        strcpy(setting.statusCode, ISE);
        return -1;
    }

    temp_format= MagickGetImageFormat(m_wand);

    if(strcmp(temp_format, "JPEG")==0){
        imageToInsert.isPng = 0;
    }
    else if(strcmp(temp_format, "PNG")==0){
        imageToInsert.isPng = 1;
    }

    /* Clean up */
    MagickRelinquishMemory((void*)temp_buff);


    if(m_wand)
        m_wand = DestroyMagickWand(m_wand); //try: after destoy, if(m_wand)=?maybe false

    MagickWandTerminus();
    return 0;
}