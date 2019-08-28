//
// Created by ezio on 28/08/19.
//


struct image imageToInsert;


//this function is called by the ServerBranch to chech
//if an image is present into the cache
//if this function returns 0 it means that the bytes of the image
//that the branch is looking for, have not been copied
//if it returns 1 it means that imageToGet.imageBytes
//has been copied from the cache to imageToGet->imageBytes
int getImageInCache(struct image *imageToGet)
{
    int key = hashFunction(*imageToGet);
    struct hash_element *hashElement = (cache + key);

    struct image tempImage;

    for(int i = 0;i < MAX_IMAGE_NUM_PER_KEY; ++i) {

        tempImage = (hashElement->conflictingImages[0]);

        //if the counters of a conflictingImages is 0
        //it means that the element is not in cache
        if (tempImage.counter == 0)
            return 0;

        if(tempImage.quality != imageToGet->quality)
            continue;

        if(tempImage.width != imageToGet->width)
            continue;

        if(tempImage.height != imageToGet->height)
            continue;

        if(imageToGet->isPng != 2 && tempImage.isPng != imageToGet->isPng)
            continue;

        if(strcmp(tempImage.name, imageToGet->name) != 0)
            continue;

        //here the image has been found
        memcpy(imageToGet->imageBytes, tempImage.imageBytes, tempImage.imageSize);
        return 1;

    }

    //if the function reaches this point it means
    //that all the images saved into the hash_elementare not the requested one
    return 0;

}



//this function is called by the cacheManager to insert
//a new image into the cache
int insert()
{
    int key = hash_function(imageToInsert);
    struct hash_element *hashElement = (cache + key);

    struct image *imageSet = hashElement->conflictingImages;

    //takeing the counter of the first image into the hash_element selected
    int selectedPosition = (imageSet + 0)->counter;

    //inserting ino the right position
    for(int j = 0; j < MAX_IMAGE_NUM_PER_KEY; ++j){

        if(selectedPosition < (imageSet + j)->counter )
            selectedPosition = (imageSet + j)->counter;
    }

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

        //waiting to insert a new element into the cache
        if(sem_wait(&activateCacheManager) == 0){
            perror("Error in semwait (activateCacheManager)");
            return;
        }

        if(insert(imageToInsert)){
            perror("Error insert");
            return;
        }

        //posting to insert a new element into the cache
        if(sem_post(&cacheManagerHasFinished) == 0){
            perror("Error in sempost (activateCacheManager)");
            return;
        }
    }

}