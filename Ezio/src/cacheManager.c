//
// Created by ezio on 28/08/19.
//

#include"hashFunction.h"

struct image imageToInsert;


//this function is called by the ServerBranch to chech
//if an image is present into the cache
//if this function returns 0 it means that the bytes of the image
//that the branch is looking for, have not been copied
//if it returns 1 it means that imageToGet.imageBytes
//has been copied from the cache to imageToGet->imageBytes
int getImageInCache(struct image *imageToGet)
{
    //printf("getImageInCache from branch %d\n", getpid());
    
    int key = hashFunction(*imageToGet);

    //printf("heash_key generated in getImage: %d\n", key);

    struct hash_element *hashElement = (cache + key);

    struct image *tempImage;

    //printf("2 waiting in semToHashBlock in cache\n");
    if(sem_wait(&(hashElement->semToHashBlock)) == -1 ){
        perror("Error in semwait (getImage in cache)");
        return -1;
    }

    for(int i = 0;i < MAX_IMAGE_NUM_PER_KEY; ++i) {

        tempImage = (hashElement->conflictingImages);
        
        //printf("Looking for image in cache, in %d hash_element and i: %d\n", key, i);

        //if the counters of a conflictingImages is 0
        //it means that the element is not in cache
        if (tempImage->counter == 0) {

            //printf("3 posting in semToHashBlock in cache\n");
            if(sem_post(&(hashElement->semToHashBlock)) == -1 ){
                perror("Error in sempost (getImage in cache)");
                return -1;
            }

            return 0;

        }

        //printf("1\n");
        
        if(strcmp(tempImage->name, imageToGet->name) != 0)
            continue;

        //printf("2\n");

        if(tempImage->quality != imageToGet->quality)
            continue;

        //printf("3\n");

        if(tempImage->width != imageToGet->width)
            continue;

        //printf("4\n");

        if(tempImage->height != imageToGet->height)
            continue;

        //printf("5\n");

        if(imageToGet->isPng != 2 && tempImage->isPng != imageToGet->isPng)
            continue;

        //printf("6\n");

        //here the image has been found
        tempImage->counter++;

        memcpy(imageToGet->imageBytes, tempImage->imageBytes, tempImage->imageSize);
        imageToGet->isPng = tempImage->isPng;
        imageToGet->imageSize = tempImage->imageSize;

        //printf("3 posting in semToHashBlock in cache\n");
        if(sem_post(&(hashElement->semToHashBlock)) == -1 ){
            perror("Error in sempost (getImage in cache)");
            return -1;
        }

        //printf("Found in position %d\n", i);
        //printf("Its counter is %d\n", tempImage->counter);


        
        return 1;

    }

    //printf("3 posting in semToHashBlock in cache\n");
    if(sem_post(&(hashElement->semToHashBlock)) == -1 ){
        perror("Error in sempost (getImage in cache)");
        return -1;
    }

    //if the function reaches this point it means
    //that all the images saved into the hash_elementare not the requested one
    return 0;

}



//this function is called by the cacheManager to insert
//a new image into the cache
int insert()
{
    //printf("Branch %d inserting image in cache\n", getpid());

    int key = hashFunction(imageToInsert);
    //printf("heash_key generated in insert: %d\n", key);

    struct hash_element *hashElement = (cache + key);

    struct image *imageSet = hashElement->conflictingImages;

    //takeing the counter of the first image into the hash_element selected
    int selectedPosition = (imageSet + 0)->counter;

    //inserting ino the right position
    for(int j = 0; j < MAX_IMAGE_NUM_PER_KEY; ++j){

        if(selectedPosition < (imageSet + j)->counter )
            selectedPosition = (imageSet + j)->counter;
    }

    //printf("5 waiting in semToHashBlock in cache\n");
    if(sem_wait(&(hashElement->semToHashBlock)) == -1 ){
        perror("Error in semwait (insert in cache)");
        return -1;
    }

    //the image with the minimum counter has been selected, inserting
    (imageSet + selectedPosition)->counter = 1;

    (imageSet + selectedPosition)->imageSize = imageToInsert.imageSize;
    (imageSet + selectedPosition)->width = imageToInsert.width;
    (imageSet + selectedPosition)->height = imageToInsert.height;
    (imageSet + selectedPosition)->quality = imageToInsert.quality;
    (imageSet + selectedPosition)->isPng = imageToInsert.isPng;
    memcpy((imageSet + selectedPosition)->name, imageToInsert.name, strlen(imageToInsert.name));
    memcpy((imageSet + selectedPosition)->imageBytes, imageToInsert.imageBytes, imageToInsert.imageSize);
    (imageSet + selectedPosition)->imageSize = imageToInsert.imageSize;

    //printf("Inserted image in cache, in %d position and i (in image array): %d\n", key, selectedPosition);

    //printf("6 posting in semToHashBlock in cache\n");
    if(sem_post(&(hashElement->semToHashBlock)) == -1 ){
        perror("Error in semwait (insert in cache)");
        return -1;
    }

    return 0;
}


//when the cache manager is called he has to check
//1.   if a struct image is present into the cache
//1.1  return the image
// OR
//2.   write a struct image given by the ServerBranch into the cache
//     this has to be done by a thread parallel to the ServerBranch
void cacheManager()
{
    //attaching to the cache
    while(1){

        //printf("cacheManager %d waiting the branch\n", getpid());

        //printf("1 waiting in activateCacheManager in cache\n");
        //waiting to insert a new element into the cache
        if(sem_wait(&activateCacheManager) == -1){
            perror("Error in semwait (activateCacheManager)");
            return;
        }

        if(insert()){
            perror("Error insert");
            return;
        }

        //printf("7 posting in cacheManagerHasFinished in cache\n");
        //posting to insert a new element into the cache
        if(sem_post(&cacheManagerHasFinished) == -1){
            perror("Error in sempost (cacheManagerHasFinished)");
            return;
        }
    }

}