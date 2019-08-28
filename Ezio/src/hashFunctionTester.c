//
// Created by ezio on 28/08/19.
//

#include "const.h"

#define IMAGES_NUM 8

#define MAX_COMMON_WIDTH 6

#define MAX_COMMON_HEIGHT 5

char *imagesName[MAX_IMAGENAME_DIM] = {"caneBagnato", "faro", "nuvole", "OcchioDeforme", "PianetiBelli",
                                       "test", "tizizAlMare", "tree"};

int commonWidth[MAX_COMMON_WIDTH] = {800, 1080, 720, 480, 600, 540};

int commonHeight[MAX_COMMON_HEIGHT] = {1920, 1024, 1280, 854, 960};

unsigned int hashFunction(struct image imageTest)
{
    int key = 0;

    //here comes the magic, the hash function
    key += strlen(imageTest.name)*11;
    key += imageTest.height*imageTest.width;
    key *= imageTest.quality;
    key += imageTest.name[0];
    key += imageTest.name[1];
    key += rand();
    //key *= rand();

    return key%MAX_HASH_KEYS;
}

int main(int argc, char **argv)
{
    struct image imageTest;

    int hashKeys[MAX_HASH_KEYS];

    int hashKeyNumber[MAX_HASH_KEYS];

    unsigned int tempKey;

    //initializing to 0 the hash jeys
    for(int i = 0; i < MAX_HASH_KEYS; ++i)
        hashKeys[i] = 0;

    for(int i = 0; i < MAX_HASH_KEYS; ++i)
        hashKeyNumber[i] = 0;


    //checking the hash keys of all the possible
    //combination of: image_name, quality, width, height, isPng
    for(int a = 0; a < IMAGES_NUM; ++a){

        for(char b = 0; b <= 9; ++b){

            for(int c = 0; c < MAX_COMMON_WIDTH; ++c){

                for(int d = 0; d < MAX_COMMON_HEIGHT; ++d){

                    for(char isPng = 0; isPng <= 1; ++isPng){

                        //retting images values
                        memset(imageTest.name, 0, MAX_IMAGENAME_DIM);
                        memcpy(imageTest.name, imagesName[a], strlen(imagesName[a]));
                        imageTest.quality = b;
                        imageTest.width = c;
                        imageTest.height = d;
                        imageTest.isPng = isPng;

                        tempKey = hashFunction(imageTest);
                        hashKeys[tempKey]++;
                    }

                }

            }

        }
    }

    printf("There were ->[ %d ]<- possibilities\n\n", IMAGES_NUM*10*MAX_COMMON_WIDTH*MAX_COMMON_HEIGHT*2);

    printf("There were ->[ %d ]<- possibilities (Giovanni\n\n", IMAGES_NUM*10*10*2);

    printf("The dimension of each hash_element would be %ld\n\n", sizeof(struct hash_element));

    printf("The dimension of the cache would be %ld, with %d hash keys\n\n", CACHE_BYTES, MAX_HASH_KEYS);

    for(int i = 0; i < MAX_HASH_KEYS; ++i)
        hashKeyNumber[hashKeys[i]] += 1;

    for(int i = 0; i < MAX_HASH_KEYS; ++i)
        printf("Key: %d has registered %d times\n", i, hashKeys[i]);

    printf("\n\n");

    for(int i = 0; i < MAX_HASH_KEYS; ++i)
        printf("There were %d keys with value %d\n", hashKeyNumber[i], i);


    return 0;
}

