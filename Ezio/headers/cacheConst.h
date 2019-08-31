//
// Created by ezio on 28/08/19.
//

#define MAX_IMAGE_DIM 400000                //Max dimension of an image in bytes

#define MAX_HASH_KEYS 800                   //max keys used in has function

#define MAX_IMAGE_NUM_PER_KEY 3             //the dimension of the array containing all the
                                            //pictures having the same hash function key

#define MAX_IMAGENAME_DIM 20                //max length of a image's name

struct image{

    char name[MAX_IMAGENAME_DIM];           //image name
    char quality;                           //the double q (for example 0.8) has to be converted
                                            //in an integer that goes from 0 to 9
    unsigned short width;
    unsigned short height;
    char isPng;
    char imageBytes[MAX_IMAGE_DIM];
    int imageSize;

    unsigned int counter;                   //number of times that the image is accessed
};

struct hash_element{

    unsigned short key;
    struct image conflictingImages[MAX_IMAGE_NUM_PER_KEY];
    sem_t semToHashBlock;
    sem_t dontRead;
};
